#ifndef CHILD_H
#define CHILD_H

typedef enum
{
    CHILD_OP_WRITE = 1,
    CHILD_OP_READ
} child_op;

typedef struct
{
    int PC;
    child_op OP;
} child_data_t;

#endif
