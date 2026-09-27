/**
 * @file queue.c
 * @brief Implementation of the circular queue operations.
 */

#include <stdio.h>

#include "utils/queue.h"

/** @brief Checks if the queue has reached its maximum capacity. */
#define IS_FULL(q) ((q)->qtd == (q)->size)

/** @brief Checks if the queue is completely empty. */
#define IS_EMPTY(q) ((q)->qtd == 0)

DebugRet queue_put(queue_t *q, int e)
{
    if (IS_FULL(q)) {
        printf("[QUEUE ERROR] Full Queue! Insertion of %d canceled.\n", e);
        return DEBUG_RET_FULL_QUEUE;
    }

    q->buffer[q->end] = e;
    q->end = (q->end + 1) % q->size;
    q->qtd++;
    
    return DEBUG_RET_SUCCESS;
}

int queue_get(queue_t *q)
{
    if (IS_EMPTY(q)) {
        printf("[QUEUE ERROR] Empty Queue! Cannot retrieve element.\n");
        return -1;
    }
    
    int idx = q->start;
    q->start = (q->start + 1) % q->size;
    q->qtd--;
    
    return q->buffer[idx];
}

#undef IS_FULL
#undef IS_EMPTY
