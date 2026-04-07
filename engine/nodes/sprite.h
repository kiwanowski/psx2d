#ifndef ENGINE_SPRITE_H
#define ENGINE_SPRITE_H

#include "../core/node.h"
#include "../core/scene.h"
#include "node2d.h"
#include <stdint.h>

#define SPRITE_POOL_SIZE 64

/*
 * SpriteData — payload for NODE_TYPE_SPRITE nodes.
 *
 * The sprite reads its world position from the nearest Node2D ancestor each
 * draw() call.  The active-camera scroll offset is subtracted to produce
 * the final screen position.
 *
 * Rotation is taken from the parent Node2D's wrot (GTE units 0..4095),
 * so a rotating Node2D parent automatically spins its Sprite children.
 * Set use_parent_rot = 0 to draw axis-aligned regardless of rotation.
 *
 * When use_theme_clut = 1, the sprite uses engine_renderer_active_clut()
 * every frame, so CLUT switches (e.g. portal theme change) are free.
 * Set use_theme_clut = 0 and fill clut manually for sprites with their
 * own fixed palette.
 */
typedef struct
{
    SceneTree *tree; /* back-pointer for active_camera lookup */

    /* Atlas source rectangle (texels in the sprite atlas) */
    int u, v; /* top-left corner in the atlas */
    int w, h; /* sprite dimensions in pixels */

    /* Renderer handles */
    uint16_t tpage;     /* packed tpage word (from engine_renderer_load_atlas) */
    uint16_t clut;      /* packed CLUT word; overwritten each frame if use_theme_clut */
    int use_theme_clut; /* 1 = follow active theme, 0 = use clut field as-is */

    /* Draw options */
    int z_index;        /* OT layer — use ENGINE_OT_* constants */
    int flip_x;         /* 1 = mirror horizontally */
    int use_parent_rot; /* 1 = inherit Node2D parent rotation */

    /* Draw-time screen offsets applied on top of world position (signed) */
    int origin_x; /* local draw origin X (e.g. -w/2 to centre) */
    int origin_y; /* local draw origin Y */
} SpriteData;

/*
 * sprite_create — allocate and wire up a Sprite node.
 *   tree  : SceneTree (for active_camera access at draw time).
 *   name  : node name.
 */
Node *sprite_create(SceneTree *tree, const char *name);

/* Sub-pool reset */
void sprite_pool_init(void);

#endif /* ENGINE_SPRITE_H */
