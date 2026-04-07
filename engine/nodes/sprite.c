#include "sprite.h"
#include "camera2d.h"
#include "../render/renderer.h"
#include <psxgpu.h>
#include <psxgte.h>
#include <string.h>

/* ── Sub-pool ─────────────────────────────────────────────────────────────── */

static SpriteData s_pool[SPRITE_POOL_SIZE];
static int s_used[SPRITE_POOL_SIZE];

void sprite_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
}

static SpriteData *sprite_data_alloc(void)
{
    int i;
    for (i = 0; i < SPRITE_POOL_SIZE; i++)
    {
        if (!s_used[i])
        {
            s_used[i] = 1;
            memset(&s_pool[i], 0, sizeof(SpriteData));
            return &s_pool[i];
        }
    }
    return 0;
}

static void sprite_data_free(SpriteData *d)
{
    int i;
    for (i = 0; i < SPRITE_POOL_SIZE; i++)
    {
        if (&s_pool[i] == d)
        {
            s_used[i] = 0;
            return;
        }
    }
}

/* ── draw() ──────────────────────────────────────────────────────────────── */

static void sprite_draw_cb(Node *self)
{
    SpriteData *d = (SpriteData *)self->data;
    Node *p;
    int wx = 0, wy = 0, wrot = 0;
    int cam_ox = 0, cam_oy = 0;
    int sx, sy;
    POLY_FT4 *poly;

    /* Find nearest Node2D ancestor for world position */
    p = self->parent;
    while (p)
    {
        if (p->type == NODE_TYPE_NODE2D)
        {
            Node2DData *nd = (Node2DData *)p->data;
            wx = nd->wx;
            wy = nd->wy;
            wrot = nd->wrot;
            break;
        }
        p = p->parent;
    }

    /* Apply draw origin offset */
    wx += d->origin_x;
    wy += d->origin_y;

    /* Subtract active camera offset */
    if (d->tree && d->tree->active_camera)
        camera2d_get_offset(d->tree->active_camera, &cam_ox, &cam_oy);
    sx = wx - cam_ox;
    sy = wy - cam_oy;

    /* Cull: skip if fully off-screen */
    if (sx + d->w < 0 || sx >= ENGINE_SCREEN_W)
        return;
    if (sy + d->h < 0 || sy >= ENGINE_SCREEN_H)
        return;

    /* Emit POLY_FT4 */
    poly = (POLY_FT4 *)engine_render_alloc_prim(sizeof(POLY_FT4));
    setPolyFT4(poly);
    setRGB0(poly, 128, 128, 128); /* neutral grey — CLUT tints the colour */

    if (d->use_parent_rot && wrot != 0)
    {
        /* Rotated quad using GTE rcos/rsin (4096 units = full circle) */
        int c = rcos(wrot);
        int s = rsin(wrot);
        int hw = d->w / 2;
        int hh = d->h / 2;
        int cx = sx + hw;
        int cy = sy + hh;
        short vx[4], vy[4];
        int i;
        static const int lx[4] = {-1, +1, -1, +1};
        static const int ly[4] = {-1, -1, +1, +1};
        for (i = 0; i < 4; i++)
        {
            int ex = lx[i] * hw;
            int ey = ly[i] * hh;
            vx[i] = (short)(cx + ((ex * c - ey * s) >> 12));
            vy[i] = (short)(cy + ((ex * s + ey * c) >> 12));
        }
        setXY4(poly, vx[0], vy[0], vx[1], vy[1], vx[2], vy[2], vx[3], vy[3]);
    }
    else if (d->flip_x)
    {
        setXY4(poly,
               sx + d->w, sy,
               sx, sy,
               sx + d->w, sy + d->h,
               sx, sy + d->h);
    }
    else
    {
        setXY4(poly,
               sx, sy,
               sx + d->w, sy,
               sx, sy + d->h,
               sx + d->w, sy + d->h);
    }

    setUV4(poly,
           (unsigned char)d->u, (unsigned char)d->v,
           (unsigned char)(d->u + d->w - 1), (unsigned char)d->v,
           (unsigned char)d->u, (unsigned char)(d->v + d->h - 1),
           (unsigned char)(d->u + d->w - 1), (unsigned char)(d->v + d->h - 1));

    poly->tpage = d->tpage;
    poly->clut = d->use_theme_clut ? engine_renderer_active_clut() : d->clut;

    addPrim(engine_render_ot(d->z_index), poly);
}

/* ── Callbacks ────────────────────────────────────────────────────────────── */

static void sprite_destroy_cb(Node *self)
{
    if (self->data)
        sprite_data_free((SpriteData *)self->data);
    self->data = 0;
}

/* ── Factory ──────────────────────────────────────────────────────────────── */

Node *sprite_create(SceneTree *tree, const char *name)
{
    SpriteData *d;
    Node *n = node_alloc(NODE_TYPE_SPRITE, name);
    if (!n)
        return 0;
    d = sprite_data_alloc();
    if (!d)
    {
        node_free(n);
        return 0;
    }
    d->tree = tree;
    d->z_index = ENGINE_OT_OBJ;
    d->use_theme_clut = 0;
    n->data = d;
    n->init = 0;
    n->update = 0;
    n->draw = sprite_draw_cb;
    n->destroy = sprite_destroy_cb;
    return n;
}
