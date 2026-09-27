#pragma once

#ifndef PIPES_H
#define PIPES_H

#include "utils/debug.h"


typedef struct
{
    int to[2];   // P1 --> P2 (forked)
    int from[2]; // P2 (forked) --> P1
} pipe_t;


DebugRet
pipe_make(pipe_t* p);

// Talvez fazer função auxiliar de close?

#endif