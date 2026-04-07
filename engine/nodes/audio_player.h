#ifndef ENGINE_AUDIO_PLAYER_H
#define ENGINE_AUDIO_PLAYER_H

#include "../core/node.h"
#include "../resource/audio.h"

#define AUDIO_PLAYER_POOL_SIZE 4

/*
 * AudioPlayerData — payload for NODE_TYPE_AUDIO_PLAYER nodes.
 *
 * Wraps XA streaming via PSn00bSDK CdControl / SpuSetCommonXxx.
 * One AudioPlayer should be active at a time per channel; the PS1 XA
 * hardware only supports a single active filter.
 *
 * Lifecycle:
 *   audio_player_play()  — sets filter, starts CdlReadS
 *   audio_player_stop()  — sends CdlPause
 *   Looping is handled via xa_event_handler registered with CdReadyCallback:
 *     when the end-of-data sentinel is detected, it reissues CdlReadS.
 *
 * IMPORTANT: CdInit() and SpuInit() must be called before any AudioPlayer
 * node is used.  The engine does NOT call these — add them to your main
 * init sequence.
 */
typedef struct
{
    AudioResource *resource; /* pointer to a loaded AudioResource */
    int playing;             /* 1 while streaming */
    int looping;             /* 1 = auto-restart on end-of-data    */
} AudioPlayerData;

/*
 * audio_player_create — allocate and wire up an AudioPlayer node.
 */
Node *audio_player_create(const char *name);

/*
 * audio_player_play / audio_player_stop — start or halt XA streaming
 * on the node's assigned resource.
 * Safe to call if resource == NULL or resource->valid == 0 (no-ops).
 */
void audio_player_play(Node *node);
void audio_player_stop(Node *node);

/* Sub-pool reset */
void audio_player_pool_init(void);

#endif /* ENGINE_AUDIO_PLAYER_H */
