# Tutorial 4 — Scenes & Scene Transitions

## The SceneTree Struct

```c
// engine/core/scene.h
struct SceneTree {
    Node *root;            // root node of the currently active scene
    Node *active_camera;   // set by Camera2D on init; read by Sprite on draw
};
```

`root` is the entry point for `tree_update` and `tree_draw`. When it is `NULL`
(between transitions), neither function does anything.

`active_camera` lets any Sprite node find the current camera without a direct
pointer dependency. `camera2d_create` sets this field during `init` and clears
it during `destroy`.

---

## SceneFactory

A **SceneFactory** is a function that builds a node tree and returns the root:

```c
typedef Node *(*SceneFactory)(SceneTree *tree);
```

The `SceneTree *tree` argument is provided so the factory can register a camera
(`tree->active_camera = cam`) or pass the tree pointer into nodes that need it
(cameras, sprites, tilemaps). Factories receive a freshly reset tree — all pools
are empty when the factory runs.

---

## scene_tree_init

```c
void scene_tree_init(SceneTree *tree);
```

Called **once** before the main loop. It calls `node_pool_init()` and
`signal_table_init()`, then zeroes the tree struct. Do not call it again — use
`scene_change` for transitions.

---

## scene_change Internals

`scene_change(tree, factory)` is the single function that handles all scene
transitions. Internally it runs these steps in order:

```
1. tree_destroy(tree->root)   — fires destroy callbacks post-order,
                                marks all node slots inactive
2. tree->root          = NULL
3. tree->active_camera = NULL
4. signal_table_init()        — clears the 64-slot connection table
5. node_pool_init()           — all 128 node slots are now free again
                                (typed sub-pools are also reinitialised
                                 by engine_init — they stay valid since
                                 scene_change does not call engine_init)
6. tree->root = factory(tree) — factory allocates nodes into the clean pool
7. tree_init(tree->root)      — fires init callbacks pre-order
```

After step 7 the new scene is live and the main loop continues normally.

> **Important**: `scene_change` completes synchronously. By the time it returns,
> the old scene is gone and the new one is fully initialised.

---

## Triggering Transitions from Inside a Node

Node `update` callbacks only receive `(Node *self, int dt)` — they have no
direct access to the `SceneTree`. The recommended pattern is a global pointer:

```c
// game_session.h (or equivalent)
extern SceneTree *g_scene;

// game_main.c
static SceneTree s_scene;
SceneTree       *g_scene = &s_scene;
```

Then any node's `update` callback can trigger a transition:

```c
static void title_update(Node *self, int dt)
{
    (void)self; (void)dt;
    if (BTN_TAPPED(PAD_CROSS))
        scene_change(g_scene, play_scene_factory);
}
```

> **Never call `scene_change` from a `draw` callback.** Drawing populates the
> prim buffer; invalidating the tree mid-draw leaves dangling pointers in the
> OT. Always transition from `update`.

---

## Anatomy of a Scene File

Every scene in the game follows the same four-part structure:

```c
// scenes/my_scene.c

#include "my_scene.h"
#include "other_scene.h"          // needed for the transition target
#include "../game_session.h"      // for g_scene, BTN_TAPPED, g_pad_btns

/* 1. Static state ─────────────────────────────────────────────────────────── */
typedef struct {
    int counter;
} MySceneData;

static MySceneData s_data;

/* 2. Lifecycle callbacks ───────────────────────────────────────────────────── */
static void my_init(Node *self)
{
    (void)self;
    s_data.counter = 0;
}

static void my_update(Node *self, int dt)
{
    (void)self; (void)dt;
    s_data.counter++;
    if (BTN_TAPPED(PAD_CROSS))
        scene_change(g_scene, other_scene_factory);
}

static void my_draw(Node *self)
{
    (void)self;
    FntPrint(-1, "Frames: %d", s_data.counter);
}

/* 3. Factory ───────────────────────────────────────────────────────────────── */
Node *my_scene_factory(SceneTree *tree)
{
    Node *root = node_alloc(NODE_TYPE_BASE, "MyScene");
    root->init   = my_init;
    root->update = my_update;
    root->draw   = my_draw;
    (void)tree;
    return root;
}
```

```c
// scenes/my_scene.h
#ifndef MY_SCENE_H
#define MY_SCENE_H
#include "../../engine/core/scene.h"

Node *my_scene_factory(SceneTree *tree);

#endif
```

The header only exports the factory function. The internal state (`MySceneData`)
stays private to the `.c` file.

---

## Checkpoint — Two Scenes

This demo has **Scene A** (blue background, frame counter) and **Scene B** (red
background, "Press X to go back"). Pressing X switches between them.

```c
// two_scenes.c
#include <psxpad.h>
#include <psxetc.h>
#include "engine/engine.h"

static char      s_pad_buf[2][34];
static SceneTree s_scene;
SceneTree       *g_scene    = &s_scene;
unsigned short   g_pad_btns = 0xFFFF;
unsigned short   g_pad_prev = 0xFFFF;
#define BTN_TAPPED(b) ((g_pad_prev & (b)) && !(g_pad_btns & (b)))

/* Forward declarations */
static Node *scene_a_factory(SceneTree *tree);
static Node *scene_b_factory(SceneTree *tree);

/* ── Scene A ────────────────────────────────────────────────────────────────── */
static int s_frames;

static void a_init(Node *self)    { (void)self; s_frames = 0; }
static void a_update(Node *self, int dt)
{
    (void)self; (void)dt;
    s_frames++;
    if (BTN_TAPPED(PAD_CROSS)) scene_change(g_scene, scene_b_factory);
}
static void a_draw(Node *self)
{
    (void)self;
    FntPrint(-1, "Scene A — frame %d\nPress X for Scene B", s_frames);
}
static Node *scene_a_factory(SceneTree *tree)
{
    Node *r = node_alloc(NODE_TYPE_BASE, "SceneA");
    r->init = a_init; 
    r->update = a_update; 
    r->draw = a_draw;
    (void)tree; return r;
}

/* ── Scene B ────────────────────────────────────────────────────────────────── */
static void b_update(Node *self, int dt)
{
    (void)self; (void)dt;
    if (BTN_TAPPED(PAD_CROSS)) scene_change(g_scene, scene_a_factory);
}
static void b_draw(Node *self)
{
    (void)self;
    FntPrint(-1, "Scene B\nPress X to go back");
}
static Node *scene_b_factory(SceneTree *tree)
{
    Node *r = node_alloc(NODE_TYPE_BASE, "SceneB");
    r->update = b_update; 
    r->draw = b_draw;
    (void)tree; return r;
}

/* ── main ───────────────────────────────────────────────────────────────────── */
int main(void)
{
    engine_init();
    InitPAD(s_pad_buf[0], 34, s_pad_buf[1], 34);
    StartPAD();
    ChangeClearPAD(0);

    scene_tree_init(g_scene);
    scene_change(g_scene, scene_a_factory);

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

<img src="screenshots/04-scenes.png" width="600px">

---

## API Reference

| Function                      | Description                                                       |
| ----------------------------- | ----------------------------------------------------------------- |
| `scene_tree_init(tree)`       | One-time init: resets node pool and signal table.                 |
| `scene_change(tree, factory)` | Destroy current scene, reset all pools, build and init new scene. |
| `scene_update(tree, dt)`      | Call `tree_update(root, dt)` if root is non-NULL.                 |
| `scene_draw(tree)`            | Call `tree_draw(root)` if root is non-NULL.                       |

---

## What's Next

[Tutorial 5 — Signals](05-signals.md)

Learn how nodes communicate without knowing about each other.
