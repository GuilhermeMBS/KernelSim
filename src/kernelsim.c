/**
 * This file is the Kernel in our simulation. It it forks the
 * Inter Controller to give the signals described in the spec
 * as IQR0, IQR1 and IQR2.
 * After that, it forks the processes that will run in
 * parallel, bounded two by two with pipes, also created by
 * our "fake" Kernel.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "include/intercontroller.h"
#include "include/kernelsim.h"
#include "aux/kernelsim_params.h"
#include "aux/retcode.h"
#include "aux/pcb.h"
#include "aux/queue.h"

#define READ  0
#define WRITE 1
#define BROTHER_PIPES(id) brother_p##id


static pcb_child_t children[NUM_CHILDREN];
static pcb_controller_t controller;

QUEUE_INIT(waiting_send, NUM_CHILDREN);
QUEUE_INIT(waiting_recieve, NUM_CHILDREN);
QUEUE_INIT(controller_sig, 3);

// Allocate brother pipes
#define X(id) QUEUE_INIT(BROTHER_PIPES(id), BROTHER_PIPE_SIZE);
CHILDREN_LIST
#undef X

// Array of brother pipes
static queue_t *brother_pipes[] = {
#define X(id) &BROTHER_PIPES(id),
    CHILDREN_LIST
#undef X
};

static retcode_t
_kernelsim_build_child_pipes()
{
    printf("Building %d Children Pipes...\n", NUM_CHILDREN);
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pipe_make(&(children[i].child));
    }
    puts("Children Pipes Ready.");

    return SUCCESS;
}


static retcode_t
_kernelsim_build_controller_pipes()
{
    puts("Building Intercontroller Pipes...");
    pipe_make(&controller.child);
    puts("Intercontroller Pipes Ready.");

    return SUCCESS;
}


static retcode_t
_kernelsim_exec_child()
{
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid > 0) {
            // Set PCB Struct
            children[i].brother = brother_pipes[i];
            children[i].pid     = pid;
            children[i].time    = 0;
            children[i].state   = PCB_STATE_WAIT;

            // Close Child Unused Pipe Ends
            close(children[i].child.to[READ]);
            close(children[i].child.from[WRITE]);
        }

        else if (pid == 0) {
            // Close Unused Pipe Ends
            close(children[i].child.to[WRITE]);
            close(children[i].child.from[READ]);

            char read_fd_str[16], write_fd_str[16], id_str[16];
            snprintf(read_fd_str, sizeof(read_fd_str), "%d", children[i].child.to[READ]);
            snprintf(write_fd_str, sizeof(write_fd_str), "%d", children[i].child.from[WRITE]);
            snprintf(id_str, sizeof(id_str), "%d", i);

            execl("./bin/child", "child", id_str, read_fd_str, write_fd_str, NULL);
        }

        else {
            printf("[PID %d] Child %d Fork Error!\n", pid, i);
            exit(FORK_ERROR);
        }
    }

    return SUCCESS;
}


static retcode_t
_kernelsim_exec_controller()
{
    pid_t pid = fork();

    if (pid > 0) {
        controller.pid = pid;

        // Close Unused Pipe Ends
        close(controller.child.to[READ]);
        close(controller.child.from[WRITE]);
    }

    else if (pid == 0) {
        // Redirects the Read End of (Kernel --> Controller) to STDOUT
        dup2(controller.child.to[READ], STDIN_FILENO);

        // Redirects the Write End of (Controller --> Kernel) to STDIN
        dup2(controller.child.from[WRITE], STDOUT_FILENO);

        // Close Unused Pipe Ends
        close(controller.child.to[WRITE]);
        close(controller.child.from[READ]);

        // Replace process image with the child binary
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d");

        execl("./bin/intercontroller", "controller", id_str, NULL);

        perror("execl failed");
        exit(EXEC_ERROR);
    }

    else {
        printf("[PID %d] Controller Fork Error!\n", getpid());
        exit(FORK_ERROR);
    }

    return SUCCESS;
}


static inline void
_kernelsim_pause_controller()
{
    kill(controller.pid, SIGSTOP);

}


static retcode_t
_kernelsim_pause_children()
{
    for(int i = 0; i < NUM_CHILDREN; i++) {
        // if State = Running --> Pause (or new state?)
        // save index in curr or next?
        kill(children[i].pid, SIGSTOP);
    }

    return SUCCESS;
}


static inline void
_kernelsim_resume_controller()
{
    kill(controller.pid, SIGCONT);
}


static retcode_t
_kernelsim_resume_children()
{
    for(int i = 0; i < NUM_CHILDREN; i++) {
        // if State = Running --> Pause
        kill(children[i].pid, SIGSTOP);
    }

    return SUCCESS;
}


void
kernelsim_init() 
{
    // Build Pipes
    _kernelsim_build_child_pipes();
    _kernelsim_build_controller_pipes();

    // Create Processes
    _kernelsim_exec_child();
    _kernelsim_exec_controller();

    // Check errors to exit (here or in functions?)
}


void 
kernelsim_start() 
{
    // Function to show initial processes states AND FLAGS
    _kernelsim_resume_controller();
    _kernelsim_resume_children();
}


void 
kernelsim_state() 
{
    // Shows all states
}

void
kernelsim_pause()
{
    _kernelsim_pause_controller();
    _kernelsim_pause_children();
}

void
kernelsim_resume()
{
    _kernelsim_resume_controller();
    _kernelsim_resume_children();
}

#undef WRITE
#undef READ
