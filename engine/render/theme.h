#ifndef ENGINE_THEME_H
#define ENGINE_THEME_H

#include <stdint.h>

/* ── Theme IDs ────────────────────────────────────────────────────────────── */
#define ENGINE_THEME_COUNT 3

/* ── PS1 BGR555 colour packing ───────────────────────────────────────────── */
#define ENGINE_RGB16(r, g, b)               \
    ((uint16_t)((((b) >> 3) & 0x1F) << 10 | \
                (((g) >> 3) & 0x1F) << 5 |  \
                (((r) >> 3) & 0x1F)))

/* ── Colour types ────────────────────────────────────────────────────────── */
typedef struct
{
    unsigned char r, g, b;
} EngineRGB8;

/*
 * Theme — describes one full visual style.
 *
 * colors[16]: 4-bit CLUT uploaded to VRAM once at startup.
 *   Sprites in the spritesheet use these indices; swapping the CLUT changes look.
 *
 * Everything else is for flat-shaded (untextured) primitives.
 */
typedef struct
{
    uint16_t colors[16];
} EngineTheme;

/* Sets a PSn00bSDK primitive's RGB0 from an EngineTheme field name */
#define ENGINE_SETRGB_TH(prim, field)           \
    setRGB0((prim),                             \
            engine_theme_get_active()->field.r, \
            engine_theme_get_active()->field.g, \
            engine_theme_get_active()->field.b)

/* Built-in theme table */
extern const EngineTheme g_engine_themes[ENGINE_THEME_COUNT];

/* Active-theme API — called by engine_renderer_set_theme() */
void engine_theme_set_active(int theme_id);
const EngineTheme *engine_theme_get_active(void);

#endif /* ENGINE_THEME_H */
