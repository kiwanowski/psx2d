#include "node2d.h"
#include <string.h>

/* ── Sub-pool ─────────────────────────────────────────────────────────────── */

static Node2DData s_pool[NODE2D_POOL_SIZE];
static int s_used[NODE2D_POOL_SIZE];

void node2d_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
}

static Node2DData *node2d_data_alloc(void)
{
    int i;
    for (i = 0; i < NODE2D_POOL_SIZE; i++)
    {
        if (!s_used[i])
        {
            s_used[i] = 1;
            memset(&s_pool[i], 0, sizeof(Node2DData));
            return &s_pool[i];
        }
    }
    return 0;
}

static void node2d_data_free(Node2DData *d)
{
    int i;
    for (i = 0; i < NODE2D_POOL_SIZE; i++)
    {
        if (&s_pool[i] == d)
        {
            s_used[i] = 0;
            return;
        }
    }
}

/* ── World transform ──────────────────────────────────────────────────────── */

void node2d_update_world_transform(Node *node)
{
    Node2DData *d;
    Node *p;
    int px = 0, py = 0, pr = 0;

    if (!node || node->type != NODE_TYPE_NODE2D)
        return;
    d = (Node2DData *)node->data;

    /* Walk up to first Node2D ancestor */
    p = node->parent;
    while (p)
    {
        if (p->type == NODE_TYPE_NODE2D)
        {
            Node2DData *pd = (Node2DData *)p->data;
            px = pd->wx;
            py = pd->wy;
            pr = pd->wrot;
            break;
        }
        p = p->parent;
    }

    d->wx = px + FIXED_TO_INT(d->x);
    d->wy = py + FIXED_TO_INT(d->y);
    d->wrot = (pr + d->rot) & 4095;
}

void node2d_get_world(const Node *node, int *wx, int *wy)
{
    if (node && node->type == NODE_TYPE_NODE2D)
    {
        Node2DData *d = (Node2DData *)node->data;
        *wx = d->wx;
        *wy = d->wy;
    }
    else
    {
        *wx = 0;
        *wy = 0;
    }
}

/* ── Callbacks ────────────────────────────────────────────────────────────── */

static void node2d_init_cb(Node *self)
{
    node2d_update_world_transform(self);
}

static void node2d_update_cb(Node *self, int dt)
{
    (void)dt;
    node2d_update_world_transform(self);
}

static void node2d_destroy_cb(Node *self)
{
    if (self->data)
        node2d_data_free((Node2DData *)self->data);
    self->data = 0;
}

/* ── Factory ──────────────────────────────────────────────────────────────── */

Node *node2d_create(const char *name)
{
    Node2DData *d;
    Node *n = node_alloc(NODE_TYPE_NODE2D, name);
    if (!n)
        return 0;
    d = node2d_data_alloc();
    if (!d)
    {
        node_free(n);
        return 0;
    }
    n->data = d;
    n->init = node2d_init_cb;
    n->update = node2d_update_cb;
    n->draw = 0;
    n->destroy = node2d_destroy_cb;
    return n;
}
