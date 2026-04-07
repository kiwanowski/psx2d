#ifndef ENGINE_AUDIO_RESOURCE_H
#define ENGINE_AUDIO_RESOURCE_H

#include <psxcd.h>

/*
 * AudioResource — a located CD-ROM file ready to stream as XA audio.
 *
 * Load once at scene start; pass the pointer to AudioPlayerData.
 * Storage is static — no heap allocation.
 */
typedef struct
{
    CdlLOC loc; /* sector address of the file on disc          */
    int valid;  /* 1 if successfully located, 0 on CD miss     */
    /* XA filter parameters (file/channel) — set before playing    */
    unsigned char xa_file;
    unsigned char xa_channel;
} AudioResource;

#define AUDIO_RESOURCE_POOL_SIZE 8

/*
 * audio_resource_load — search the CD directory for 'cdpath' (e.g.
 *   "\\MUSIC.XA;1") and fill an AudioResource.
 *   Returns 1 on success, 0 if the file was not found.
 *   Resources are allocated from a small static pool; call
 *   audio_resource_pool_init() at scene start to reset it.
 */
int audio_resource_load(const char *cdpath,
                        unsigned char xa_file,
                        unsigned char xa_channel,
                        AudioResource *out);

void audio_resource_pool_init(void);

#endif /* ENGINE_AUDIO_RESOURCE_H */
