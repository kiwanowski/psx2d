#ifndef ENGINE_NODE_H
#define ENGINE_NODE_H

#define NODE_MAX_CHILDREN 16
#define NODE_NAME_LEN 16
#define NODE_POOL_SIZE 128

typedef enum
{
    NODE_TYPE_BASE = 0,
    NODE_TYPE_NODE2D,
    NODE_TYPE_SPRITE,
    NODE_TYPE_TILEMAP,
    NODE_TYPE_CAMERA2D,
    NODE_TYPE_COLLISION_SHAPE,
    NODE_TYPE_AUDIO_PLAYER,
} NodeType;

typedef struct Node Node;

struct Node
{
    char name[NODE_NAME_LEN];
    NodeType type;

    /* Lifecycle callbacks — any may be NULL */
    void (*init)(Node *self);
    void (*update)(Node *self, int dt);
    void (*draw)(Node *self);
    void (*destroy)(Node *self);

    Node *parent;
    Node *children[NODE_MAX_CHILDREN];
    int child_count;

    void *data; /* typed payload — points into a typed sub-pool */
    int active; /* 0 = pool slot free, 1 = live */
};

/* Pool management ---------------------------------------------------------- */
void node_pool_init(void);
Node *node_alloc(NodeType type, const char *name); /* returns NULL if pool full */
void node_free(Node *node);                        /* frees node + all descendants */

/* Tree wiring -------------------------------------------------------------- */
void node_add_child(Node *parent, Node *child);
void node_remove_child(Node *parent, Node *child);

/* Recursive traversal ------------------------------------------------------ */
/* tree_init:    calls init()    pre-order  (parent before children)          */
/* tree_update:  calls update()  pre-order                                    */
/* tree_draw:    calls draw()    pre-order                                    */
/* tree_destroy: calls destroy() post-order (children before parent), then   */
/*               marks every node in the subtree inactive                     */
void tree_init(Node *root);
void tree_update(Node *root, int dt);
void tree_draw(Node *root);
void tree_destroy(Node *root);

#endif /* ENGINE_NODE_H */
