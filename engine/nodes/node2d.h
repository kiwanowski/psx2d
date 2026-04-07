#ifndef ENGINE_NODE2D_H
#define ENGINE_NODE2D_H

#include "../core/node.h"
#include "../math/fixed.h"

#define NODE2D_POOL_SIZE 64

/*
 * Node2DData — payload for NODE_TYPE_NODE2D nodes.
 *
 * Local transform is relative to the nearest Node2D ancestor.
 * World transform is recomputed at the start of update() by walking up the
 * parent chain.  draw() reads the cached world values.
 *
 * Rotation is stored in GTE units (0-4095 = full circle), matching rcos/rsin.
 */
typedef struct
{
    /* Local transform (relative to parent Node2D, or screen if no parent) */
    fixed_t x, y; /* position in pixels (16.16) */
    int rot;      /* rotation in GTE units 0..4095 */

    /* World transform (screen space) — updated every frame in update() */
    int wx, wy; /* integer pixel position */
    int wrot;   /* accumulated rotation, GTE units */
} Node2DData;

/*
 * node2d_create — allocate a Node2D from the node pool + data sub-pool.
 * Returns NULL if either pool is exhausted.
 */
Node *node2d_create(const char *name);

/*
 * node2d_get_world — read the cached world-space position of any node.
 * If the node is not NODE_TYPE_NODE2D, wx and wy are set to 0.
 */
void node2d_get_world(const Node *node, int *wx, int *wy);

/*
 * node2d_update_world_transform — recompute wx/wy/wrot from parent chain.
 * Called automatically from the node's update() callback; exposed so
 * init() can also seed initial world coordinates.
 */
void node2d_update_world_transform(Node *node);

/* Sub-pool reset — called from node_pool_init() via scene reset */
void node2d_pool_init(void);

#endif /* ENGINE_NODE2D_H */
