#ifndef ENGINE_COLLISION_SHAPE_H
#define ENGINE_COLLISION_SHAPE_H

#include "../core/node.h"
#include "node2d.h"
#include <stdint.h>

#define COLLISION_SHAPE_POOL_SIZE 64

/*
 * Layer / group masks — 32 bits each, game defines meaning.
 * Predefined bits:
 *   LAYER_WORLD   : static solid geometry (ground, tiles)
 *   LAYER_PLAYER  : the player body
 *   LAYER_HAZARD  : spikes, fatal objects
 *   LAYER_COLLECT : collectibles (coins)
 *   LAYER_SENSOR  : non-solid triggers (portals, finish line)
 */
#define PHYS_LAYER_WORLD (1u << 0)
#define PHYS_LAYER_PLAYER (1u << 1)
#define PHYS_LAYER_HAZARD (1u << 2)
#define PHYS_LAYER_COLLECT (1u << 3)
#define PHYS_LAYER_SENSOR (1u << 4)

typedef struct
{
    /* Local offset of the AABB relative to the parent Node2D origin */
    int ox, oy; /* top-left corner offset in pixels */
    int w, h;   /* width and height in pixels */

    /* Filtering: this shape resides on layer_mask and collides with
       objects whose layer_mask overlaps scan_mask. */
    uint32_t layer_mask; /* which layer(s) this shape is on      */
    uint32_t scan_mask;  /* which layer(s) this shape tests against */

    /* Sensor flag: 1 = overlap-only, never treated as solid */
    int sensor;

    /* Cached world AABB — updated every frame in update() */
    int world_x, world_y; /* top-left in world pixels */
} CollisionShapeData;

/*
 * collision_shape_create — allocate and wire up a CollisionShape node.
 * The shape auto-registers with the physics system on init() and
 * deregisters on destroy().
 */
Node *collision_shape_create(const char *name);

/* Sub-pool reset */
void collision_shape_pool_init(void);

#endif /* ENGINE_COLLISION_SHAPE_H */
