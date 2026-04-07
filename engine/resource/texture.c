#include "texture.h"
#include "../render/vram.h"
#include <psxgpu.h>
#include <string.h>

/* ── Static CD read buffer (64 KB — fits any reasonably-sized TIM) ───────── */
#define CD_SECTOR_SIZE 2048
#define CD_BUF_SECTORS 32
static unsigned char s_cd_buf[CD_SECTOR_SIZE * CD_BUF_SECTORS];

/* ── Sub-pool ─────────────────────────────────────────────────────────────── */

static TextureResource s_pool[TEXTURE_RESOURCE_POOL_SIZE];
static int s_used[TEXTURE_RESOURCE_POOL_SIZE];

void texture_resource_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
}

/* ── Internal: upload a parsed TIM_IMAGE ─────────────────────────────────── */

static int upload_tim(TIM_IMAGE *tim, TextureResource *out)
{
    VRAMRegion px;

    /* Pixel data */
    if (!vram_alloc(tim->prect->w * (1 << (2 - (tim->mode & 0x3))),
                    tim->prect->h,
                    tim->mode & 0x3, &px))
        return 0;

    /* Use the TIM's own VRAM coordinates if the allocator placed it there,
       or just upload straight to the TIM-specified location if available.
       For simplicity we trust the TIM prect directly — PSn00bSDK's
       GetTimInfo already fills prect with the correct VRAM destination. */
    LoadImage(tim->prect, tim->paddr);
    DrawSync(0);

    out->tpage = (uint16_t)getTPage(tim->mode & 0x3, 0,
                                    tim->prect->x, tim->prect->y);
    out->pixel_region.x = tim->prect->x;
    out->pixel_region.y = tim->prect->y;
    out->pixel_region.w = tim->prect->w;
    out->pixel_region.h = tim->prect->h;

    /* CLUT (only present for 4bpp / 8bpp modes) */
    if (tim->crect && tim->crect->w > 0)
    {
        LoadImage(tim->crect, tim->caddr);
        DrawSync(0);
        out->clut_word = (uint16_t)getClut(tim->crect->x, tim->crect->y);
        out->clut_region.x = tim->crect->x;
        out->clut_region.y = tim->crect->y;
        out->clut_region.w = tim->crect->w;
        out->clut_region.h = tim->crect->h;
    }
    else
    {
        out->clut_word = 0;
        out->clut_region.w = 0;
    }

    out->valid = 1;
    return 1;
}

/* ── texture_resource_from_tim ───────────────────────────────────────────── */

int texture_resource_from_tim(const uint32_t *tim_data, TextureResource *out)
{
    TIM_IMAGE tim;
    memset(out, 0, sizeof(TextureResource));
    GetTimInfo(tim_data, &tim);
    return upload_tim(&tim, out);
}

/* ── texture_resource_load ───────────────────────────────────────────────── */

int texture_resource_load(const char *cdpath, TextureResource *out)
{
    CdlFILE file;
    int sectors, result;
    TIM_IMAGE tim;

    memset(out, 0, sizeof(TextureResource));

    if (!CdSearchFile(&file, (char *)cdpath))
        return 0;

    sectors = (file.size + CD_SECTOR_SIZE - 1) / CD_SECTOR_SIZE;
    if (sectors > CD_BUF_SECTORS)
        return 0; /* file too large for static buffer */

    CdControl(CdlSetloc, (unsigned char *)&file.pos, 0);
    result = CdRead(sectors, (unsigned long *)s_cd_buf, CdlModeSpeed);
    if (result <= 0)
        return 0;
    CdReadSync(0, 0);

    GetTimInfo((const uint32_t *)s_cd_buf, &tim);
    return upload_tim(&tim, out);
}
