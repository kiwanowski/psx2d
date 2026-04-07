# Tutorial 1 — Engine Overview & Architecture

## What We Built and Why

This engine is a lightweight, Godot-inspired 2D game framework for the original
PlayStation (PS1). Its goals:

- A **node-based scene graph** for organising game objects hierarchically
- A **double-buffered renderer** that wraps the PS1's Ordering Table (OT)
- A **fixed-point math** layer suited to the PS1's integer-only CPU
- **Physics**, **signals**, **audio**, and **resource** subsystems, each backed
  by a fixed-size pool that fits in the 2 MB RAM budget

The engine lets you write PS1 games in terms of `Node` trees and `SceneTree`
transitions rather than raw GPU primitives and manual polling loops.

---

## PS1 Hardware Constraints

Every design decision in the engine is a direct consequence of the hardware.

| Resource | Limit                                  | Engine consequence                                                      |
| -------- | -------------------------------------- | ----------------------------------------------------------------------- |
| RAM      | 2 MB total                             | All pools are static arrays — no `malloc` anywhere                      |
| VRAM     | 1 MB (1024 × 512 px, 16-bit)           | Bump allocator; two 320×240 display buffers eat ~150 KB                 |
| CPU      | MIPS R3000A @ 33 MHz, **integer only** | All positions stored as `fixed_t` (16.16); no `float`                   |
| GTE      | Hardware geometry coprocessor          | `rcos`/`rsin` work in a 4096-unit circle; used by `Sprite` for rotation |
| GPU      | OT-driven tile renderer                | Primitives are chained into an Ordering Table and DMA'd back-to-front   |
| CD-ROM   | 2× speed                               | XA audio is streamed continuously from disc                             |

---

## The Ordering Table

The PS1 GPU draws primitives **back-to-front** by walking a linked list called
the **Ordering Table (OT)**. Each slot is a bucket — primitives in the same slot
draw in undefined order, but slots are always flushed highest-index first.

The engine defines **8 OT slots**:

```
Index  Constant             Purpose
  7    ENGINE_OT_BG         Sky / background fill         ← drawn first (back)
  6    ENGINE_OT_TILES      TileMap layer
  5    (spare)              e.g. glow behind objects
  4    ENGINE_OT_OBJ        Enemies, pickups, obstacles
  3    (spare)
  2    ENGINE_OT_PLAYER     Player sprite
  1    ENGINE_OT_UI         HUD, menus
  0    ENGINE_OT_OVERLAY    Full-screen flash, overlays   ← drawn last (front)
```

**Lower index draws on top.** Use `engine_render_ot(ENGINE_OT_*)` to obtain the
OT entry pointer for a layer, then pass it to PSn00bSDK's `addPrim()`.

---

## Subsystem Map

```
engine_init()
│
├── engine/math/fixed.h            fixed_t arithmetic macros (header-only)
│
├── engine/core/
│   ├── node.h / node.c            128-slot Node pool, tree traversal
│   ├── signal.h / signal.c        64-slot Connection table
│   ├── scene.h / scene.c          SceneTree, scene_change()
│   └── physics.h / physics.c      AABB queries, 64-shape registry
│
├── engine/render/
│   ├── ot.h                       OT layer constants (header-only)
│   ├── vram.h / vram.c            VRAM bump allocator
│   ├── theme.h / theme.c          3 built-in colour themes + CLUT upload
│   └── renderer.h / renderer.c    Double-buffer, prim buffer, frame bookends
│
├── engine/nodes/
│   ├── node2d.h / node2d.c        2D position + rotation, 64-slot pool
│   ├── camera2d.h / camera2d.c    Camera scroll offset, 4-slot pool
│   ├── sprite.h / sprite.c        Textured quad (POLY_FT4), GTE rotation, 64-slot pool
│   ├── tilemap.h / tilemap.c      Tile grid with column culling, 4-slot pool
│   ├── collision_shape.h / .c     AABB physics body, 64-slot pool
│   └── audio_player.h / .c        XA audio streaming node, 4-slot pool
│
└── engine/resource/
    ├── texture.h / texture.c      TIM parsing + VRAM upload
    └── audio.h / audio.c          CdSearchFile wrapper for XA files
```

