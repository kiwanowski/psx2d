# Tutorial 8 — Physics & Collision

_Building on Tutorial 7: add `CollisionShape` nodes to the gems and collect them
with `physics_query_aabb`._

---

## CollisionShapeData

Every physics body is an AABB (axis-aligned bounding box) stored in a
`CollisionShapeData`:

| Field                | Type       | Description                                               |
| -------------------- | ---------- | --------------------------------------------------------- |
| `ox`, `oy`           | `int`      | Offset from the parent Node2D origin to the AABB top-left |
| `w`, `h`             | `int`      | AABB width and height in pixels                           |
| `layer_mask`         | `uint32_t` | Which layer(s) this shape is **on**                       |
| `scan_mask`          | `uint32_t` | Which layer(s) this shape **tests against**               |
| `sensor`             | `int`      | `1` = overlap-only; no solid response applied             |
| `world_x`, `world_y` | `int`      | Cached world AABB origin — updated every frame            |

`CollisionShape` nodes automatically call `physics_register(self)` on `init` and
`physics_unregister(self)` on `destroy`.

### Predefined layer masks

```c
PHYS_LAYER_WORLD    (1u << 0)  /* static solid geometry */
PHYS_LAYER_PLAYER   (1u << 1)  /* the player body */
PHYS_LAYER_HAZARD   (1u << 2)  /* lethal objects */
PHYS_LAYER_COLLECT  (1u << 3)  /* collectibles */
PHYS_LAYER_SENSOR   (1u << 4)  /* non-solid triggers */
```

For gem pickups, set `layer_mask = PHYS_LAYER_COLLECT` and `sensor = 1`:

```c
CollisionShapeData *cs = (CollisionShapeData *)gem_col->data;
cs->ox         = 2;                   /* 2 px inset from gem's top-left */
cs->oy         = 2;
cs->w          = GC_TILE_SIZE - 4;
cs->h          = GC_TILE_SIZE - 4;
cs->layer_mask = PHYS_LAYER_COLLECT;  /* this shape IS a collectible    */
cs->scan_mask  = 0;                   /* gems don't scan for anything   */
cs->sensor     = 1;                   /* no solid response              */
```

---

## The Physics Pipeline

The correct per-frame order is:

```
scene_update(g_scene, 1)
    └── tree_update(root, 1)      — node2d transforms refreshed,
                                    cs_update_cb refreshes world_x/world_y
                                    for all registered shapes
physics_update()                  — belt-and-suspenders refresh of ALL
                                    shape world positions (optional but safe)
physics_query_aabb(...)           — now safe to query
```

In practice, for simple games you can skip calling `physics_update()` from the
main loop and call it from inside the root's `update` callback just before your
queries — see the checkpoint below.

---

## physics_query_aabb

```c
int physics_query_aabb(int x, int y, int w, int h,
                       uint32_t scan_mask,
                       AABBHit *out, int max);
```

Returns the number of shapes whose world AABB overlaps `(x, y, w, h)` **and**
whose `layer_mask & scan_mask != 0`. Results are written into `out[]`.

```c
AABBHit hits[8];
int count = physics_query_aabb(
    player_wx, player_wy,       /* query AABB top-left  */
    GC_PLAYER_W, GC_PLAYER_H,   /* query AABB size      */
    PHYS_LAYER_COLLECT,          /* only find collectibles */
    hits, 8                      /* output buffer        */
);
```

Each `AABBHit` contains:

- `hit.node` — the `CollisionShape` node
- `hit.shape` — its `CollisionShapeData *`

---

## Collecting Gems

The root's `update` callback queries the physics system after refreshing the
player's world transform. For each hit, the gem's Node2D parent is freed (which
also frees the `CollisionShape` child and calls `physics_unregister`):

```c
static void gc_update(Node *self, int dt)
{
    AABBHit hits[8];
    int count, i;

    /* ... move player, update world transform ... */

    /* Refresh all shape world positions before querying */
    physics_update();

    /* Query for gem overlaps at the player's current world position */
    count = physics_query_aabb(
        s_player_nd->wx, s_player_nd->wy,
        GC_PLAYER_W, GC_PLAYER_H,
        PHYS_LAYER_COLLECT,
        hits, 8
    );

    for (i = 0; i < count; i++) {
        /* hits[i].node = the CollisionShape node
           its parent   = the gem's Node2D                */
        Node *gem_nd = hits[i].node->parent;
        if (gem_nd) {
            node_free(gem_nd);  /* frees Node2D + Sprite placeholder + shape */
            s_gems_collected++;
        }
    }
}
```

> `node_free` detaches the gem Node2D from the root's `children[]` and marks all
> descendants inactive. Since `root->update` fires **before** the children
> traversal loop, the freed gem is never visited in the same frame.

---

## Signals for Physics Events

For more decoupled designs, you can emit `SIGNAL_BODY_ENTERED` from inside the
loop above and let a listener react:

