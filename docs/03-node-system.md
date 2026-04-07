# Tutorial 3 — The Node System

## The Node Struct

Every game object in the engine is a `Node`. The full struct, annotated:

```c
// engine/core/node.h
struct Node {
    char     name[16];      // human-readable label (15 chars + NUL)
    NodeType type;          // BASE, NODE2D, SPRITE, … (determines which sub-pool
                            //   the `data` pointer points into)

    // Lifecycle callbacks — any can be NULL
    void (*init)   (Node *self);            // called once after tree is built
    void (*update) (Node *self, int dt);    // called every frame, pre-order
    void (*draw)   (Node *self);            // called every frame, pre-order
    void (*destroy)(Node *self);            // called on tree teardown, post-order

    Node *parent;                   // NULL for root node
    Node *children[16];             // up to 16 direct children
    int   child_count;

    void *data;     // typed payload — points into a sub-pool (or NULL for BASE)
    int   active;   // 0 = pool slot is free; 1 = live node
};
```

The `dt` parameter passed to `update` is the delta in frames (always `1` in the
fixed-timestep main loop).

---

## The 128-Slot Pool

All nodes live in a single static array:

```c
// Inside node.c (simplified)
static Node s_pool[128];
```

`node_alloc(type, name)` scans for the first slot where `active == 0`, zeroes
it, sets `active = 1`, and returns a pointer. It returns `NULL` if all 128 slots
are taken.

`node_free(node)` detaches the node from its parent, recursively frees all
children, and sets `active = 0` (putting the slot back). It does **not** fire
`destroy` — that is handled by `tree_destroy`.

> **Budget tip**: a typical scene uses 10–30 nodes. The 128 limit is generous
> for a single-screen PS1 game but can be reached by large TileMaps with many
> entity children. Keep entity counts in mind.

---

## Lifecycle Call Order

```
scene_change() is called
│
└── tree_init(root)         PRE-ORDER (parent init fires before children)
    ├── root->init(root)
    ├── child_A->init(child_A)
    └── child_B->init(child_B)

Every frame:
├── tree_update(root, dt)   PRE-ORDER
│   ├── root->update(root, dt)
│   ├── child_A->update(child_A, dt)
│   └── child_B->update(child_B, dt)
│
└── tree_draw(root)         PRE-ORDER
    ├── root->draw(root)
    ├── child_A->draw(child_A)
    └── child_B->draw(child_B)

scene_change() is called again (or game exits):
└── tree_destroy(root)      POST-ORDER (children destroyed before parent)
    ├── child_B->destroy(child_B)
    ├── child_A->destroy(child_A)
    └── root->destroy(root)
    (then node_pool_init clears all slots)
```

**Pre-order for init/update/draw** means a parent can set up state before its
children run. **Post-order for destroy** means children clean up before the
parent that owns them disappears.

---

## Tree Wiring

```c
Node *root   = node_alloc(NODE_TYPE_BASE, "Root");
Node *child  = node_alloc(NODE_TYPE_BASE, "Child");

node_add_child(root, child);  // sets child->parent = root
                              // appends child to root->children[]
                              // increments root->child_count

node_remove_child(root, child); // detaches without freeing
```

`NODE_MAX_CHILDREN` is 16. Adding a 17th child is silently ignored (the slot is
full). Keep hierarchies shallow.

---

## BASE vs Typed Nodes

`NODE_TYPE_BASE` nodes have `data == NULL`. You give them behaviour purely
through custom callbacks. This is the recommended type for scene roots,
controllers, and any object that doesn't need a 2D position or sprite.

**Typed nodes** are created with typed factory functions that allocate both a
`Node` slot and a corresponding entry in a typed sub-pool, storing it in
`node->data`:

| Factory                        | Type enum                   | Data struct          | Sub-pool size |
| ------------------------------ | --------------------------- | -------------------- | ------------- |
| `node2d_create(name)`          | `NODE_TYPE_NODE2D`          | `Node2DData`         | 64            |
| `sprite_create(tree, name)`    | `NODE_TYPE_SPRITE`          | `SpriteData`         | 64            |
| `tilemap_create(tree, name)`   | `NODE_TYPE_TILEMAP`         | `TileMapData`        | 4             |
| `camera2d_create(tree, name)`  | `NODE_TYPE_CAMERA2D`        | `Camera2DData`       | 4             |
| `collision_shape_create(name)` | `NODE_TYPE_COLLISION_SHAPE` | `CollisionShapeData` | 64            |
| `audio_player_create(name)`    | `NODE_TYPE_AUDIO_PLAYER`    | `AudioPlayerData`    | 4             |

