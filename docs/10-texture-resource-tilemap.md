# Tutorial 10 — TextureResource & CD-ROM Tilemap

_A standalone tutorial: load a tileset TIM from CD-ROM at runtime using
`TextureResource`, then feed it into a `TileMap` node._

This covers the case where your tileset is **too large to embed** in the
executable, or where different scenes use different tilesets that you want to
swap without recompiling.

---

## What We Are Building

A single scene that:

1. Reads `TILES.TIM;1` from the CD-ROM at scene-enter time
2. Creates a `TileMap` node pointing at that texture
3. Scrolls the map with a `Camera2D` using the D-pad

The finished scene tree looks like:

```
Root (BASE)
 ├── MapOrigin (Node2D)   — sets world origin of the map
 │    └── Map (TileMap)   — draws visible tiles each frame
 └── Camera (Camera2D)   — provides scroll offset
```

---

## 1. Preparing the Tileset TIM

The TIM must be added to the CD image so the engine can find it at
`\TILES.TIM;1`.

### Pixel format

Use **8bpp** (256-colour) if you need more than 16 tile colours, or **4bpp**
(16-colour) to save VRAM. 4bpp is usually enough for tilesets and matches the
spritesheet format already in the project.

### Tile atlas layout

Pack all tile frames in a horizontal strip (or a 2D grid for
`tiles_per_row > 0`). For example, a 4×1 strip of 16×16 tiles:

```
 U=0     U=16    U=32    U=48
┌───────┬───────┬───────┬───────┐
│ empty │ grass │ stone │ water │  ← all at V=0
└───────┴───────┴───────┴───────┘
```

Tile index `0` is always transparent — the TileMap skips it. So `empty` at index
0 is free; your real tiles start at index 1.

### Adding to the CD image

In `iso.xml` (or your equivalent CD layout file), add:

```xml
<file name="TILES.TIM" source="assets/tiles.tim" />
```

Then convert your PNG with `img2tim` from PSn00bSDK:

```
img2tim -bpp 4 -org 640 0 -plt 480 0 -o assets/tiles.tim assets/tiles.png
```

The `-org 640 0` places pixels at VRAM x=640, y=0 (past the two 320-wide
framebuffers). `-plt 480 0` puts the CLUT at y=480 row (past framebuffers, same
row the theme CLUTs use — pick a free x column).

---

## 2. Enabling CD-ROM

`engine_init()` does not call `CdInit()`. Add it in `main()` before the scene
loop — once per program run, not once per scene:

```c
/* game_main.c */
#include <psxcd.h>

int main(void)
{
    engine_init();
    CdInit();   /* required before texture_resource_load */
    ...
}
```

---

## 3. The TextureResource

`TextureResource` is the engine wrapper around a VRAM-uploaded TIM. It holds
both the `tpage` word and the `clut_word`, which is essential for 4bpp/8bpp
textures that carry their own palette (separate from the theme CLUTs).

```c
#include "engine/resource/texture.h"

static TextureResource s_tiles_tex;
```

Load it inside the scene's `init` callback (not in the factory — the CD read
happens on the first frame after the scene goes live):

```c
static void map_scene_init(Node *self)
{
    (void)self;

    if (!texture_resource_load("\\TILES.TIM;1", &s_tiles_tex))
    {
        /* CD read failed — handle gracefully */
        s_tiles_tex.valid = 0;
        return;
    }

    /* s_tiles_tex.tpage    — tpage word for TileMapData.tpage  */
    /* s_tiles_tex.clut_word — CLUT word for TileMapData.clut   */
}
```

`texture_resource_load` uses a static 64 KB sector buffer internally. Do **not**
call it from a CD interrupt context or from two threads simultaneously.

### Resetting the pool on scene change

Each `TextureResource` occupies one slot in an 8-slot pool. Call
`texture_resource_pool_init()` when tearing down a scene so the slots are
reclaimed for the next scene:

```c
static void map_scene_destroy(Node *self)
{
    (void)self;
    texture_resource_pool_init();
}
```

The engine's `scene_change` already calls `tree_destroy` on the old root, so
wiring `texture_resource_pool_init` into the root's `destroy` callback is the
natural place.

---

## 4. The Map Data

Define the tile grid as a static `uint8_t` array. Index `0` = empty (skipped by
the TileMap draw). The array is row-major: `tiles[row * MAP_W + col]`.

```c
#define MAP_W 40   /* tiles across */
#define MAP_H 15   /* tiles down   */
#define TILE_SIZE 16

static const uint8_t s_map[MAP_H * MAP_W] = {
    /* row 0 — sky */
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
    /* ... */

    /* row 13 — ground surface */
    1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,

    /* row 14 — subsurface */
    2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,
};
```

The map data is **not owned** by the TileMap — it only stores a pointer. You can
swap `tm->tiles` at runtime to change the map without touching VRAM.

---

## 5. Building the Scene

