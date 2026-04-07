#include "node.h"
#include <string.h>

static Node s_pool[NODE_POOL_SIZE];

/* ── Pool management ──────────────────────────────────────────────────────── */

void node_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
}

Node *node_alloc(NodeType type, const char *name)
{
    int i;
    for (i = 0; i < NODE_POOL_SIZE; i++)
    {
        if (!s_pool[i].active)
        {
            memset(&s_pool[i], 0, sizeof(Node));
            s_pool[i].active = 1;
            s_pool[i].type = type;
            if (name)
            {
                strncpy(s_pool[i].name, name, NODE_NAME_LEN - 1);
                s_pool[i].name[NODE_NAME_LEN - 1] = '\0';
            }
            return &s_pool[i];
        }
    }
    return 0; /* pool exhausted */
}

void node_free(Node *node)
{
    int i;
    if (!node || !node->active)
        return;
    if (node->parent)
        node_remove_child(node->parent, node);
    for (i = 0; i < node->child_count; i++)
        node_free(node->children[i]);
    node->active = 0;
}

/* ── Tree wiring ─────────────────────────────────────────────────────────── */

void node_add_child(Node *parent, Node *child)
{
    if (!parent || !child)
        return;
    if (parent->child_count >= NODE_MAX_CHILDREN)
        return;
    child->parent = parent;
    parent->children[parent->child_count++] = child;
}

void node_remove_child(Node *parent, Node *child)
{
    int i, j;
    if (!parent || !child)
        return;
    for (i = 0; i < parent->child_count; i++)
    {
        if (parent->children[i] == child)
        {
            for (j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            parent->children[--parent->child_count] = 0;
            child->parent = 0;
            return;
        }
    }
}

/* ── Recursive traversal ─────────────────────────────────────────────────── */

void tree_init(Node *root)
{
    int i;
    if (!root || !root->active)
        return;
    if (root->init)
        root->init(root);
    for (i = 0; i < root->child_count; i++)
        tree_init(root->children[i]);
}

void tree_update(Node *root, int dt)
{
    int i;
    if (!root || !root->active)
        return;
    if (root->update)
        root->update(root, dt);
    for (i = 0; i < root->child_count; i++)
        tree_update(root->children[i], dt);
}

void tree_draw(Node *root)
{
    int i;
    if (!root || !root->active)
        return;
    if (root->draw)
        root->draw(root);
    for (i = 0; i < root->child_count; i++)
        tree_draw(root->children[i]);
}

void tree_destroy(Node *root)
{
    int i;
    if (!root || !root->active)
        return;
    /* post-order: children before parent */
    for (i = 0; i < root->child_count; i++)
        tree_destroy(root->children[i]);
    root->child_count = 0;
    if (root->destroy)
        root->destroy(root);
    root->active = 0;
}
