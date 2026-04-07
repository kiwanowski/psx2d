#include "signal.h"
#include <string.h>

static Connection s_table[SIGNAL_TABLE_SIZE];

void signal_table_init(void)
{
    memset(s_table, 0, sizeof(s_table));
}

int signal_connect(uint32_t signal_id, Node *emitter,
                   Node *target, SignalCallback cb)
{
    int i;
    for (i = 0; i < SIGNAL_TABLE_SIZE; i++)
    {
        if (!s_table[i].active)
        {
            s_table[i].signal_id = signal_id;
            s_table[i].emitter = emitter;
            s_table[i].target = target;
            s_table[i].callback = cb;
            s_table[i].active = 1;
            return i;
        }
    }
    return -1; /* table full */
}

void signal_disconnect_all(Node *node)
{
    int i;
    for (i = 0; i < SIGNAL_TABLE_SIZE; i++)
    {
        if (s_table[i].active &&
            (s_table[i].emitter == node || s_table[i].target == node))
            s_table[i].active = 0;
    }
}

void signal_emit(uint32_t signal_id, Node *emitter, void *args)
{
    int i;
    for (i = 0; i < SIGNAL_TABLE_SIZE; i++)
    {
        if (!s_table[i].active)
            continue;
        if (s_table[i].signal_id != signal_id)
            continue;
        if (s_table[i].emitter && s_table[i].emitter != emitter)
            continue;
        if (s_table[i].callback)
            s_table[i].callback(emitter, args);
    }
}
