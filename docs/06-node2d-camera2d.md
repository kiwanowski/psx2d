# Tutorial 6 — 2D Space: Node2D & Camera2D

_This tutorial starts the **Gem Collector** project — a minimal side-scroller
built across tutorials 6–9 to demonstrate every engine subsystem._

---

## Node2D

A `Node2DData` stores a node's position and rotation in two coordinate systems:

| Field      | Type      | Meaning                                                    |
| ---------- | --------- | ---------------------------------------------------------- |
| `x`, `y`   | `fixed_t` | **Local** position relative to the nearest Node2D ancestor |
| `rot`      | `int`     | **Local** rotation in GTE units (0–4095 = full circle)     |
| `wx`, `wy` | `int`     | **World** (screen-space) position — updated every frame    |
| `wrot`     | `int`     | **World** accumulated rotation                             |

`wx` and `wy` are recomputed during `update()` by walking up the parent chain
and adding each ancestor Node2D's world position. If there is no Node2D
ancestor, the world position equals the local position.

```c
Node *player = node2d_create("Player");
Node2DData *nd = (Node2DData *)player->data;

nd->x = INT_TO_FIXED(64);   // local X = 64 px
nd->y = INT_TO_FIXED(128);  // local Y = 128 px
nd->rot = 0;                 // no rotation
```

After `tree_init` and every subsequent `tree_update`, `nd->wx` and `nd->wy` hold
the integer screen-space position.

### Modifying position

You can write to `nd->x` or `nd->y` from a parent node's `update` callback.
Because `tree_update` is **pre-order**, the parent fires before the Node2D
child, so the Node2D's own update callback picks up the freshly changed values:

```c
// In the parent BASE node's update callback:
nd->x += INT_TO_FIXED(2);   // move 2 px/frame to the right
// ↑ Node2D's update fires next and calls node2d_update_world_transform()
//   using the new nd->x, refreshing nd->wx.
```

If you need the world position **immediately** (e.g. to feed into a physics
query in the same frame), call `node2d_update_world_transform()` yourself after
changing `x`/`y`:

```c
nd->x += INT_TO_FIXED(2);
node2d_update_world_transform(player_node);  // nd->wx is now current
```

---

## Camera2D

A `Camera2DData` stores an **integer scroll offset**:

| Field  | Type          | Meaning                                |
| ------ | ------------- | -------------------------------------- |
| `tree` | `SceneTree *` | Back-pointer; set by `camera2d_create` |
| `ox`   | `int`         | Additional X offset                    |
| `oy`   | `int`         | Additional Y offset                    |

`camera2d_get_offset(cam, &ox, &oy)` returns:

```
ox  =  nearest Node2D ancestor wx  +  cam->ox
oy  =  nearest Node2D ancestor wy  +  cam->oy
```

When a `Camera2D` has **no Node2D ancestor** (it is a direct child of a BASE
root), the offset is purely `cam->ox, cam->oy`.

Every `Sprite` subtracts `(cam_ox, cam_oy)` from the Node2D world position to
get the screen position. The `TileMap` does the same for tiles.

### Camera follows player

```c
// After updating the player's world position this frame:
int scroll_x = player_nd->wx - 160;  // centre player at x=160
if (scroll_x < 0) scroll_x = 0;
cam_data->ox = scroll_x;
```

When `cam->ox == player_wx - 160`:

```
Sprite screen X  =  player_wx  −  cam_ox
                 =  player_wx  −  (player_wx − 160)
                 =  160                              ← centred ✓
```

---

## API Reference

| Function                              | Description                                            |
| ------------------------------------- | ------------------------------------------------------ |
| `node2d_create(name)`                 | Allocate a Node2D + data. Returns `NULL` if pool full. |
| `node2d_get_world(node, &wx, &wy)`    | Read the cached world-space position.                  |
| `node2d_update_world_transform(node)` | Recompute `wx/wy/wrot` from parent chain.              |
| `camera2d_create(tree, name)`         | Allocate a Camera2D, store `tree` backpointer.         |
| `camera2d_get_offset(cam, &ox, &oy)`  | Return current camera scroll offset.                   |

---

## Checkpoint — Gem Collector: Movement & Camera

The player is a white 16×16 square. Move with the D-pad. The camera scrolls
horizontally to keep the player centred, clamped at the level edges.

