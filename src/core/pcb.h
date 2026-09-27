#ifndef PCB_H
#define PCB_H

#include <signal.h>

#include "ipc/pipes.h"
#include "utils/debug.h"
#include "utils/queue.h"

#define NUM_CHILDREN 6
#define BROTHER_PIPE_SIZE 16

// This list should have the size of NUM_CHILDREN
#define CHILDREN_LIST   \
    X(0)                \
    X(1)                \
    X(2)                \
    X(3)                \
    X(4)                \
    X(5)


typedef enum
{
    PCB_STATE_READY = 0,
    PCB_STATE_WAIT_RECV,
    PCB_STATE_WAIT_SEND,
    PCB_RUNNING,
    PCB_STATE_DONE
} PcbState;

typedef struct
{
    queue_t* brother;
    pipe_t child;
    pid_t pid;
    int state;
    int data;           // Data stored in syscall
} pcb_child_t;

typedef struct
{
    pipe_t child;
    pid_t pid;
} pcb_controller_t;

#endif