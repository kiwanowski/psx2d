#ifndef ENGINE_RENDERER_H
#define ENGINE_RENDERER_H

#include <stdint.h>
#include "ot.h"

#define ENGINE_SCREEN_W 320
#define ENGINE_SCREEN_H 240

/*
 * engine_renderer_init — call once at startup, after ResetGraph(0).
 *   and resets the VRAM allocator.  Theme CLUTs are NOT uploaded here;
 *   call engine_renderer_set_theme() to activate a theme on demand.
 */
void engine_renderer_init(void);

/*
 * engine_renderer_load_atlas — upload a TIM sprite atlas to VRAM.
 *   tim_data : pointer to the raw TIM blob (e.g. from psn00bsdk_target_incbin)
 *   out_clut : if non-NULL, receives the packed CLUT word for the TIM's
 *              embedded palette (0 for 16bpp TIMs with no CLUT).  Pass NULL
 *              if the TIM uses the active theme palette instead.
 *   Returns the packed tpage word for use in POLY_FT4.tpage.
 *   Call after engine_renderer_init().
 */
uint16_t engine_renderer_load_atlas(const uint32_t *tim_data, uint16_t *out_clut);

/* Per-frame bookends — call at start and end of every game-loop tick */
void engine_renderer_begin_frame(void);
void engine_renderer_end_frame(void); /* FntFlush → DrawSync → VSync → swap */

/*
 * engine_render_alloc_prim — reserve 'size' bytes in the current prim buffer.
 *   Returns a pointer the caller fills with a PSn00bSDK primitive struct,
 *   then passes to addPrim() before calling end_frame.
 *   No bounds check past the 32 KB buffer — callers must stay within budget.
 */
void *engine_render_alloc_prim(int size);

/*
 * engine_render_ot — pointer to OT entry at z_layer, for use with addPrim().
 *   z_layer must be in [0, ENGINE_OTLEN).
 */
uint32_t *engine_render_ot(int z_layer);

/*
 * engine_renderer_set_theme — switch the active palette.
 *   Calls engine_theme_set_active(), updates CLUT word, syncs draw-env
 *   clear colour.  Safe to call at any time (takes effect next frame).
 */
void engine_renderer_set_theme(int theme_id);

/* Packed CLUT word for the active theme — use as POLY_FT4.clut */
uint16_t engine_renderer_active_clut(void);

#endif /* ENGINE_RENDERER_H */
