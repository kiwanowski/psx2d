# Engine static library — Godot-inspired 2D engine for PS1 (PSn00bSDK)

add_library(engine STATIC
    engine.c

    core/node.c
    core/signal.c
    core/scene.c
    core/physics.c

    render/vram.c
    render/theme.c
    render/renderer.c

    nodes/node2d.c
    nodes/camera2d.c
    nodes/sprite.c
    nodes/tilemap.c
    nodes/collision_shape.c
    nodes/audio_player.c

    resource/texture.c
    resource/audio.c
)

# Make all engine headers available as #include "engine/..." from the root,
# and as #include "..." from within the engine subtree itself.
target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # allows #include "engine/engine.h"
        ${CMAKE_CURRENT_SOURCE_DIR}      # allows #include "engine.h" inside engine/
)
