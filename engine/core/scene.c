#include "scene.h"
#include "signal.h"
#include <string.h>

void scene_tree_init(SceneTree *tree)
{
    node_pool_init();
    signal_table_init();
    memset(tree, 0, sizeof(SceneTree));
}

void scene_change(SceneTree *tree, SceneFactory factory)
{
    if (tree->root)
        tree_destroy(tree->root);

    /* Reset to a clean state before running the next factory */
    tree->root = 0;
    tree->active_camera = 0;
    signal_table_init();
    node_pool_init();

    if (factory)
        tree->root = factory(tree);

    /* Fire init callbacks on the freshly built tree */
    if (tree->root)
        tree_init(tree->root);
}

void scene_update(SceneTree *tree, int dt)
{
    if (tree->root)
        tree_update(tree->root, dt);
}

void scene_draw(SceneTree *tree)
{
    if (tree->root)
        tree_draw(tree->root);
}
