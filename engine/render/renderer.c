#include "renderer.h"
#include "theme.h"
#include "vram.h"
#include <psxgpu.h>
#include <psxgte.h>
#include <psxetc.h>

/* ── Double-buffer state ─────────────────────────────────────────────────── */
static DISPENV s_disp[2];
static DRAWENV s_draw[2];
static int s_db = 0;

static uint32_t s_ot[2][ENGINE_OTLEN];
static char s_pribuff[2][32768];
static char *s_nextpri;

/* Active packed CLUT word (getTPage result for the current theme slot) */
static uint16_t s_clut;

/* ── Internal helpers ────────────────────────────────────────────────────── */

/* Sync both draw-env clear colours to the current active theme sky */
static void sync_draw_env_bg(void)
{
    const EngineTheme *t = engine_theme_get_active();
}

/* ── engine_renderer_init ────────────────────────────────────────────────── */
void engine_renderer_init(void)
{
    int i;

    ResetGraph(0);
    vram_init();

    /* Buffer 0: display y=0,  draw area y=240 */
    SetDefDispEnv(&s_disp[0], 0, 0, ENGINE_SCREEN_W, ENGINE_SCREEN_H);
    SetDefDrawEnv(&s_draw[0], 0, 240, ENGINE_SCREEN_W, ENGINE_SCREEN_H);
    /* Buffer 1: display y=240, draw area y=0  */
    SetDefDispEnv(&s_disp[1], 0, 240, ENGINE_SCREEN_W, ENGINE_SCREEN_H);
    SetDefDrawEnv(&s_draw[1], 0, 0, ENGINE_SCREEN_W, ENGINE_SCREEN_H);

    s_draw[0].isbg = 1;
    s_draw[1].isbg = 1;
    sync_draw_env_bg();

    s_db = 0;
    s_nextpri = s_pribuff[0];
    PutDrawEnv(&s_draw[1]); /* start drawing into buffer 0's draw area */

    /* Debug font — fixed position outside the texture allocator zone */
    FntLoad(960, 0);
    FntOpen(4, 16, ENGINE_SCREEN_W - 8, ENGINE_SCREEN_H - 8, 0, 512);

    /* Upload all theme CLUTs to VRAM (y=480, one slot per theme) */
    for (i = 0; i < ENGINE_THEME_COUNT; i++)
        vram_upload_clut(i, g_engine_themes[i].colors);

    s_clut = vram_get_clut_word(0); /* default to theme 0's CLUT */
}

/* ── engine_renderer_load_atlas ──────────────────────────────────────────── */
uint16_t engine_renderer_load_atlas(const uint32_t *tim_data)
{
    TIM_IMAGE tim;
    GetTimInfo(tim_data, &tim);
    LoadImage(tim.prect, tim.paddr);
    DrawSync(0);
    return (uint16_t)getTPage(tim.mode & 0x3, 0, tim.prect->x, tim.prect->y);
}

/* ── engine_renderer_begin_frame ─────────────────────────────────────────── */
void engine_renderer_begin_frame(void)
{
    ClearOTagR(s_ot[s_db], ENGINE_OTLEN);
    s_nextpri = s_pribuff[s_db];
}

/* ── engine_renderer_end_frame ───────────────────────────────────────────── */
void engine_renderer_end_frame(void)
{
    FntFlush(-1);
    DrawSync(0);
    VSync(0);
    PutDispEnv(&s_disp[s_db]);
    PutDrawEnv(&s_draw[s_db]);
    SetDispMask(1);
    DrawOTag(s_ot[s_db] + ENGINE_OTLEN - 1);
    s_db = !s_db;
    s_nextpri = s_pribuff[s_db];
}

/* ── engine_render_alloc_prim ────────────────────────────────────────────── */
void *engine_render_alloc_prim(int size)
{
    void *p = s_nextpri;
    s_nextpri += size;
    return p;
}

/* ── engine_render_ot ────────────────────────────────────────────────────── */
uint32_t *engine_render_ot(int z_layer)
{
    return s_ot[s_db] + z_layer;
}

/* ── engine_renderer_set_theme ───────────────────────────────────────────── */
void engine_renderer_set_theme(int theme_id)
{
    engine_theme_set_active(theme_id);
    s_clut = vram_get_clut_word(theme_id);
    sync_draw_env_bg();
}

/* ── engine_renderer_active_clut ─────────────────────────────────────────── */
uint16_t engine_renderer_active_clut(void)
{
    return s_clut;
}
