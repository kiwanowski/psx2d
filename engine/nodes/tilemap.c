#include "tilemap.h"
#include "camera2d.h"
#include "../render/renderer.h"
#include <psxgpu.h>
#include <string.h>

/* ── Sub-pool ─────────────────────────────────────────────────────────────── */

static TileMapData s_pool[TILEMAP_POOL_SIZE];
static int s_used[TILEMAP_POOL_SIZE];

void tilemap_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
}

static TileMapData *tm_data_alloc(void)
{
    int i;
    for (i = 0; i < TILEMAP_POOL_SIZE; i++)
    {
        if (!s_used[i])
        {
            s_used[i] = 1;
            memset(&s_pool[i], 0, sizeof(TileMapData));
            return &s_pool[i];
        }
    }
    return 0;
}

static void tm_data_free(TileMapData *d)
{
    int i;
    for (i = 0; i < TILEMAP_POOL_SIZE; i++)
    {
        if (&s_pool[i] == d)
        {
            s_used[i] = 0;
            return;
        }
    }
}

/* ── draw() ──────────────────────────────────────────────────────────────── */

static void tilemap_draw_cb(Node *self)
{
    TileMapData *d = (TileMapData *)self->data;
    int cam_ox = 0, cam_oy = 0;
    int map_ox = 0, map_oy = 0; /* tilemap world origin */
    int first_col, last_col;
    int col, row;
    uint16_t active_clut;
    Node *p;

    if (!d->tiles || d->tile_size <= 0)
        return;

    /* Tilemap world origin from nearest Node2D ancestor */
    p = self->parent;
    while (p)
    {
        if (p->type == NODE_TYPE_NODE2D)
        {
            node2d_get_world(p, &map_ox, &map_oy);
            break;
        }
        p = p->parent;
    }

    /* Camera scroll offset */
    if (d->tree && d->tree->active_camera)
        camera2d_get_offset(d->tree->active_camera, &cam_ox, &cam_oy);

    active_clut = d->use_theme_clut ? engine_renderer_active_clut() : d->clut;

    /* Visible column range (add 1 tile margin each side) */
    first_col = (cam_ox - map_ox) / d->tile_size - 1;
    last_col = first_col + (ENGINE_SCREEN_W / d->tile_size) + 2;
    if (first_col < 0)
        first_col = 0;
    if (last_col >= d->map_w)
        last_col = d->map_w - 1;

    for (row = 0; row < d->map_h; row++)
    {
        int sy = map_oy + row * d->tile_size - cam_oy;
        /* Vertical cull */
        if (sy + d->tile_size < 0 || sy >= ENGINE_SCREEN_H)
            continue;

        for (col = first_col; col <= last_col; col++)
        {
            uint8_t tile_idx;
            int src_col, src_row;
            int u, v;
            int sx;
            POLY_FT4 *poly;

            tile_idx = d->tiles[row * d->map_w + col];
            if (tile_idx == 0)
                continue; /* tile 0 = transparent/empty */

            /* Atlas UV */
            src_col = (d->tiles_per_row > 0)
                          ? (tile_idx % d->tiles_per_row)
                          : tile_idx;
            src_row = (d->tiles_per_row > 0)
                          ? (tile_idx / d->tiles_per_row)
                          : 0;
            u = d->atlas_u + src_col * d->tile_size;
            v = d->atlas_v + src_row * d->tile_size;

            sx = map_ox + col * d->tile_size - cam_ox;

            poly = (POLY_FT4 *)engine_render_alloc_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, 128, 128, 128);
            setXY4(poly,
                   sx, sy,
                   sx + d->tile_size, sy,
                   sx, sy + d->tile_size,
                   sx + d->tile_size, sy + d->tile_size);
            setUV4(poly,
                   (unsigned char)u, (unsigned char)v,
                   (unsigned char)(u + d->tile_size - 1), (unsigned char)v,
                   (unsigned char)u, (unsigned char)(v + d->tile_size - 1),
                   (unsigned char)(u + d->tile_size - 1), (unsigned char)(v + d->tile_size - 1));
            poly->tpage = d->tpage;
            poly->clut = active_clut;
            addPrim(engine_render_ot(d->z_index), poly);
        }
    }
}

/* ── Callbacks ────────────────────────────────────────────────────────────── */

static void tilemap_destroy_cb(Node *self)
{
    if (self->data)
        tm_data_free((TileMapData *)self->data);
    self->data = 0;
}

/* ── Factory ──────────────────────────────────────────────────────────────── */

Node *tilemap_create(SceneTree *tree, const char *name)
{
    TileMapData *d;
    Node *n = node_alloc(NODE_TYPE_TILEMAP, name);
    if (!n)
        return 0;
    d = tm_data_alloc();
    if (!d)
    {
        node_free(n);
        return 0;
    }
    d->tree = tree;
    d->z_index = ENGINE_OT_TILES;
    d->use_theme_clut = 0;
    d->tiles_per_row = 0; /* default: single row of tiles in atlas */
    d->tile_size = 16;
    n->data = d;
    n->init = 0;
    n->update = 0;
    n->draw = tilemap_draw_cb;
    n->destroy = tilemap_destroy_cb;
    return n;
}
