#include "camera2d.h"
#include <string.h>

/* ── Sub-pool ─────────────────────────────────────────────────────────────── */

static Camera2DData s_pool[CAMERA2D_POOL_SIZE];
static int s_used[CAMERA2D_POOL_SIZE];

void camera2d_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
}

static Camera2DData *cam_data_alloc(void)
{
    int i;
    for (i = 0; i < CAMERA2D_POOL_SIZE; i++)
    {
        if (!s_used[i])
        {
            s_used[i] = 1;
            memset(&s_pool[i], 0, sizeof(Camera2DData));
            return &s_pool[i];
        }
    }
    return 0;
}

static void cam_data_free(Camera2DData *d)
{
    int i;
    for (i = 0; i < CAMERA2D_POOL_SIZE; i++)
    {
        if (&s_pool[i] == d)
        {
            s_used[i] = 0;
            return;
        }
    }
}

/* ── Offset query ─────────────────────────────────────────────────────────── */

void camera2d_get_offset(const Node *cam, int *ox, int *oy)
{
    Camera2DData *d;
    Node *p;

    *ox = 0;
    *oy = 0;
    if (!cam || cam->type != NODE_TYPE_CAMERA2D)
        return;
    d = (Camera2DData *)cam->data;
    *ox = d->ox;
    *oy = d->oy;

    /* Add nearest Node2D ancestor's world position */
    p = cam->parent;
    while (p)
    {
        if (p->type == NODE_TYPE_NODE2D)
        {
            int wx, wy;
            node2d_get_world(p, &wx, &wy);
            *ox += wx;
            *oy += wy;
            break;
        }
        p = p->parent;
    }
}

/* ── Callbacks ────────────────────────────────────────────────────────────── */

static void cam_init_cb(Node *self)
{
    Camera2DData *d = (Camera2DData *)self->data;
    if (d->tree)
        d->tree->active_camera = self;
}

static void cam_destroy_cb(Node *self)
{
    Camera2DData *d = (Camera2DData *)self->data;
    /* Unregister from scene if still active */
    if (d->tree && d->tree->active_camera == self)
        d->tree->active_camera = 0;
    cam_data_free(d);
    self->data = 0;
}

/* ── Factory ──────────────────────────────────────────────────────────────── */

Node *camera2d_create(SceneTree *tree, const char *name)
{
    Camera2DData *d;
    Node *n = node_alloc(NODE_TYPE_CAMERA2D, name);
    if (!n)
        return 0;
    d = cam_data_alloc();
    if (!d)
    {
        node_free(n);
        return 0;
    }
    d->tree = tree;
    n->data = d;
    n->init = cam_init_cb;
    n->update = 0; /* camera position is driven by its Node2D parent */
    n->draw = 0;
    n->destroy = cam_destroy_cb;
    return n;
}
