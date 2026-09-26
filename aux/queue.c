#include <stdio.h>

#include "queue.h"


#define IS_FULL(q) ((q->end + 1) % q->size == q->start)
#define IS_EMPTY(q) (q->start == q->end)


void
queue_put(queue_t *q, int e)
{
    if (IS_FULL(q)) {
        printf(
            "[Queue Put of %d Error]      \
            Full Queue! Insertion Canceled.", e);
        return;
    }

    q->buffer[q->end] = e;
    q->end = (q->end + 1) % q->size;
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
    return q->buffer[idx];
}

#undef IS_FULL
#undef IS_EMPTY
