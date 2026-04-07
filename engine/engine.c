#include "engine.h"

void engine_init(void)
{
    /* Renderer: ResetGraph, double-buffer setup, FntLoad, CLUT upload */
    engine_renderer_init();

    /* Core pools */
    node_pool_init();
    signal_table_init();
    physics_init();

    /* Typed node sub-pools */
    node2d_pool_init();
    camera2d_pool_init();
    sprite_pool_init();
    tilemap_pool_init();
    collision_shape_pool_init();
    audio_player_pool_init();

    /* Resource pools */
    texture_resource_pool_init();
    audio_resource_pool_init();
}
