# PSX2D

A lightweight, Godot-inspired 2D game engine for the original **PlayStation 1**,
written in C.

PSX2D lets you write PS1 games using a node tree, scene transitions, and a
signal system — instead of raw GPU primitives and polling loops. It builds as a
static library via CMake and links into a standard PS1 executable produced by
[PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK).

---

## Features

- **Node system** — tree-based game objects with `init`, `update`, `draw`, and
  `destroy` callbacks
- **Scene management** — scene transitions that destroy the old tree, reset all
  pools, and call a factory to build the new one
- **Signals** — lightweight event connections modeled after Godot's signal
  system
- **Physics** — AABB collision detection with layer/scan masks; no dynamic
  allocation
- **Node2D / Camera2D** — 2D transforms with 16.16 fixed-point math and GTE
  rotation units
- **Sprites** — atlas-based POLY_FT4 sprites with flip, rotation inheritance,
  and theme CLUT support
- **TileMap** — row-major tile layer with frustum culling and configurable tile
  atlas layout
- **Audio** — XA streaming from CD-ROM via PSn00bSDK SPU helpers
- **Renderer** — 320×240 double-buffered display, 8-slot ordering table, 32 KB
  primitive buffer, 3 built-in themes
- **Zero dynamic allocation** — all subsystems use static fixed-size pools to
  stay within the 2 MB PS1 RAM budget

---

## Requirements

- [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK) 0.24+ (provides
  `mipsel-none-elf-gcc`, `mkpsxiso`, and CMake module)
- CMake 3.21+
- Ninja

---

## Building

Configure once using the preset defined in `CMakePresets.json`:

```sh
cmake --preset <preset-name>
```

Then build:

```sh
cmake --build ./build
```

Outputs are placed under `build/mygame/`:

- `mygame` — PS1 ELF executable
- `mygame.cue` / `cd_image_*.xml` — CD image ready for emulation or burning

---

## Project Structure

```
psx2d/
├── engine/               Static library — the engine itself
│   ├── engine.h/.c       Single include + engine_init()
│   ├── math/fixed.h      16.16 fixed-point macros (header-only)
│   ├── core/             node, signal, scene, physics
│   ├── render/           ordering table, VRAM allocator, theme, renderer
│   ├── nodes/            node2d, sprite, tilemap, camera2d,
│   │                     collision_shape, audio_player
│   └── resource/         texture (TIM loader), audio (XA)
├── mygame/               Sample game: Gem Collector
│   ├── src/main.c        Complete three-scene game
│   ├── tools/            tiled_export.py — Tiled → C array code generator
│   ├── levels/           Tiled map sources
│   ├── tileset.tim       Sprite/tile atlas
│   └── music.xa          XA background music
├── lib/                  psxmcrd memory card helper
└── docs/                 Tutorial docs (01–10)
```

---

## Getting Started

Include the engine header and call `engine_init()` before anything else:

```c
#include "engine.h"

int main(void) {
    engine_init();

    // Audio requires manual init (not done by engine_init)
    CdInit();
    SpuInit();

    // Load your texture atlas
    TextureResource tileset;
    texture_resource_from_tim(_binary_TILESET_TIM, &tileset);

    // Set up controller input
    InitPAD(pad_buf[0], 34, pad_buf[1], 34);
    StartPAD();

    // Build initial scene and enter the main loop
    SceneTree scene;
    scene_tree_init(&scene);
    scene_change(&scene, my_title_factory);

    while (1) {
        scene_update(&scene, 1);

        engine_renderer_begin_frame();
        scene_draw(&scene);
        engine_renderer_end_frame();
    }
}
```

See [`mygame/src/main.c`](mygame/src/main.c) for a complete working example
covering all engine subsystems.

---

## Node System

Every game object is a `Node`. Specialised node types (`Node2D`, `Sprite`,
`TileMap`, `Camera2D`, `CollisionShape`, `AudioPlayer`) are allocated from fixed
sub-pools and attached via the `data` pointer.

```c
Node *player = node2d_create("Player");
Node *sprite = sprite_create("Sprite");
node_add_child(player, sprite);

// Nodes are freed recursively — freeing the parent frees all children
node_free(player);
```

Pool limits: 128 total nodes, 64 Node2Ds, 64 Sprites, 4 TileMaps, 4 Camera2Ds,
64 CollisionShapes, 64 AudioPlayers.

---

## Ordering Table Layers

| OT Index | Constant            | Use                   |
| -------- | ------------------- | --------------------- |
| 7        | `ENGINE_OT_BG`      | Background / sky      |
| 6        | `ENGINE_OT_TILES`   | TileMap layer         |
| 4        | `ENGINE_OT_OBJ`     | Enemies, collectibles |
| 2        | `ENGINE_OT_PLAYER`  | Player sprite         |
| 1        | `ENGINE_OT_UI`      | HUD                   |
| 0        | `ENGINE_OT_OVERLAY` | Full-screen flash     |

---

## Documentation

Step-by-step tutorials are in the [`docs/`](docs/) directory:

| File                                                                  | Topic                    |
| --------------------------------------------------------------------- | ------------------------ |
| [01-engine-overview.md](docs/01-engine-overview.md)                   | Architecture overview    |
| [02-getting-started.md](docs/02-getting-started.md)                   | Setting up a project     |
| [03-node-system.md](docs/03-node-system.md)                           | Nodes and the tree       |
| [04-scenes.md](docs/04-scenes.md)                                     | Scene management         |
| [05-signals.md](docs/05-signals.md)                                   | Signals and events       |
| [06-node2d-camera2d.md](docs/06-node2d-camera2d.md)                   | 2D transforms and camera |
| [07-sprites-tilemap.md](docs/07-sprites-tilemap.md)                   | Sprites and tile maps    |
| [08-physics.md](docs/08-physics.md)                                   | AABB collision           |
| [09-audio-complete.md](docs/09-audio-complete.md)                     | XA audio streaming       |
| [10-texture-resource-tilemap.md](docs/10-texture-resource-tilemap.md) | Texture resources        |

---

## License

Mozilla Public License 2.0
