# Tutorial 2 — Getting Started: Project Setup & the Main Loop

## Prerequisites

- **PSn00bSDK** installed (tested with 0.24). The toolchain provides
  `mipsel-none-elf-gcc`, `mkpsxiso`, and the PSn00bSDK CMake module.
- **CMake 3.21+** and **Ninja** (or Make).
- The `engine/` folder from this repository copied into your project root.

---

## CMakeLists.txt Setup

A minimal project that uses the engine looks like this:

```cmake
cmake_minimum_required(VERSION 3.21)

project(mygame LANGUAGES C ASM)

# 1. Build the engine as a static library
add_subdirectory(engine)

# 2. Add your game executable
psn00bsdk_add_executable(mygame GPREL
    src/main.c
)

# 3. Link the engine
target_link_libraries(mygame PRIVATE engine)

# 4. Embed a TIM sprite atlas as a binary blob
#    The symbol _binary_SPRITES_TIM is then available as an extern uint32_t[].
psn00bsdk_target_incbin(mygame PRIVATE _binary_SPRITES_TIM sprites.tim)

# 5. Build a CD image
psn00bsdk_add_cd_image(iso mygame iso.xml DEPENDS mygame system.cnf)
```

The `engine` target is defined in `engine/CMakeLists.txt` as a static library
and automatically exposes its include paths — you only need
`#include "engine/engine.h"` in your source.

---

## The `engine_init()` Call

`engine_init()` is the single entry point that initialises every subsystem in
dependency order:

1. `engine_renderer_init()` — calls `ResetGraph(0)`, sets up double-buffered
   `DISPENV`/`DRAWENV` pairs, loads the debug font, uploads all theme CLUTs to
   VRAM.
2. `vram_init()` — resets the VRAM bump allocator.
3. All node sub-pool inits (`node_pool_init`, `node2d_pool_init`,
   `sprite_pool_init`, `collision_shape_pool_init`, …).
4. `signal_table_init()` and `physics_init()`.

Call it **before anything else** in `main()`.

---

## Loading a Sprite Atlas

The engine's `Sprite` node draws textured quads. Textures are uploaded once at
startup with `engine_renderer_load_atlas()`:

```c
// Declared by psn00bsdk_target_incbin in CMakeLists.txt
extern const uint32_t _binary_SPRITES_TIM[];

uint16_t tpage = engine_renderer_load_atlas(_binary_SPRITES_TIM);
```

The function parses the TIM file, uploads the pixel data and CLUT to VRAM via
the bump allocator, and returns the **packed tpage word** you set on every
`SpriteData.tpage` that uses this atlas.

> You can call `engine_renderer_load_atlas` multiple times for multiple atlases;
> VRAM space permitting.

---

## Pad Reading

PSn00bSDK's PAD API writes raw button data into a pair of buffers. The engine
doesn't abstract this — you read it yourself once per frame and store the result
in two globals that scene nodes can read:

```c
static char s_pad_buf[2][34];   // storage provided to PSn00bSDK

// In main() before the loop:
InitPAD(s_pad_buf[0], 34, s_pad_buf[1], 34);
StartPAD();
ChangeClearPAD(0);  // suppress the "controller not connected" clear

// At the top of the game loop:
PADTYPE *pad   = (PADTYPE *)s_pad_buf[0];
g_pad_prev     = g_pad_btns;       // save last frame
g_pad_btns     = 0xFFFF;           // default: all buttons released
if (pad->stat == 0 &&
    (pad->type == 0x4 || pad->type == 0x5 || pad->type == 0x7))
    g_pad_btns = pad->btn;
```

**Active-low convention**: a button bit is **0** when held and **1** when
released. Edge detection (tapped this frame):

```c
// Button was just pressed (was 1 last frame, is 0 this frame):
#define BTN_TAPPED(b)  ( (g_pad_prev & (b)) && !(g_pad_btns & (b)) )
// Button is currently held down:
#define BTN_HELD(b)    (!(g_pad_btns & (b)))
```

Useful button constants from `<psxpad.h>`: `PAD_CROSS`, `PAD_CIRCLE`,
`PAD_SQUARE`, `PAD_TRIANGLE`, `PAD_UP`, `PAD_DOWN`, `PAD_LEFT`, `PAD_RIGHT`,
`PAD_L1`, `PAD_L2`, `PAD_R1`, `PAD_R2`, `PAD_START`, `PAD_SELECT`.

---

## The Main Loop

The canonical game loop, annotated:

```c
while (1) {
    // 1. Read pad (see above) — do this first so scene nodes see fresh input

    // 2. Update all scene logic (calls every active node's update callback)
    scene_update(g_scene, 1);
    //                     ^ dt — pass 1 for one-frame fixed timestep

    // 3. Swap and clear the prim buffer, clear the OT
    engine_renderer_begin_frame();

    // 4. Draw all scene nodes (populates OT + prim buffer)
    scene_draw(g_scene);

    // 5. FntFlush → DrawSync(0) → VSync(0) → swap display/draw buffers
    engine_renderer_end_frame();
}
```

**Why update before render?** `engine_renderer_begin_frame` clears the prim
buffer. Any draw calls that happen before it (e.g. in `update`) would be writing
into the buffer that is currently being DMA'd to the GPU. Always keep logic in
`update` and visuals in `draw`.

---

## Checkpoint — Minimal Main

This produces a blank screen with the theme sky colour and exits cleanly on
pressing START:

```c
#include <psxpad.h>
#include <psxapi.h>
#include "engine/engine.h"

static char     s_pad_buf[2][34];
static SceneTree s_scene;
SceneTree       *g_scene    = &s_scene;
unsigned short   g_pad_btns = 0xFFFF;
unsigned short   g_pad_prev = 0xFFFF;

int main(void)
{
    engine_init();

    InitPAD(s_pad_buf[0], 34, s_pad_buf[1], 34);
    StartPAD();
    ChangeClearPAD(0);

    scene_tree_init(g_scene);   // initialise empty tree (no factory yet)

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

---

## What's Next

[Tutorial 3 — The Node System](03-node-system.md)

Learn how the 128-slot pool works, how lifecycle callbacks fire, and how to
write your first custom node.
