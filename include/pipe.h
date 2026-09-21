#ifndef PIPE_H
#define PIPE_H

#include "retcode.h"


typedef struct
{
    int to[2];   // P1 --> P2 (forked)
    int from[2]; // P2 (forked) --> P1
} pipe_t;


retcode_t pipe_make(pipe_t* p);
// Talvez fazer função auxiliar de close?

#endif