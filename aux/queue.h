#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

#include "kernelsim_params.h"


typedef struct
{
    int* buffer;
    int start;
    int end;
    int size;
} queue_t;


#define _QUEUE_INIT_IMPL(name, q_size)       \
    static int _##name##_buffer[q_size];    \
    static queue_t name = {                 \
        .buffer = _##name##_buffer,         \
        .start = 0,                         \
        .end = 0,                           \
        .size = q_size                      \
    }

#define QUEUE_INIT(name, q_size) _QUEUE_INIT_IMPL(name, q_size)


void
queue_put(queue_t *q, int e);

int
queue_get(queue_t *q);

#endif