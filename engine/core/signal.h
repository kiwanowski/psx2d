#ifndef ENGINE_SIGNAL_H
#define ENGINE_SIGNAL_H

#include "node.h"
#include <stdint.h>

#define SIGNAL_TABLE_SIZE 64

/* Predefined signal IDs — game code may define its own above SIGNAL_USER */
#define SIGNAL_BODY_ENTERED 0x0001u
#define SIGNAL_BODY_EXITED 0x0002u
#define SIGNAL_ANIM_FINISHED 0x0003u
#define SIGNAL_DIED 0x0004u
#define SIGNAL_USER 0x0100u /* first game-defined ID */

typedef void (*SignalCallback)(Node *emitter, void *args);

typedef struct
{
    uint32_t signal_id;
    Node *emitter; /* NULL = match any emitter */
    Node *target;  /* informational; callback receives it */
    SignalCallback callback;
    int active;
} Connection;

void signal_table_init(void);

/* Returns connection index on success, -1 if table is full */
int signal_connect(uint32_t signal_id, Node *emitter,
                   Node *target, SignalCallback cb);

/* Remove all connections where node is emitter or target.
   Call from a node's destroy() callback. */
void signal_disconnect_all(Node *node);

/* Fire all active connections matching (signal_id, emitter) */
void signal_emit(uint32_t signal_id, Node *emitter, void *args);

#endif /* ENGINE_SIGNAL_H */
