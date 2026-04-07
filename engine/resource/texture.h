#ifndef ENGINE_TEXTURE_RESOURCE_H
#define ENGINE_TEXTURE_RESOURCE_H

#include <stdint.h>
#include <psxcd.h>
#include "../render/vram.h"

#define TEXTURE_RESOURCE_POOL_SIZE 8

/*
 * TextureResource — a TIM image uploaded to VRAM, ready for use in
 * POLY_FT4 / SPRT primitives.
 *
 * Use texture_resource_load() to load a TIM from CD-ROM, or
 * texture_resource_from_tim() to upload an already-in-RAM TIM blob
 * (e.g. from psn00bsdk_target_incbin).
 */
typedef struct
{
    uint16_t tpage;          /* packed tpage word for POLY_FT4.tpage  */
    uint16_t clut_word;      /* packed CLUT word; 0 if no CLUT (16bpp)*/
    VRAMRegion pixel_region; /* where pixel data lives in VRAM        */
    VRAMRegion clut_region;  /* where the CLUT lives (w=0 if none)    */
    int valid;               /* 1 if upload succeeded                 */
} TextureResource;

/*
 * texture_resource_from_tim — upload a TIM blob already present in RAM.
 *   tim_data : 32-bit aligned pointer to raw TIM data.
 *   out      : filled on success.
 *   Returns 1 on success, 0 if VRAM is exhausted.
 */
int texture_resource_from_tim(const uint32_t *tim_data, TextureResource *out);

/*
 * texture_resource_load — load a TIM file from CD-ROM into a temp sector
 *   buffer, then upload to VRAM.
 *   cdpath : full PS1 path, e.g. "\\TILES.TIM;1"
 *   out    : filled on success.
 *   Returns 1 on success.
 *
 *   NOTE: uses a static 64 KB sector buffer — not re-entrant.
 *         Call only outside of the CD interrupt context.
 */
int texture_resource_load(const char *cdpath, TextureResource *out);

/* Sub-pool reset — call when starting a new scene */
void texture_resource_pool_init(void);

#endif /* ENGINE_TEXTURE_RESOURCE_H */
