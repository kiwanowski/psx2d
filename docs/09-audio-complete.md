# Tutorial 9 — Audio & Putting It All Together

_Building on Tutorial 8: add an `AudioPlayer` for background music and wire up a
title screen and win screen with `scene_change`, completing the Gem Collector._

---

## Audio Resources and Players

The engine separates **locating** audio from **playing** it:

- `AudioResource` — a located CD-ROM file position (`CdlLOC`). Load once.
- `AudioPlayerData` — a streaming node that holds a pointer to an
  `AudioResource`.

### CdInit and SpuInit

`engine_init()` does **not** call `CdInit()` or `SpuInit()`. Add them to
`main()` before any AudioPlayer nodes are created:

```c
#include <psxcd.h>
#include <psxspu.h>

int main(void) {
    engine_init();
    CdInit();    /* initialise CD-ROM drive */
    SpuInit();   /* initialise SPU (sound processing unit) */
    SpuSetCommonMasterVolume(0x3FFF, 0x3FFF);
    SpuSetCommonCDVolume(0x7FFF, 0x7FFF);
    ...
}
```

### Loading an audio resource

```c
#include "engine/resource/audio.h"

static AudioResource s_bgm;

/* In the play scene factory or init: */
audio_resource_load(
    "\\MUSIC.XA;1",   /* PS1 CD path  */
    1,                /* XA file index */
    0,                /* XA channel   */
    &s_bgm
);
/* s_bgm.valid == 1 on success */
```

### Creating an AudioPlayer node

```c
#include "engine/nodes/audio_player.h"

Node *bgm_node = audio_player_create("BGM");
AudioPlayerData *ap = (AudioPlayerData *)bgm_node->data;
ap->resource = &s_bgm;
ap->looping  = 1;    /* restart from the beginning when the XA stream ends */
node_add_child(root, bgm_node);
```

### Starting and stopping playback

```c
audio_player_play(bgm_node);   /* starts CdlReadS on the resource's CdlLOC */
audio_player_stop(bgm_node);   /* sends CdlPause                           */
```

Call `audio_player_play` after `tree_init` (i.e. after `scene_change` returns).
The simplest place is the scene root's `init` callback:

```c
static Node *s_bgm_node;

static void play_scene_init(Node *self) {
    (void)self;
    audio_player_play(s_bgm_node);
}

/* In factory: */
root->init = play_scene_init;
```

### AudioPlayerData fields

| Field      | Type              | Description                                    |
| ---------- | ----------------- | ---------------------------------------------- |
| `resource` | `AudioResource *` | Pointer to a loaded resource. Set before play. |
| `playing`  | `int`             | Read-only: `1` while streaming.                |
| `looping`  | `int`             | `1` = auto-restart on end-of-data sentinel.    |

---

## Texture Resources (alternative to incbin)

For assets loaded from disc at runtime (instead of embedded via `incbin`):

```c
#include "engine/resource/texture.h"

static TextureResource s_tex;

/* At scene start, outside of the CD interrupt: */
texture_resource_load("\\TILES.TIM;1", &s_tex);
/* s_tex.tpage and s_tex.clut_word are ready to use in Sprite/TileMap */
```

For embedded TIMs (the incbin approach used in earlier tutorials):

```c
texture_resource_from_tim(_binary_SPRITES_TIM, &s_tex);
```

---

## API Reference

| Function                                     | Description                                        |
| -------------------------------------------- | -------------------------------------------------- |
| `audio_resource_load(path, file, chan, out)` | Locate an XA file on disc. Returns `1` on success. |
| `audio_player_create(name)`                  | Allocate an AudioPlayer node + data.               |
| `audio_player_play(node)`                    | Start XA streaming.                                |
| `audio_player_stop(node)`                    | Pause XA streaming.                                |
| `texture_resource_from_tim(tim_data, out)`   | Upload embedded TIM to VRAM.                       |
| `texture_resource_load(cdpath, out)`         | Load TIM from disc and upload to VRAM.             |

---

## Complete Gem Collector

The final game has three scenes:

```
title_scene  — "Press X to start"
play_scene   — Tutorial 8 play scene + AudioPlayer
win_scene    — "You collected all gems! Press X to play again"
```

The complete listing is a single file. Sections are labelled so you can split
them into separate files in your own project.