When you call a typed factory, the built-in `init`, `update`, `draw`, and
`destroy` callbacks are already wired up. You can still override them — just
save the original pointer and call it from your wrapper if you need both.

---

## The Static-Data Pattern

BASE nodes need somewhere to store their game-specific state. The idiomatic
approach on PS1 — where heap allocation is forbidden — is a **file-static
struct**:

```c
// my_node.c

typedef struct {
    int health;
    int timer;
} EnemyData;

static EnemyData s_data;   // one instance is fine if only one enemy exists

static void enemy_init(Node *self)
{
    (void)self;
    s_data.health = 3;
    s_data.timer  = 0;
}

static void enemy_update(Node *self, int dt)
{
    (void)self; (void)dt;
    if (--s_data.timer <= 0) { /* do something */ }
}

Node *enemy_create(void)
{
    Node *n = node_alloc(NODE_TYPE_BASE, "Enemy");
    n->init   = enemy_init;
    n->update = enemy_update;
    return n;
}
```

If you need multiple instances of the same node type, use a small static array
and allocate from it with a used/free flag — the same pattern the engine itself
uses for all sub-pools.

---

## Checkpoint — Three Nodes, One Tree

This builds a root node with two children. Each child's `draw` callback prints
its name using the PSn00bSDK debug font:

```c
#include <psxetc.h>
#include "engine/engine.h"

/* ── Child node ────────────────────────────────────────────────────────────── */
static void child_draw(Node *self)
{
    FntPrint(-1, "Hello from: %s\n", self->name);
}

/* ── Scene factory ─────────────────────────────────────────────────────────── */
static Node *my_scene_factory(SceneTree *tree)
{
    (void)tree;
    Node *root = node_alloc(NODE_TYPE_BASE, "Root");

    Node *a = node_alloc(NODE_TYPE_BASE, "NodeA");
    a->draw = child_draw;
    node_add_child(root, a);

    Node *b = node_alloc(NODE_TYPE_BASE, "NodeB");
    b->draw = child_draw;
    node_add_child(root, b);

    return root;
}

/* ── Globals ────────────────────────────────────────────────────────────────── */
static char      s_pad_buf[2][34];
static SceneTree s_scene;
SceneTree       *g_scene    = &s_scene;
unsigned short   g_pad_btns = 0xFFFF;
unsigned short   g_pad_prev = 0xFFFF;

/* ── main ───────────────────────────────────────────────────────────────────── */
int main(void)
{
    engine_init();
    InitPAD(s_pad_buf[0], 34, s_pad_buf[1], 34);
    StartPAD();
    ChangeClearPAD(0);

    scene_tree_init(g_scene);
    scene_change(g_scene, my_scene_factory);

    while (1) {
        PADTYPE *pad = (PADTYPE *)s_pad_buf[0];
        g_pad_prev   = g_pad_btns;
        g_pad_btns   = 0xFFFF;
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

Expected output on screen:

```
Hello from: NodeA
Hello from: NodeB
```

<img src="screenshots/03-node-system.png" width="500" height="141">

---

## API Reference

| Function                           | Description                                                         |
| ---------------------------------- | ------------------------------------------------------------------- |
| `node_alloc(type, name)`           | Allocate a node from the 128-slot pool. Returns `NULL` if full.     |
| `node_free(node)`                  | Detach from parent, recursively free children, mark slot inactive.  |
| `node_add_child(parent, child)`    | Wire parent-child relationship. Max 16 children per node.           |
| `node_remove_child(parent, child)` | Detach without freeing.                                             |
| `tree_init(root)`                  | Fire `init` callbacks, pre-order.                                   |
| `tree_update(root, dt)`            | Fire `update` callbacks, pre-order.                                 |
| `tree_draw(root)`                  | Fire `draw` callbacks, pre-order.                                   |
| `tree_destroy(root)`               | Fire `destroy` callbacks, post-order, then mark all slots inactive. |

---

## What's Next

[Tutorial 4 — Scenes & Scene Transitions](04-scenes.md)

Learn how the `SceneTree` manages your node hierarchy and how to switch between
scenes cleanly.
