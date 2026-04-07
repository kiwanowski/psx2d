#include "collision_shape.h"
#include "../core/physics.h"
#include <string.h>

/* ── Sub-pool ─────────────────────────────────────────────────────────────── */

static CollisionShapeData s_pool[COLLISION_SHAPE_POOL_SIZE];
static int s_used[COLLISION_SHAPE_POOL_SIZE];

void collision_shape_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
}

static CollisionShapeData *cs_data_alloc(void)
{
    int i;
    for (i = 0; i < COLLISION_SHAPE_POOL_SIZE; i++)
    {
        if (!s_used[i])
        {
            s_used[i] = 1;
            memset(&s_pool[i], 0, sizeof(CollisionShapeData));
            return &s_pool[i];
        }
    }
    return 0;
}

static void cs_data_free(CollisionShapeData *d)
{
    int i;
    for (i = 0; i < COLLISION_SHAPE_POOL_SIZE; i++)
    {
        if (&s_pool[i] == d)
        {
            s_used[i] = 0;
            return;
        }
    }
}

/* ── World AABB refresh ───────────────────────────────────────────────────── */

static void cs_refresh_world(Node *self)
{
    CollisionShapeData *d = (CollisionShapeData *)self->data;
    Node *p = self->parent;
    int px = 0, py = 0;

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

/* ── Callbacks ────────────────────────────────────────────────────────────── */

static void cs_init_cb(Node *self)
{
    cs_refresh_world(self);
    physics_register(self);
}

static void cs_update_cb(Node *self, int dt)
{
    (void)dt;
    cs_refresh_world(self);
}

static void cs_destroy_cb(Node *self)
{
    physics_unregister(self);
    if (self->data)
        cs_data_free((CollisionShapeData *)self->data);
    self->data = 0;
}

/* ── Factory ──────────────────────────────────────────────────────────────── */

Node *collision_shape_create(const char *name)
{
    CollisionShapeData *d;
    Node *n = node_alloc(NODE_TYPE_COLLISION_SHAPE, name);
    if (!n)
        return 0;
    d = cs_data_alloc();
    if (!d)
    {
        node_free(n);
        return 0;
    }
    n->data = d;
    n->init = cs_init_cb;
    n->update = cs_update_cb;
    n->draw = 0;
    n->destroy = cs_destroy_cb;
    return n;
}
