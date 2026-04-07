#include "vram.h"
#include <psxgpu.h>

/* ── VRAM allocation constants ───────────────────────────────────────────── */

/* Texture area starts right of both display buffers */
#define TEX_X_START 320
/* Cap x below font area (PSn00bSDK FntLoad uses x=960, y=0..63) */
#define TEX_X_CAP_Y0 960
/* Full width available on rows y >= 64 (font only occupies y=0..63) */
#define TEX_X_CAP 1024
#define TEX_Y_CAP 480 /* CLUT row begins at y=480 */

#define CLUT_VRAM_Y 480
#define CLUT_SLOT_W 16 /* 16 halfwords = 32 bytes per CLUT */

/* ── Bump allocator state ────────────────────────────────────────────────── */

static int s_cur_x;
static int s_cur_y;
static int s_row_h;

void vram_init(void)
{
    s_cur_x = TEX_X_START;
    s_cur_y = 0;
    s_row_h = 0;
}

int vram_alloc(int pixel_w, int pixel_h, int mode, VRAMRegion *out)
{
    int vram_w;
    int cap_x;

    /* Convert pixel width to VRAM halfword width */
    switch (mode)
    {
    case 0:
        vram_w = pixel_w / 4;
        break; /* 4bpp */
    case 1:
        vram_w = pixel_w / 2;
        break; /* 8bpp */
    default:
        vram_w = pixel_w;
        break; /* 16bpp */
    }
    if (vram_w < 1)
        vram_w = 1;

    /* Row x cap: protect font on y=0..63 */
    cap_x = (s_cur_y < 64) ? TEX_X_CAP_Y0 : TEX_X_CAP;

    /* Wrap to next row if needed */
    if (s_cur_x + vram_w > cap_x)
    {
        s_cur_y += s_row_h;
        s_cur_x = TEX_X_START;
        s_row_h = 0;
        cap_x = (s_cur_y < 64) ? TEX_X_CAP_Y0 : TEX_X_CAP;
    }

    if (s_cur_y + pixel_h > TEX_Y_CAP)
        return 0; /* exhausted */

    out->x = s_cur_x;
    out->y = s_cur_y;
    out->w = vram_w;
    out->h = pixel_h;

    s_cur_x += vram_w;
    if (pixel_h > s_row_h)
        s_row_h = pixel_h;

    return 1;
}

void vram_upload(const VRAMRegion *region, const uint32_t *data)
{
    RECT r;
    setRECT(&r, (short)region->x, (short)region->y, (short)region->w, (short)region->h);
    LoadImage(&r, data);
    DrawSync(0);
}

void vram_upload_clut(int x_slot, const uint16_t *colors)
{
    RECT r;
    setRECT(&r, (short)(x_slot * CLUT_SLOT_W), CLUT_VRAM_Y, CLUT_SLOT_W, 1);
    LoadImage(&r, (const uint32_t *)colors);
    DrawSync(0);
}

uint16_t vram_get_clut_word(int x_slot)
{
    return (uint16_t)getClut(x_slot * CLUT_SLOT_W, CLUT_VRAM_Y);
}
