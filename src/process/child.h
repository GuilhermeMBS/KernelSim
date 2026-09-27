#pragma once

#ifndef CHILD_H
#define CHILD_H

typedef enum
{
    CHILD_OP_WRITE = 1,
    CHILD_OP_READ
} ChildOp;

typedef struct
{
    int pc;
    int n;
} child_data_t;

#endif
