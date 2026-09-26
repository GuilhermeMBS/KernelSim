#ifndef KERNELSIM_PARAMS_H
#define KERNELSIM_PARAMS_H

#define NUM_CHILDREN 6
#define BROTHER_PIPE_SIZE 1024

// This list should have the size of NUM_CHILDREN
#define CHILDREN_LIST   \
    X(0)                \
    X(1)                \
    X(2)                \
    X(3)                \
    X(4)                \
    X(5)

#endif