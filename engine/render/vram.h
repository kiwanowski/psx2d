#ifndef ENGINE_VRAM_H
#define ENGINE_VRAM_H

#include <stdint.h>

/*
 * VRAM layout (1024x512 at 16bpp halfword coordinates):
 *   x=0..319,  y=0..239   — display buffer 0
 *   x=0..319,  y=240..479 — display buffer 1
 *   x=320..959, y=0..479  — texture area  (font lives at x=960,y=0)
 *   x=0..1023, y=480..511 — CLUT row + spare
 *
 * Texture coordinates for n-bpp textures:
 *   4bpp:  vram_w = pixel_w / 4   (4 pixels per 16bpp word)
 *   8bpp:  vram_w = pixel_w / 2
 *  16bpp:  vram_w = pixel_w
 */

typedef struct
{
    int x, y; /* VRAM coordinates (16bpp halfword units) */
    int w, h; /* extent in the same units                */
} VRAMRegion;

/* Reset the bump allocator — call once from engine_renderer_init() */
void vram_init(void);

/*
 * Allocate a texture region.
 *   pixel_w / pixel_h : dimensions in pixels
 *   mode              : 0=4bpp  1=8bpp  2=16bpp
 *   out               : filled on success
 * Returns 1 on success, 0 when VRAM is exhausted.
 */
int vram_alloc(int pixel_w, int pixel_h, int mode, VRAMRegion *out);

/*
 * Upload pixel data to a previously-allocated VRAMRegion.
 * data must be 32-bit aligned (standard for PSn00bSDK LoadImage).
 * Calls DrawSync(0) after upload.
 */
void vram_upload(const VRAMRegion *region, const uint32_t *data);

/*
 * Upload a 16-color (32-byte) CLUT to the CLUT row at y=480.
 *   x_slot : theme index; theme i → x = i*16
 *   colors : array of 16 BGR555 halfwords
 * Calls DrawSync(0) after upload.
 */
void vram_upload_clut(int x_slot, const uint16_t *colors);

/* Return a packed CLUT word (for POLY_FT4.clut) for a given x_slot */
uint16_t vram_get_clut_word(int x_slot);

#endif /* ENGINE_VRAM_H */