```c
// gc_t6_main.c
#include <psxpad.h>
#include <psxgpu.h>
#include "engine/engine.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */
#define GC_TILE_SIZE    16
#define GC_MAP_W        40
#define GC_MAP_H        12
#define GC_WORLD_W      (GC_MAP_W * GC_TILE_SIZE)   /* 640 px            */
#define GC_GROUND_Y     (9 * GC_TILE_SIZE)           /* y=144, top of ground */
#define GC_PLAYER_W     16
#define GC_PLAYER_H     16
#define GC_PLAYER_START_X  32
#define GC_PLAYER_START_Y  (GC_GROUND_Y - GC_PLAYER_H)  /* 128 */
#define GC_PLAYER_SPEED    INT_TO_FIXED(2)

/* ── Globals ────────────────────────────────────────────────────────────────── */
static char      s_pad_buf[2][34];
static SceneTree s_scene;
SceneTree       *g_scene    = &s_scene;
unsigned short   g_pad_btns = 0xFFFF;
unsigned short   g_pad_prev = 0xFFFF;
#define BTN_TAPPED(b) ((g_pad_prev & (b)) && !(g_pad_btns & (b)))
#define BTN_HELD(b)   (!(g_pad_btns & (b)))

/* ── Scene state ────────────────────────────────────────────────────────────── */
static Node2DData   *s_player_nd;
static Node         *s_player_node;
static Camera2DData *s_cam_data;

/* ── Scene callbacks ────────────────────────────────────────────────────────── */
static void gc_update(Node *self, int dt)
{
    int scroll;
    (void)self; (void)dt;

    /* 1. Move player */
    if (BTN_HELD(PAD_RIGHT)) s_player_nd->x += GC_PLAYER_SPEED;
    if (BTN_HELD(PAD_LEFT))  s_player_nd->x -= GC_PLAYER_SPEED;

    /* Clamp to world bounds */
    if (s_player_nd->x < 0)
        s_player_nd->x = 0;
    if (s_player_nd->x > INT_TO_FIXED(GC_WORLD_W - GC_PLAYER_W))
        s_player_nd->x = INT_TO_FIXED(GC_WORLD_W - GC_PLAYER_W);

    /* 2. Refresh world transform now so wx is current for camera */
    node2d_update_world_transform(s_player_node);

    /* 3. Camera follows player horizontally */
    scroll = s_player_nd->wx - (ENGINE_SCREEN_W / 2);
    if (scroll < 0)                          scroll = 0;
    if (scroll > GC_WORLD_W - ENGINE_SCREEN_W) scroll = GC_WORLD_W - ENGINE_SCREEN_W;
    s_cam_data->ox = scroll;
}

static void gc_draw(Node *self)
{
    int sx, sy;
    TILE *t;
    (void)self;

    /* Draw player as a solid white tile (no Sprite node yet) */
    sx = s_player_nd->wx - s_cam_data->ox;
    sy = s_player_nd->wy;   /* no vertical scroll */

    t = (TILE *)engine_render_alloc_prim(sizeof(TILE));
    setTile(t);
    setXY0(t, (short)sx, (short)sy);
    setWH(t, GC_PLAYER_W, GC_PLAYER_H);
    setRGB0(t, 255, 255, 255);
    addPrim(engine_render_ot(ENGINE_OT_PLAYER), t);
}

/* ── Factory ────────────────────────────────────────────────────────────────── */
static Node *gc_factory(SceneTree *tree)
{
    /* Root — holds movement + draw logic */
    Node *root  = node_alloc(NODE_TYPE_BASE, "GCRoot");
    root->update = gc_update;
    root->draw   = gc_draw;

    /* Player Node2D */
    Node *player     = node2d_create("Player");
    s_player_node    = player;
    s_player_nd      = (Node2DData *)player->data;
    s_player_nd->x   = INT_TO_FIXED(GC_PLAYER_START_X);
    s_player_nd->y   = INT_TO_FIXED(GC_PLAYER_START_Y);
    node_add_child(root, player);

    /* Camera2D — no Node2D parent, offset-driven */
    Node *cam   = camera2d_create(tree, "Camera");
    s_cam_data  = (Camera2DData *)cam->data;
    s_cam_data->ox = 0;
    s_cam_data->oy = 0;
    node_add_child(root, cam);

    return root;
}

/* ── main ───────────────────────────────────────────────────────────────────── */
int main(void)
{
    engine_init();

    InitPAD(s_pad_buf[0], 34, s_pad_buf[1], 34);
    StartPAD();
    ChangeClearPAD(0);

    scene_tree_init(g_scene);
    scene_change(g_scene, gc_factory);

    while (1) {
        PADTYPE *pad = (PADTYPE *)s_pad_buf[0];
        g_pad_prev = g_pad_btns;
        g_pad_btns = 0xFFFF;
        if (pad->stat == 0 &&
            (pad->type == 0x4 || pad->type == 0x5 || pad->type == 0x7))
            g_pad_btns = pad->btn;

        scene_update(g_scene, 1);
        engine_renderer_begin_frame();
        scene_draw(g_scene);
        engine_renderer_end_frame();
    }
    return 0;
}
```

<video width="320" height="240" controls>
  <source src="screenshots/06-camera2d.mp4" type="video/mp4">
</video>

---

## What's Next

[Tutorial 7 — Rendering: Sprites & TileMaps](07-sprites-tilemap.md)

Replace the placeholder TILE with a textured `Sprite` and add a scrolling
`TileMap` for the level background.
