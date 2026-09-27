#ifndef PCB_H
#define PCB_H

#include <signal.h>

#include "ipc/pipes.h"
#include "process/child.h"
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
    int nread;          // Number of Read Syscalls
    int nwrite;         // Number of Write Syscalls
} pcb_data_t;

typedef struct
{
    pipe_t child;       // Child Pipes (from and to)
    queue_t* brother;   // Brother Queue (fake pipe)
    pcb_data_t data;    // Data stored in syscall
    child_data_t ctx;   // Process Context
    pid_t pid;          // Process PID
    PcbState state;     // Process Current State
} pcb_child_t;

typedef struct
{
    pipe_t child;
    pid_t pid;
} pcb_controller_t;

#endif