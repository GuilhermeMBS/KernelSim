#ifndef PCB_H
#define PCB_H

#include <signal.h>

#include "pipe.h"
#include "queue.h"


typedef enum
{
    PCB_STATE_READY = 0,
    PCB_STATE_WAIT,
    PCB_STATE_DONE
} PcbState;

typedef struct
{
    queue_t* brother;
    pipe_t child;
    pid_t pid;
    int state;
    int time;           // Remaining time
} pcb_child_t;

typedef struct
{
    pipe_t child;
    pid_t pid;
} pcb_controller_t;

#endif