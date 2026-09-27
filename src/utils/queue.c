#include <stdio.h>

#include "utils/queue.h"


#define IS_FULL(q) (q->qtd == q->size)
#define IS_EMPTY(q) (q->qtd == 0)


DebugRet
queue_put(queue_t *q, int e)
{
    if (IS_FULL(q)) {
        printf(
            "[Queue Put of %d Error]            \
            Full Queue! Insertion Canceled.", e);
        return DEBUG_RET_FULL_QUEUE;
    }

    q->buffer[q->end] = e;
    q->end = (q->end + 1) % q->size;
    q->qtd++;
    return DEBUG_RET_SUCCESS;
}


inline int
queue_get(queue_t *q)
{
    if (IS_EMPTY(q)) {
        printf("[Queue Get] Empty Queue!");
        return -1;
    }
    
    int idx = q->start;
    q->start = (q->start + 1) % q->size;
    q->qtd--;
    return q->buffer[idx];
}

#undef IS_FULL
#undef IS_EMPTY
