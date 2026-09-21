#ifndef PCB_H
#define PCB_H

#include <signal.h>

#include "pipe.h"


typedef enum
{
    READY = 0,
    WAIT,
    DONE
} pcb_state;

typedef struct
{
    pipe_t child;
    pipe_t brother;
    pid_t pid;
    int state;
    int time; // Remaining time
} pcb_child_t;

typedef struct
{
    pipe_t child;
    pid_t pid;
} pcb_controller_t;

#endif