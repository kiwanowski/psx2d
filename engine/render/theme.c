#include "theme.h"

/* ── Built-in theme data ─────────────────────────────────────────────────── */
/* Order matches ENGINE_THEME_* constants: 0=CLASSIC_GD, 1=SYNTHWAVE, 2=LAVA */

const EngineTheme g_engine_themes[ENGINE_THEME_COUNT] = {

    // 16 CLUT colors
    {
        {
            0x30E4, /* 0:  grey 60  → rgb(37,60,103)          */
            0x7F2F, /* 1:  grey 200 → rgb(125,203,255)        */
            0x0000, /* 2:  unused                             */
            0x7F71, /* 3:  grey 220 → rgb(137,223,255)        */
            0x65E9, /* 4:  grey 120 → rgb(75,121,206)         */
            0x7E6B, /* 5:  grey 150 → rgb(93,152,255) ← base  */
            0x0000,
            0x0000,
            0x0000,
            0x0000,
            0x0000,
            0x0000,
            0x0000,
            0x0000,
            0x0000,
            0x0000,
        }},

    {{
        0x3069, /* 0:  grey 60  → rgb(75,28,103)          */
        0x7D7F, /* 1:  grey 200 → rgb(250,93,255)         */
        0x0000, /* 2:  unused                             */
        0x7D9F, /* 3:  grey 220 → rgb(255,103,255)        */
        0x64F2, /* 4:  grey 120 → rgb(150,56,206)         */
        0x7D17, /* 5:  grey 150 → rgb(187,70,255) ← base  */
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
    }},

    {{
        0x0000,                      /* 0:  transparent          */
        0x8000,                      /* 1:  black (STP bit set)  */
        ENGINE_RGB16(60, 20, 0),     /* 2:  darkest              */
        ENGINE_RGB16(120, 40, 0),    /* 3:  dark                 */
        ENGINE_RGB16(180, 70, 10),   /* 4:  mid                  */
        ENGINE_RGB16(220, 100, 20),  /* 5:  base (lava) ← base   */
        ENGINE_RGB16(240, 140, 40),  /* 6:  highlight            */
        ENGINE_RGB16(255, 180, 80),  /* 7:  bright               */
        ENGINE_RGB16(255, 210, 120), /* 8: brightest            */
        ENGINE_RGB16(255, 235, 175), /* 9: near-white (220)     */
        ENGINE_RGB16(255, 248, 225), /* 10: near-white (240)    */
        ENGINE_RGB16(255, 255, 255), /* 11: pure white          */
        0x0000,
        0x0000,
        0x0000,
        0x0000, /* 12-15: reserved    */
    }},
};

/* ── Active theme ─────────────────────────────────────────────────────────── */

static const EngineTheme *s_active = &g_engine_themes[0]; /* default to theme 0 */

void engine_theme_set_active(int theme_id)
{
    if (theme_id >= 0 && theme_id < ENGINE_THEME_COUNT)
        s_active = &g_engine_themes[theme_id];
}

const EngineTheme *engine_theme_get_active(void)
{
    return s_active;
}