---

## Pool Budget

Every subsystem uses a fixed-size static array. Nothing is heap-allocated.

| Pool                | Capacity      | What fills it                                          |
| ------------------- | ------------- | ------------------------------------------------------ |
| Node                | 128           | Every `node_alloc` / typed `_create` call              |
| Node2D data         | 64            | Each `node2d_create`                                   |
| Sprite data         | 64            | Each `sprite_create`                                   |
| CollisionShape data | 64            | Each `collision_shape_create`                          |
| Camera2D data       | 4             | Typically one per scene                                |
| TileMap data        | 4             |                                                        |
| AudioPlayer data    | 4             |                                                        |
| Signal connections  | 64            | Each `signal_connect`; cleared on every `scene_change` |
| Physics shapes      | 64            | Auto-registered by CollisionShape on `init`            |
| Prim buffer         | 32 KB × 2     | All GPU primitives drawn in a single frame             |
| OT                  | 8 entries × 2 | Double-buffered, reset each frame                      |

> If a pool is exhausted, `node_alloc` returns `NULL` and `signal_connect`
> returns `-1`. Check these during development.

---

## Fixed-Point Math

The PS1 has no FPU. The engine represents all positions, velocities, and
sub-pixel offsets as **`fixed_t`** — an `int32_t` in **16.16 format**: the upper
16 bits are the integer part, the lower 16 bits are the fraction.

```
Value   fixed_t hex       Decimal check
1.0     0x00010000        65536
0.5     0x00008000        32768
100.0   0x00640000
-1.5    0xFFFE8000
```

The macros in `engine/math/fixed.h`:

```c
#include "engine/math/fixed.h"

fixed_t x    = INT_TO_FIXED(100);           // 100    → 0x00640000
fixed_t vel  = FLOAT_TO_FIXED(1.5);         // 1.5    → compile-time constant only
int     px   = FIXED_TO_INT(x);             // 0x00640000 → 100  (truncates fraction)

// Plain + and - work directly on fixed_t
fixed_t next = x + vel;                     // fine

// Multiplication and division need the macros (use int64_t internally)
fixed_t area  = FIXED_MUL(x, vel);
fixed_t speed = FIXED_DIV(x, INT_TO_FIXED(4));
```

**Rule of thumb**: keep all state as `fixed_t`; call `FIXED_TO_INT` only when
writing screen coordinates to GPU primitive fields (`setXY0`, `setXY4`, etc.).

---

## VRAM Layout

```
VRAM — 1024 × 512, 16-bit pixels
  x:   0         320                       960 1024
       ┌──────────┬─────────────────────────┐
  y=0  │ Display  │                         │
       │ buffer 0 │   Texture atlas         │
       │ 320×240  │   (bump-allocated       │
       │          │    from x=320, y=0      │
y=239  │          │    rightward then down) │
       ├──────────┤                         │
y=240  │ Display  │                         │
       │ buffer 1 │                         │
       │ 320×240  │                         │
y=479  │          │                         │
       ├──────────┴─────────────────────────┤
y=480  │ CLUT row                           │
       │  theme 0 @ x=0                     │
       │  theme 1 @ x=16                    │
       │  theme 2 @ x=32                    │
       └────────────────────────────────────┘
  x=960..1023, y=0..63  reserved — PSn00bSDK debug font
```

`vram_alloc()` starts at `x=320, y=0`, walks right then wraps down, capping at
`x=960` on the font row (`y < 64`) and stopping before `y=480` (the CLUT row).

---

## What's Next

[Tutorial 2 — Getting Started: Project Setup & the Main Loop](02-getting-started.md)

Learn how to wire the engine into your `CMakeLists.txt` and write a working game
loop.
