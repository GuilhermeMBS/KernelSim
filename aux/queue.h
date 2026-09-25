#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>


typedef struct
{
    int* buffer;
    int start;
    int end;
    int size;
} queue_t;


#define QUEUE_INIT(name, size)          \
do {                                    \
    int buffer[size];                   \
    queue_t name = {                    \
        .buffer = name,                 \
        .start = 0,                     \
        .end = 0,                       \
        .size = size                    \
    }                                   \
} while(0);                             \


void
queue_put(queue_t q, int e);

int
queue_get(queue_t q);

#endif