```c
#include "engine/core/node.h"
#include "engine/core/scene.h"
#include "engine/nodes/node2d.h"
#include "engine/nodes/tilemap.h"
#include "engine/nodes/camera2d.h"
#include "engine/resource/texture.h"
#include "engine/render/ot.h"

static TextureResource s_tiles_tex;
static Camera2DData   *s_cam;

/* ── init ─────────────────────────────────────────────────────────────────── */

static void map_scene_init(Node *self)
{
    (void)self;
    texture_resource_load("\\TILES.TIM;1", &s_tiles_tex);
}

/* ── update ───────────────────────────────────────────────────────────────── */

static void map_scene_update(Node *self, int dt)
{
    (void)self; (void)dt;

    /* Scroll with D-pad (g_pad_btns bits are active-low) */
    if (!(g_pad_btns & PAD_RIGHT)) s_cam->ox += 2;
    if (!(g_pad_btns & PAD_LEFT))  s_cam->ox -= 2;
    if (s_cam->ox < 0) s_cam->ox = 0;
}

/* ── destroy ──────────────────────────────────────────────────────────────── */

static void map_scene_destroy(Node *self)
{
    (void)self;
    texture_resource_pool_init();   /* reclaim the VRAM pool slot */
}

/* ── factory ──────────────────────────────────────────────────────────────── */

Node *map_scene_factory(SceneTree *tree)
{
    Node *root = node_alloc(NODE_TYPE_BASE, "MapScene");
    root->init    = map_scene_init;
    root->update  = map_scene_update;
    root->destroy = map_scene_destroy;

    /* Map origin — world (0, 0) */
    Node *origin = node2d_create("MapOrigin");
    /* origin stays at 0,0 — no adjustment needed */

    /* TileMap */
    Node *map = tilemap_create(tree, "Map");
    TileMapData *tm = (TileMapData *)map->data;

    tm->tiles         = s_map;
    tm->map_w         = MAP_W;
    tm->map_h         = MAP_H;
    tm->tile_size     = TILE_SIZE;

    /* These are filled from s_tiles_tex in init(), but the factory runs
       before init(). Set them here with zeroes — the TileMap draw callback
       reads tm->tpage/clut each frame, so they will be correct from frame 2
       onward. For frame 1 nothing visible is drawn. */
    tm->tpage         = 0;
    tm->clut          = 0;
    tm->use_theme_clut = 0;   /* tileset has its own CLUT, not the theme's */

    tm->atlas_u       = 0;    /* tile strip starts at U=0 in the TIM       */
    tm->atlas_v       = 0;
    tm->tiles_per_row = 0;    /* horizontal strip: tile N → column N        */
    tm->z_index       = ENGINE_OT_TILES;

    node_add_child(origin, map);
    node_add_child(root, origin);

    /* Camera */
    Node *cam = camera2d_create(tree, "Camera");
    s_cam = (Camera2DData *)cam->data;
    node_add_child(root, cam);

    return root;
}
```

Because `init` runs before the first `draw`, we must write `tpage`/`clut` from
`s_tiles_tex` into the `TileMapData` after the load completes. Update `init`:

```c
static void map_scene_init(Node *self)
{
    (void)self;

    if (!texture_resource_load("\\TILES.TIM;1", &s_tiles_tex))
        return;

    /* Patch the TileMap that the factory already wired up */
    TileMapData *tm = (TileMapData *)s_map_node->data;
    tm->tpage = s_tiles_tex.tpage;
    tm->clut  = s_tiles_tex.clut_word;
}
```

Add `static Node *s_map_node;` to the file and set it in the factory:

```c
s_map_node = map;
```

---

## 6. `tiles_per_row` for 2D Atlases

When the tileset is arranged in a grid instead of a single row, set
`tiles_per_row` to the number of columns in the atlas. The TileMap then
computes:

```
src_col = (tile_idx - 1) % tiles_per_row
src_row = (tile_idx - 1) / tiles_per_row

u = atlas_u + src_col * tile_size
v = atlas_v + src_row * tile_size
```

Example: 8 tiles arranged in a 4×2 grid:

```
 U=0   U=16  U=32  U=48
┌─────┬─────┬─────┬─────┐ V=0
│  1  │  2  │  3  │  4  │
├─────┼─────┼─────┼─────┤ V=16
│  5  │  6  │  7  │  8  │
└─────┴─────┴─────┴─────┘
```

```c
tm->tiles_per_row = 4;   /* 4 tiles per atlas row */
```

---

## 7. `use_theme_clut` vs own CLUT

| Situation                                                          | Setting                                                  |
| ------------------------------------------------------------------ | -------------------------------------------------------- |
| Tileset shares the theme palette (e.g. same 4-bit CLUT as sprites) | `use_theme_clut = 1`, ignore `clut`                      |
| Tileset has its own embedded CLUT in the TIM                       | `use_theme_clut = 0`, set `clut = s_tiles_tex.clut_word` |
| Tileset is 16bpp (no CLUT)                                         | `use_theme_clut = 0`, `clut = 0`                         |

For a CD-loaded tileset that ships its own palette, always use
`use_theme_clut = 0` and copy `s_tiles_tex.clut_word` into `tm->clut`.

---

## API Reference

| Function                              | Description                                                        |
| ------------------------------------- | ------------------------------------------------------------------ |
| `texture_resource_load(cdpath, out)`  | Read a TIM from CD-ROM, upload to VRAM. Returns 1 on success.      |
| `texture_resource_from_tim(ptr, out)` | Upload a TIM already in RAM (e.g. `incbin`). Returns 1 on success. |
| `texture_resource_pool_init()`        | Reset the 8-slot pool. Call in the scene `destroy` callback.       |
| `tilemap_create(tree, name)`          | Allocate a TileMap node + `TileMapData`.                           |
| `tilemap_pool_init()`                 | Reset TileMap sub-pool. Called by `scene_change` automatically.    |
| `CdInit()`                            | Initialise the CD-ROM drive. Call once in `main()`.                |