```c
// gc_complete.c — complete Gem Collector, Tutorial 9
#include <psxpad.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <psxcd.h>
#include <psxspu.h>
#include "engine/engine.h"
#include "gc_map.h"

/* ════════════════════════════════════════════════════════════════════════════
 * Constants
 * ════════════════════════════════════════════════════════════════════════════ */
#define GC_GROUND_Y (9 * GC_TILE_SIZE) /* y=144 — top of ground  */
#define GC_PLAYER_W 16
#define GC_PLAYER_H 16
#define GC_PLAYER_START_X 32
#define GC_PLAYER_START_Y (GC_GROUND_Y - GC_PLAYER_H) /* 128 */
#define GC_PLAYER_SPEED INT_TO_FIXED(2)
#define GC_GEM_COUNT 2
#define GC_GEM_W 16
#define GC_GEM_H 16

static const int s_gem_world_x[GC_GEM_COUNT] = {96, 208};
static const int s_gem_world_y = GC_GROUND_Y - GC_TILE_SIZE; /* 128 */

/* ════════════════════════════════════════════════════════════════════════════
 * Globals
 * ════════════════════════════════════════════════════════════════════════════ */
static char      s_pad_buf[2][34];
static SceneTree s_scene;
SceneTree       *g_scene    = &s_scene;
unsigned short   g_pad_btns = 0xFFFF;
unsigned short   g_pad_prev = 0xFFFF;
#define BTN_TAPPED(b) ((g_pad_prev & (b)) && !(g_pad_btns & (b)))
#define BTN_HELD(b)   (!(g_pad_btns & (b)))

static TextureResource s_tileset;

/* ════════════════════════════════════════════════════════════════════════════
 * Forward declarations
 * ════════════════════════════════════════════════════════════════════════════ */
static Node *title_factory(SceneTree *tree);
static Node *play_factory(SceneTree *tree);
static Node *win_factory(SceneTree *tree);

/* ════════════════════════════════════════════════════════════════════════════
 * Title Scene
 * ════════════════════════════════════════════════════════════════════════════ */
static void title_update(Node *self, int dt)
{
    (void)self; (void)dt;
    if (BTN_TAPPED(PAD_CROSS))
        scene_change(g_scene, play_factory);
}
static void title_draw(Node *self)
{
    (void)self;
    FntPrint(-1, "\n\n\n  GEM COLLECTOR\n\n  Collect all %d gems!\n\n  [X] Start", GC_GEM_COUNT);
}
static Node *title_factory(SceneTree *tree)
{
    Node *r = node_alloc(NODE_TYPE_BASE, "TitleRoot");
    r->update = title_update;
    r->draw   = title_draw;
    (void)tree;
    return r;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Win Scene
 * ════════════════════════════════════════════════════════════════════════════ */
static void win_update(Node *self, int dt)
{
    (void)self; (void)dt;
    if (BTN_TAPPED(PAD_CROSS))
        scene_change(g_scene, title_factory);
}
static void win_draw(Node *self)
{
    (void)self;
    FntPrint(-1, "\n\n  You got all the gems!\n\n  [X] Play again");
}
static Node *win_factory(SceneTree *tree)
{
    Node *r = node_alloc(NODE_TYPE_BASE, "WinRoot");
    r->update = win_update;
    r->draw   = win_draw;
    (void)tree;
    return r;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Play Scene
 * ════════════════════════════════════════════════════════════════════════════ */
static Node2DData   *s_player_nd;
static Node         *s_player_node;
static Camera2DData *s_cam_data;
static int           s_gems_collected;
static int           s_pending_win;
static AudioResource s_bgm_res;
static Node         *s_bgm_node;

static void play_init(Node *self)
{
    (void)self;
    /* Start background music once the scene tree is live */
    if (s_bgm_res.valid)
        audio_player_play(s_bgm_node);
}

static void play_update(Node *self, int dt)
{
    AABBHit hits[8];
    int count, i, scroll;
    (void)self; (void)dt;

    /* Move player */
    if (BTN_HELD(PAD_RIGHT)) s_player_nd->x += GC_PLAYER_SPEED;
    if (BTN_HELD(PAD_LEFT))  s_player_nd->x -= GC_PLAYER_SPEED;
    if (s_player_nd->x < 0)
        s_player_nd->x = 0;
    if (s_player_nd->x > INT_TO_FIXED(GC_MAP_PX_W - GC_PLAYER_W))
        s_player_nd->x = INT_TO_FIXED(GC_MAP_PX_W - GC_PLAYER_W);

    /* Refresh world transform before physics query */
    node2d_update_world_transform(s_player_node);

    /* Update all CollisionShape world positions */
    physics_update();

    /* Collect gems */
    count = physics_query_aabb(
        s_player_nd->wx, s_player_nd->wy,
        GC_PLAYER_W, GC_PLAYER_H,
        PHYS_LAYER_COLLECT,
        hits, 8
    );
    for (i = 0; i < count; i++) {
        Node *gem_nd = hits[i].node->parent;
        if (gem_nd) {
            node_free(gem_nd);
            s_gems_collected++;
        }
    }

    if (s_gems_collected >= GC_GEM_COUNT)
        s_pending_win = 1;

    /* Camera follow */
    scroll = s_player_nd->wx - ENGINE_SCREEN_W / 2;
    if (scroll < 0)                            scroll = 0;
    if (scroll > GC_MAP_PX_W - ENGINE_SCREEN_W) scroll = GC_MAP_PX_W - ENGINE_SCREEN_W;
    s_cam_data->ox = scroll;
}

static void play_draw(Node *self)
{
    (void)self;
    FntPrint(-1, " Gems: %d/%d", s_gems_collected, GC_GEM_COUNT);

    /* Draw gem placeholders in yellow for remaining uncollected gems.
       Note: freed gems are no longer in the tree so we track by count. */
}

/* ── Gem helper ─────────────────────────────────────────────────────────────── */
static Node *make_gem(int world_x, int world_y)
{
    Node *gem_nd = node2d_create("Gem");
    Node2DData *gnd = (Node2DData *)gem_nd->data;
    gnd->x = INT_TO_FIXED(world_x);
    gnd->y = INT_TO_FIXED(world_y);

    /* Sensor CollisionShape */
    Node *gem_col = collision_shape_create("GemCol");
    CollisionShapeData *cs = (CollisionShapeData *)gem_col->data;
    cs->ox = 2;  cs->oy = 2;
    cs->w  = GC_TILE_SIZE - 4;
    cs->h  = GC_TILE_SIZE - 4;
    cs->layer_mask = PHYS_LAYER_COLLECT;
    cs->scan_mask  = 0;
    cs->sensor     = 1;
    node_add_child(gem_nd, gem_col);

    // Sprite for the gem's visual representation — child of the Node2D so it moves with the world position
    Node *gem_spr = sprite_create(g_scene, "GemSpr");
    SpriteData *sd = (SpriteData *)gem_spr->data;
    sd->tpage = s_tileset.tpage;
    sd->clut = s_tileset.clut_word;
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

/* ── Play factory ────────────────────────────────────────────────────────────── */
static Node *play_factory(SceneTree *tree)
{
    int i;
    Node *root = node_alloc(NODE_TYPE_BASE, "PlayRoot");
    root->init   = play_init;
    root->update = play_update;
    root->draw   = play_draw;

    /* Reset state */
    s_gems_collected = 0;
    s_pending_win    = 0;

    /* Load background audio (safe to call each time — finds file by directory scan) */
    audio_resource_load("\\MUSIC.XA;1", 1, 0, &s_bgm_res);

   /* TileMap — parented to a Node2D at (0, 0) for world-space origin */
    Node *map_nd = node2d_create("MapOrigin");
    Node *map = tilemap_create(tree, "Map");
    TileMapData *tm = (TileMapData *)map->data;
    tm->tiles = GC_TILE_LAYER_1_tiles;
    tm->map_w = GC_MAP_W;
    tm->map_h = GC_MAP_H;
    tm->tile_size = GC_TILE_SIZE;
    tm->tpage = s_tileset.tpage;
    tm->clut = s_tileset.clut_word;
    tm->use_theme_clut = 0;
    tm->atlas_u = 0; /* tile N → atlas x = 0 + N*16 */
    tm->atlas_v = 0;
    tm->tiles_per_row = GC_TILES_PER_ROW; /* horizontal strip */
    tm->z_index = ENGINE_OT_TILES;
    node_add_child(map_nd, map);
    node_add_child(root, map_nd);

    /* Player Node2D */
    Node *player = node2d_create("Player");
    s_player_node = player;
    s_player_nd = (Node2DData *)player->data;
    s_player_nd->x = INT_TO_FIXED(GC_PLAYER_START_X);
    s_player_nd->y = INT_TO_FIXED(GC_PLAYER_START_Y);

    /* Player Sprite — child of Player Node2D */
    Node *spr = sprite_create(tree, "PlayerSpr");
    SpriteData *sd = (SpriteData *)spr->data;
    sd->tpage = s_tileset.tpage;
    sd->use_theme_clut = 1;
    sd->u = 0; /* UV_PLAYER_X: player texture at atlas x=0 */
    sd->v = 0; /* UV_PLAYER_Y */
    sd->w = GC_PLAYER_W;
    sd->h = GC_PLAYER_H;
    sd->z_index = ENGINE_OT_PLAYER;
    sd->use_parent_rot = 0;
    node_add_child(player, spr);
    node_add_child(root, player);

    /* Gems — Node2D + sensor CollisionShape */
    s_gems_collected = 0;
    s_pending_win = 0;
    for (int i = 0; i < GC_GEM_COUNT; i++)
    {
        Node *gem = make_gem(s_gem_world_x[i], s_gem_world_y);
        node_add_child(root, gem);
    }

    /* AudioPlayer */
    s_bgm_node = audio_player_create("BGM");
    AudioPlayerData *ap = (AudioPlayerData *)s_bgm_node->data;
    ap->resource = &s_bgm_res;
    ap->looping  = 1;
    node_add_child(root, s_bgm_node);

    /* Camera */
    Node *cam  = camera2d_create(tree, "Camera");
    s_cam_data = (Camera2DData *)cam->data;
    node_add_child(root, cam);

    return root;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Incbin + main
 * ════════════════════════════════════════════════════════════════════════════ */
extern const uint32_t _binary_TILESET_TIM[];

int main(void)
{
    engine_init();
    texture_resource_from_tim(_binary_TILESET_TIM, &s_tileset);

    /* Audio subsystem — not initialised by engine_init() */
    CdInit();
    SpuInit();
    SpuSetCommonMasterVolume(0x3FFF, 0x3FFF);
    SpuSetCommonCDVolume(0x7FFF, 0x7FFF);

    InitPAD(s_pad_buf[0], 34, s_pad_buf[1], 34);
    StartPAD();
    ChangeClearPAD(0);

    scene_tree_init(g_scene);
    scene_change(g_scene, title_factory);

    while (1) {
        PADTYPE *pad = (PADTYPE *)s_pad_buf[0];
        g_pad_prev = g_pad_btns;
        g_pad_btns = 0xFFFF;
        if (pad->stat == 0 &&
            (pad->type == 0x4 || pad->type == 0x5 || pad->type == 0x7))
            g_pad_btns = pad->btn;

        scene_update(g_scene, 1);

        /* Deferred scene transition: never call scene_change from draw */
        if (s_pending_win) {
            s_pending_win = 0;
            audio_player_stop(s_bgm_node);
            scene_change(g_scene, win_factory);
        }

        engine_renderer_begin_frame();
        scene_draw(g_scene);
        engine_renderer_end_frame();
    }
    return 0;
}
```