```c
for (i = 0; i < count; i++) {
    signal_emit(SIGNAL_BODY_ENTERED, hits[i].node, NULL);
}
```

Connect a handler in the factory:

```c
signal_connect(SIGNAL_BODY_ENTERED, gem_col, root, on_gem_hit);
```

This is useful when the same gem type is used across many scenes and you want
the scene root — not the gem itself — to decide what happens on collection.

---

## API Reference

| Function                                      | Description                                                    |
| --------------------------------------------- | -------------------------------------------------------------- |
| `collision_shape_create(name)`                | Allocate a CollisionShape + data; auto-registers with physics. |
| `physics_update()`                            | Refresh all registered shape world positions.                  |
| `physics_query_aabb(x,y,w,h, mask, out, max)` | Find overlapping shapes. Returns hit count.                    |
| `physics_check_pair(a, b)`                    | `1` if two shape nodes' world AABBs overlap.                   |

---

## Checkpoint — Gem Collector: Physics

Changes from Tutorial 7:

- Each gem Node2D gets a sensor `CollisionShape` child
- `gc_update` calls `physics_update()` then `physics_query_aabb`
- Gems are freed on contact; `s_gems_collected` increments
- A `s_pending_win` flag triggers a win message after all gems collected

```c
// gc_t8_main.c  — adds CollisionShape + physics to the Tutorial 7 checkpoint
#include <psxpad.h>
#include <psxgpu.h>
#include <psxetc.h>
#include "engine/engine.h"
#include "gc_map.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */
#define GC_GROUND_Y (9 * GC_TILE_SIZE) /* y=144 — top of ground  */
#define GC_PLAYER_W 16
#define GC_PLAYER_H 16
#define GC_PLAYER_START_X 32
#define GC_PLAYER_START_Y (GC_GROUND_Y - GC_PLAYER_H) /* 128 */
#define GC_PLAYER_SPEED INT_TO_FIXED(2)
#define GC_GEM_COUNT 2
#define GC_GEM_W 16
#define GC_GEM_H 16

/* Gem world positions (evenly spaced, sitting on ground) */
static const int s_gem_world_x[GC_GEM_COUNT] = {96, 208};
static const int s_gem_world_y = GC_GROUND_Y - GC_TILE_SIZE; /* 128 */

/* ── Globals ────────────────────────────────────────────────────────────────── */
static char s_pad_buf[2][34];
static SceneTree s_scene;
SceneTree *g_scene = &s_scene;
unsigned short g_pad_btns = 0xFFFF;
unsigned short g_pad_prev = 0xFFFF;
#define BTN_HELD(b) (!(g_pad_btns & (b)))

static TextureResource s_tileset; /* loaded in main() via texture_resource_from_tim */

/* ── Scene state ────────────────────────────────────────────────────────────── */
static Node2DData *s_player_nd;
static Node *s_player_node;
static Camera2DData *s_cam_data;
static int s_gems_collected = 0;
static int s_pending_win = 0;

/* ── Scene callbacks ────────────────────────────────────────────────────────── */
static void gc_update(Node *self, int dt)
{
    AABBHit hits[8];
    int count, i, scroll;
    (void)self;
    (void)dt;

    /* Move player */
    if (BTN_HELD(PAD_RIGHT))
        s_player_nd->x += GC_PLAYER_SPEED;
    if (BTN_HELD(PAD_LEFT))
        s_player_nd->x -= GC_PLAYER_SPEED;
    if (s_player_nd->x < 0)
        s_player_nd->x = 0;
    if (s_player_nd->x > INT_TO_FIXED(GC_MAP_PX_W - GC_PLAYER_W))
        s_player_nd->x = INT_TO_FIXED(GC_MAP_PX_W - GC_PLAYER_W);

    /* Refresh player world transform so wx is current */
    node2d_update_world_transform(s_player_node);

    /* Refresh ALL shape world positions before querying */
    physics_update();

    /* Detect gem overlaps */
    count = physics_query_aabb(
        s_player_nd->wx, s_player_nd->wy,
        GC_PLAYER_W, GC_PLAYER_H,
        PHYS_LAYER_COLLECT,
        hits, 8);
    for (i = 0; i < count; i++)
    {
        Node *gem_nd = hits[i].node->parent;
        if (gem_nd)
        {
            node_free(gem_nd);
            s_gems_collected++;
        }
    }

    if (s_gems_collected >= GC_GEM_COUNT)
        s_pending_win = 1;

    /* Camera follow */
    scroll = s_player_nd->wx - ENGINE_SCREEN_W / 2;
    if (scroll < 0)
        scroll = 0;
    if (scroll > GC_MAP_PX_W - ENGINE_SCREEN_W)
        scroll = GC_MAP_PX_W - ENGINE_SCREEN_W;
    s_cam_data->ox = scroll;
}

static void gc_draw(Node *self)
{
    int i;
    (void)self;

    /* Gem placeholders — only draw if still in the tree (active) */
    /* (Freed gems are no longer children of root and won't be drawn here) */
    if (!s_pending_win)
        FntPrint(-1, " Gems: %d/%d", s_gems_collected, GC_GEM_COUNT);
    else
        FntPrint(-1, " You collected all gems!");
}

/* ── Gem factory helper ─────────────────────────────────────────────────────── */
static Node *make_gem(int world_x, int world_y)
{
    /* Node2D for the gem's world position */
    Node *gem_nd = node2d_create("Gem");
    Node2DData *gnd = (Node2DData *)gem_nd->data;
    gnd->x = INT_TO_FIXED(world_x);
    gnd->y = INT_TO_FIXED(world_y);

    /* Sensor CollisionShape — marks this node as a collectible */
    Node *gem_col = collision_shape_create("GemCol");
    CollisionShapeData *cs = (CollisionShapeData *)gem_col->data;
    cs->ox = 2;
    cs->oy = 2;
    cs->w = GC_TILE_SIZE - 4;
    cs->h = GC_TILE_SIZE - 4;
    cs->layer_mask = PHYS_LAYER_COLLECT;
    cs->scan_mask = 0;
    cs->sensor = 1;
    node_add_child(gem_nd, gem_col);

    // Sprite for the gem's visual representation — child of the Node2D so it moves with the world position
    Node *gem_spr = sprite_create(g_scene, "GemSpr");
    SpriteData *sd = (SpriteData *)gem_spr->data;
    sd->tpage = s_tileset.tpage;
    sd->clut  = s_tileset.clut_word;
    sd->use_theme_clut = 0;
    sd->u = 32;
    sd->v = 32;
    sd->w = GC_GEM_W;
    sd->h = GC_GEM_H;
    sd->z_index = ENGINE_OT_OBJ;
    sd->use_parent_rot = 0;
    node_add_child(gem_nd, gem_spr);

    return gem_nd;
}

/* ── Factory ────────────────────────────────────────────────────────────────── */
static Node *gc_factory(SceneTree *tree)
{
    int i;
    Node *root = node_alloc(NODE_TYPE_BASE, "GCRoot");
    root->update = gc_update;
    root->draw   = gc_draw;

    /* TileMap (same as Tutorial 7) */
    Node *map_nd = node2d_create("MapOrigin");
    Node *map    = tilemap_create(tree, "Map");
    TileMapData *tm = (TileMapData *)map->data;
    tm->tiles = s_map_data; tm->map_w = GC_MAP_W; tm->map_h = GC_MAP_H;
    tm->tile_size = GC_TILE_SIZE; tm->tpage = s_tpage;
    tm->use_theme_clut = 1; tm->atlas_u = 0; tm->atlas_v = 0;
    tm->tiles_per_row = 0; tm->z_index = ENGINE_OT_TILES;
    node_add_child(map_nd, map);
    node_add_child(root, map_nd);

    /* Player Node2D + Sprite (same as Tutorial 7) */
    Node *player  = node2d_create("Player");
    s_player_node = player;
    s_player_nd   = (Node2DData *)player->data;
    s_player_nd->x = INT_TO_FIXED(GC_PLAYER_START_X);
    s_player_nd->y = INT_TO_FIXED(GC_PLAYER_START_Y);
    Node *spr  = sprite_create(tree, "PlayerSpr");
    SpriteData *sd = (SpriteData *)spr->data;
    sd->tpage = s_tpage; sd->use_theme_clut = 1;
    sd->u = 0; sd->v = 0; sd->w = GC_PLAYER_W; sd->h = GC_PLAYER_H;
    sd->z_index = ENGINE_OT_PLAYER;
    node_add_child(player, spr);
    node_add_child(root, player);

    /* Gems — Node2D + sensor CollisionShape */
    s_gems_collected = 0;
    s_pending_win    = 0;
    for (i = 0; i < GC_GEM_COUNT; i++) {
        Node *gem = make_gem(s_gem_world_x[i], s_gem_world_y);
        node_add_child(root, gem);
    }

    /* Camera */
    Node *cam  = camera2d_create(tree, "Camera");
    s_cam_data = (Camera2DData *)cam->data;
    node_add_child(root, cam);

    return root;
}

extern const uint32_t _binary_SPRITES_TIM[];

int main(void)
{
    engine_init();
    texture_resource_from_tim(_binary_TILESET_TIM, &s_tileset);

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

        /* Check win condition AFTER scene_update, BEFORE render */
        if (s_pending_win) {
            FntFlush(-1);  /* flush font before scene_change destroys state */
            /* scene_change would go here — implemented in Tutorial 9 */
        }

        engine_renderer_begin_frame();
        scene_draw(g_scene);
        engine_renderer_end_frame();
    }
    return 0;
}
```

---

## What's Next

[Tutorial 9 — Audio & Putting It All Together](09-audio-complete.md)

Add background music via `AudioPlayer`, wire up a title screen and win screen
via `scene_change`, and complete the Gem Collector game.
