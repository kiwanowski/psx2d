#include "physics.h"
#include "../nodes/collision_shape.h"
#include <string.h>

/* ── Shape registry ───────────────────────────────────────────────────────── */

static Node *s_shapes[PHYSICS_MAX_SHAPES];
static int s_shape_count = 0;

void physics_init(void)
{
    memset(s_shapes, 0, sizeof(s_shapes));
    s_shape_count = 0;
}

void physics_register(Node *shape_node)
{
    int i;
    if (!shape_node || s_shape_count >= PHYSICS_MAX_SHAPES)
        return;
    /* Avoid duplicates */
    for (i = 0; i < s_shape_count; i++)
    {
        if (s_shapes[i] == shape_node)
            return;
    }
    s_shapes[s_shape_count++] = shape_node;
}

void physics_unregister(Node *shape_node)
{
    int i, j;
    for (i = 0; i < s_shape_count; i++)
    {
        if (s_shapes[i] == shape_node)
        {
            for (j = i; j < s_shape_count - 1; j++)
                s_shapes[j] = s_shapes[j + 1];
            s_shapes[--s_shape_count] = 0;
            return;
        }
    }
}

/* ── Internal AABB overlap test ───────────────────────────────────────────── */

static int aabb_overlap(int ax, int ay, int aw, int ah,
                        int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

/* ── physics_update ───────────────────────────────────────────────────────── */

/*
 * Refresh world AABBs — called once per frame after tree_update() so all
 * Node2D transforms are already up to date.  The CollisionShape's own
 * update() callback also does this per-node, so this function is a
 * belt-and-suspenders guarantee for shapes that may have been skipped.
 */
void physics_update(void)
{
    int i;
    for (i = 0; i < s_shape_count; i++)
    {
        Node *n = s_shapes[i];
        CollisionShapeData *d;
        Node *p;
        int px = 0, py = 0;

        if (!n || !n->active)
            continue;
        d = (CollisionShapeData *)n->data;
        p = n->parent;
        while (p)
        {
            if (p->type == NODE_TYPE_NODE2D)
            {
                node2d_get_world(p, &px, &py);
                break;
            }
            p = p->parent;
        }
        d->world_x = px + d->ox;
        d->world_y = py + d->oy;
    }
}

/* ── physics_query_aabb ───────────────────────────────────────────────────── */

int physics_query_aabb(int x, int y, int w, int h,
                       uint32_t scan_mask,
                       AABBHit *out, int max)
{
    int i, count = 0;
    for (i = 0; i < s_shape_count && count < max; i++)
    {
        Node *n = s_shapes[i];
        CollisionShapeData *d;

        if (!n || !n->active)
            continue;
        d = (CollisionShapeData *)n->data;

        if (!(d->layer_mask & scan_mask))
            continue;

        if (aabb_overlap(x, y, w, h, d->world_x, d->world_y, d->w, d->h))
        {
            out[count].node = n;
            out[count].shape = d;
            count++;
        }
    }
    return count;
}

/* ── physics_check_pair ───────────────────────────────────────────────────── */

int physics_check_pair(const Node *a, const Node *b)
{
    CollisionShapeData *da, *db;
    if (!a || !b ||
        a->type != NODE_TYPE_COLLISION_SHAPE ||
        b->type != NODE_TYPE_COLLISION_SHAPE)
        return 0;
    da = (CollisionShapeData *)a->data;
    db = (CollisionShapeData *)b->data;
    return aabb_overlap(da->world_x, da->world_y, da->w, da->h,
                        db->world_x, db->world_y, db->w, db->h);
}