---

## Key Patterns Demonstrated

| Pattern                                                 | Location        |
| ------------------------------------------------------- | --------------- |
| `CdInit` / `SpuInit` before any audio                   | `main()`        |
| `audio_player_play` in `init` callback                  | `play_init()`   |
| `audio_player_stop` before `scene_change`               | main loop       |
| `s_pending_win` flag — defer transition out of `update` | main loop       |
| `node_free` on gem from inside root `update`            | `play_update()` |
| `physics_update()` before `physics_query_aabb`          | `play_update()` |
| Three-scene structure: title → play → win → title       | factories       |

---

## Summary: The Complete Engine

| Tutorial | Subsystem covered                                                 |
| -------- | ----------------------------------------------------------------- |
| 1        | Architecture, hardware constraints, fixed-point math, VRAM layout |
| 2        | CMake setup, `engine_init`, pad reading, main loop                |
| 3        | Node pool, lifecycle call order, tree wiring, static-data pattern |
| 4        | SceneTree, `scene_change` internals, `g_scene` global pattern     |
| 5        | Signal Connection table, `signal_connect`/`emit`/`disconnect_all` |
| 6        | Node2D world transforms, Camera2D scroll offset                   |
| 7        | Sprite atlas & UV setup, TileMap column culling                   |
| 8        | CollisionShape AABB, `physics_query_aabb`, gem collection         |
| 9        | AudioResource + AudioPlayer, multi-scene game loop                |

The complete geodash project (`game/`) demonstrates all of these systems at once
with a more complex game design. Reading its scene files alongside these
tutorials is the best way to see the engine under real conditions.
