#ifndef ENGINE_PHYSICS_H
#define ENGINE_PHYSICS_H

#include "../core/node.h"
#include "../nodes/collision_shape.h"

#define PHYSICS_MAX_SHAPES 64  /* must match COLLISION_SHAPE_POOL_SIZE */
#define PHYSICS_MAX_RESULTS 32 /* max hits returned from a single query */

/*
 * AABBHit — one result entry returned by physics_query_aabb().
 */
typedef struct
{
    Node *node;                /* the CollisionShape node that was hit */
    CollisionShapeData *shape; /* its data (for w/h/layer inspection)  */
} AABBHit;

/* ── System lifetime ────────────────────────────────────────────────────── */

/* Reset the shape registry — call from physics_init() / scene_change(). */
void physics_init(void);

/* ── Shape registry (called automatically by CollisionShape node) ─────── */
void physics_register(Node *shape_node);
void physics_unregister(Node *shape_node);

/* ── Queries ─────────────────────────────────────────────────────────────
 *
 * physics_query_aabb — find all shapes whose world AABB overlaps the
 *   test rectangle (x, y, w, h) AND whose layer_mask & scan_mask != 0.
 *   Results written into 'out' (up to 'max' entries).
 *   Returns the number of hits (0 = no overlap).
 *
 * physics_check_pair — returns 1 if the two CollisionShape nodes' world
 *   AABBs overlap, 0 otherwise.  Does NOT check masks.
 */
int physics_query_aabb(int x, int y, int w, int h,
                       uint32_t scan_mask,
                       AABBHit *out, int max);

int physics_check_pair(const Node *a, const Node *b);

/*
 * physics_update — refresh world AABB of every registered shape from its
 *   parent Node2D world transform.  Call once per frame BEFORE queries,
 *   after tree_update() so all Node2D transforms are current.
 */
void physics_update(void);

#endif /* ENGINE_PHYSICS_H */
