#include "audio_player.h"
#include <psxapi.h>
#include <psxcd.h>
#include <psxspu.h>
#include <string.h>

/* ── XA sector bookkeeping (shared across all AudioPlayer nodes) ────────── */

/*
 * Only one XA stream can be active at a time on the PS1.
 * We keep a single pointer to the currently-playing node so the CD
 * interrupt handler knows which resource to restart on loop.
 */
static Node *s_active_node = 0;

/* XA subheader audio bit (submode byte, bit 2) */
#define XA_SUBMODE_AUDIO (1 << 2)

typedef struct
{
    unsigned char file, channel, submode, coding_info;
} XA_SubHeader;

typedef struct
{
    CdlLOC pos;
    XA_SubHeader sub[2];
    unsigned char data[2048];
    unsigned int edc;
    unsigned char ecc[276];
} XA_Sector;

static XA_Sector s_xa_sector; /* must be static — read from CD interrupt */

static void xa_event_cb(CdlIntrResult event, unsigned char *payload)
{
    AudioPlayerData *d;
    (void)payload;

    if (event != CdlDataReady)
        return;
    CdGetSector(&s_xa_sector, sizeof(XA_Sector) / 4);

    /* End-of-stream detection: neither subheader is an audio sector */
    if (!(s_xa_sector.sub[0].submode & XA_SUBMODE_AUDIO) &&
        !(s_xa_sector.sub[1].submode & XA_SUBMODE_AUDIO))
    {
        if (!s_active_node)
            return;
        d = (AudioPlayerData *)s_active_node->data;
        if (d->playing && d->looping && d->resource && d->resource->valid)
            CdControlF(CdlReadS, &d->resource->loc);
        else
            d->playing = 0;
    }
}

/* ── Sub-pool ─────────────────────────────────────────────────────────────── */

static AudioPlayerData s_pool[AUDIO_PLAYER_POOL_SIZE];
static int s_used[AUDIO_PLAYER_POOL_SIZE];

void audio_player_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
    s_active_node = 0;
}

static AudioPlayerData *ap_data_alloc(void)
{
    int i;
    for (i = 0; i < AUDIO_PLAYER_POOL_SIZE; i++)
    {
        if (!s_used[i])
        {
            s_used[i] = 1;
            memset(&s_pool[i], 0, sizeof(AudioPlayerData));
            return &s_pool[i];
        }
    }
    return 0;
}

static void ap_data_free(AudioPlayerData *d)
{
    int i;
    for (i = 0; i < AUDIO_PLAYER_POOL_SIZE; i++)
    {
        if (&s_pool[i] == d)
        {
            s_used[i] = 0;
            return;
        }
    }
}

/* ── Play / stop ──────────────────────────────────────────────────────────── */

void audio_player_play(Node *node)
{
    AudioPlayerData *d;
    CdlFILTER filter;
    int mode;

    if (!node || node->type != NODE_TYPE_AUDIO_PLAYER)
        return;
    d = (AudioPlayerData *)node->data;
    if (!d->resource || !d->resource->valid)
        return;

    /* Set XA filter */
    filter.file = d->resource->xa_file;
    filter.chan = d->resource->xa_channel;
    CdControl(CdlSetfilter, (unsigned char *)&filter, 0);

    /* Set CD mode: real-time, XA filter, double-speed */
    mode = CdlModeSpeed | CdlModeRT | CdlModeSF;
    CdControl(CdlSetmode, (unsigned char *)&mode, 0);

    /* Set SPU volumes */
    SpuSetCommonMasterVolume(0x3FFF, 0x3FFF);
    SpuSetCommonCDVolume(0x7FFF, 0x7FFF);

    /* Register interrupt handler and start streaming */
    EnterCriticalSection();
    CdReadyCallback(xa_event_cb);
    ExitCriticalSection();

    d->playing = 1;
    s_active_node = node;
    CdControl(CdlReadS, &d->resource->loc, 0);
}

void audio_player_stop(Node *node)
{
    AudioPlayerData *d;
    if (!node || node->type != NODE_TYPE_AUDIO_PLAYER)
        return;
    d = (AudioPlayerData *)node->data;
    d->playing = 0;
    if (s_active_node == node)
        s_active_node = 0;
    CdControl(CdlPause, 0, 0);
}

/* ── Callbacks ────────────────────────────────────────────────────────────── */

static void ap_destroy_cb(Node *self)
{
    AudioPlayerData *d = (AudioPlayerData *)self->data;
    if (d->playing)
        audio_player_stop(self);
    ap_data_free(d);
    self->data = 0;
}

/* ── Factory ──────────────────────────────────────────────────────────────── */

Node *audio_player_create(const char *name)
{
    AudioPlayerData *d;
    Node *n = node_alloc(NODE_TYPE_AUDIO_PLAYER, name);
    if (!n)
        return 0;
    d = ap_data_alloc();
    if (!d)
    {
        node_free(n);
        return 0;
    }
    d->looping = 1; /* default: loop */
    n->data = d;
    n->init = 0;
    n->update = 0;
    n->draw = 0;
    n->destroy = ap_destroy_cb;
    return n;
}
