#ifndef ENGINE_H
#define ENGINE_H

/* Math */
#include "math/fixed.h"

/* Core */
#include "core/node.h"
#include "core/signal.h"
#include "core/scene.h"
#include "core/physics.h"

/* Renderer */
#include "render/ot.h"
#include "render/vram.h"
#include "render/theme.h"
#include "render/renderer.h"

/* Nodes */
#include "nodes/node2d.h"
#include "nodes/camera2d.h"
#include "nodes/sprite.h"
#include "nodes/tilemap.h"
#include "nodes/collision_shape.h"
#include "nodes/audio_player.h"

/* Resources */
#include "resource/texture.h"
#include "resource/audio.h"

/*
 * engine_init — one-time startup.
 *   Calls engine_renderer_init() (which calls vram_init / FntLoad / CLUT upload)
 *   and resets all sub-pools.
 *
 *   Callers must have already called ResetGraph(0) before this (or leave it
 *   to engine_renderer_init which calls it internally).
 *
 *   CdInit() / SpuInit() are NOT called here — add them before engine_init()
 *   in main() when you use audio.
 */
void engine_init(void);

#endif /* ENGINE_H */
