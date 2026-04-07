#ifndef ENGINE_SCENE_H
#define ENGINE_SCENE_H

#include "node.h"

typedef struct SceneTree SceneTree;

/* A SceneFactory builds a new scene into the node pool and returns the root.
   The tree pointer is provided so the factory can set e.g. active_camera. */
typedef Node *(*SceneFactory)(SceneTree *tree);

struct SceneTree
{
    Node *root;
    Node *active_camera; /* set by Camera2D node on init; read by Sprite draw */
};

/* One-time initialisation: calls node_pool_init() + signal_table_init() */
void scene_tree_init(SceneTree *tree);

/* Destroy current scene, reset pool + signal table, build new scene.
   active_camera is cleared before the factory runs.                    */
void scene_change(SceneTree *tree, SceneFactory factory);

/* Per-frame helpers — call in main loop */
void scene_update(SceneTree *tree, int dt);
void scene_draw(SceneTree *tree);

#endif /* ENGINE_SCENE_H */
