#ifndef ENGINE_TILEMAP_H
#define ENGINE_TILEMAP_H

#include "../core/node.h"
#include "../core/scene.h"
#include "node2d.h"
#include <stdint.h>

#define TILEMAP_POOL_SIZE 4

/*
 * TileMapData — payload for NODE_TYPE_TILEMAP nodes.
 *
 * The tile array is owned externally (e.g. a static level array) — TileMap
 * only stores a pointer, so scene transitions don't need to copy map data.
 *
 * UV layout assumption: the tile atlas is a horizontal strip where tile N
 * starts at (atlas_u + N * tile_size, atlas_v).  For multi-row atlases,
 * set tiles_per_row to the number of tiles per atlas row.
 *
 * Draw order: TileMap emits one POLY_FT4 per visible tile into ENGINE_OT_TILES
 * (z_index default) each frame.  Only columns visible within
 * [cam_ox, cam_ox + SCREEN_W] are emitted — off-screen tiles are culled.
 */
typedef struct
{
    SceneTree *tree; /* for active_camera lookup */

    /* Map data (external, not owned) */
    const uint8_t *tiles; /* row-major: tiles[row * map_w + col] */
    int map_w;            /* map width  in tiles */
    int map_h;            /* map height in tiles */
    int tile_size;        /* tile size length in pixels (e.g. 16) */

    /* Atlas handles */
    uint16_t tpage;
    uint16_t clut;
    int use_theme_clut;
    int atlas_u;       /* texel X of tile-0 top-left in atlas */
    int atlas_v;       /* texel Y of tile-0 top-left in atlas */
    int tiles_per_row; /* number of tiles per atlas row (for 2D atlases) */

    /* Draw order */
    int z_index;
} TileMapData;

/*
 * tilemap_create — allocate and wire up a TileMap node.
 */
Node *tilemap_create(SceneTree *tree, const char *name);

/* Sub-pool reset */
void tilemap_pool_init(void);

#endif /* ENGINE_TILEMAP_H */
