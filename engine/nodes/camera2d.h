#ifndef ENGINE_CAMERA2D_H
#define ENGINE_CAMERA2D_H

#include "../core/node.h"
#include "../core/scene.h"
#include "node2d.h"

#define CAMERA2D_POOL_SIZE 4

/*
 * Camera2DData — payload for NODE_TYPE_CAMERA2D nodes.
 *
 * A Camera2D is always a child of a Node2D (or itself one), and reads the
 * parent Node2D's world position as the scroll offset applied at draw time.
 *
 * On init() the camera registers itself with the SceneTree as the active
 * camera.  Only one camera is active per scene.
 *
 * Offset (ox, oy): additional sub-pixel scroll fine-tuning, separate from
 * the Node2D position.  Usually left at 0.
 */
typedef struct
{
    SceneTree *tree; /* back-pointer so init() can register itself */
    int ox;          /* additional integer X offset */
    int oy;          /* additional integer Y offset */
} Camera2DData;

/*
 * camera2d_create — allocate and wire up a Camera2D node.
 *   tree : the SceneTree this camera will register with on init().
 *   name : node name.
 */
Node *camera2d_create(SceneTree *tree, const char *name);

/*
 * camera2d_get_offset — return the current world-space scroll offset.
 *   Reads the parent Node2D's wx/wy plus the camera's own ox/oy.
 *   Pass the result to Sprite/TileMap draw calls.
 *   Returns (0, 0) if no parent Node2D is found.
 */
void camera2d_get_offset(const Node *cam, int *ox, int *oy);

/* Sub-pool reset */
void camera2d_pool_init(void);

#endif /* ENGINE_CAMERA2D_H */
