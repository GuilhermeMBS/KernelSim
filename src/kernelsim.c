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
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "include/kernelsim.h"
#include "include/intercontroller.h"
#include "include/child.h"
#include "aux/kernelsim_params.h"
#include "aux/pcb.h"
#include "aux/queue.h"
#include "aux/retcode.h"

#define READ  0
#define WRITE 1
#define BROTHER_PIPES(id) brother_p##id

static pcb_child_t children[NUM_CHILDREN];
static pcb_controller_t controller;
static int curr_child;

QUEUE_INIT(children_ready, NUM_CHILDREN);
QUEUE_INIT(children_wsend, NUM_CHILDREN);
QUEUE_INIT(children_wrecv, NUM_CHILDREN);
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
    for (int i = 0; i < NUM_CHILDREN; i++) pipe_make(&(children[i].child));
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


static void
_kernelsim_handle_iqr(int signal)
{
    switch (signal) {
        case INTERCONTROLLER_SIG_IQR0:
            kill(children[curr_child].pid, SIGSTOP);
            queue_put(&children_ready, curr_child);

            int curr_child = queue_get(&children_ready);
            if (curr_child == -1) return;
            kill(children[curr_child].pid, SIGCONT);

            break;

        case INTERCONTROLLER_SIG_IQR1:
            int child_to_move = queue_get(&children_wsend);
            if (child_to_move != -1) queue_put(&children_ready, child_to_move);

            break;
        
        case INTERCONTROLLER_SIG_IQR2:
            int child_to_move = queue_get(&children_wrecv);
            if (child_to_move != -1) queue_put(&children_ready, child_to_move);

            break;

        case INTERCONTROLLER_SIG_ERROR:
            exit(10);

        default:
            puts("[Undefined Intercontroller Signal]");
    }
}


static void
_kernelsim_handle_syscall(int signal)
{
    switch (signal) {
        case CHILD_OP_WRITE:
            kill(children[curr_child].pid, SIGSTOP);
            queue_put(&children_wsend, curr_child);

            break;

        case CHILD_OP_READ:
            kill(children[curr_child].pid, SIGSTOP);
            queue_put(&children_wrecv, curr_child);

            break;
        default:
            puts("[Undefined Child OP Signal]");
    }
}


inline void
kernelsim_pause()
{
    kill(controller.pid, SIGSTOP);
    kill(children[curr_child].pid, SIGSTOP);
}


inline void
kernelsim_resume()
{
    kill(controller.pid, SIGCONT);
    kill(children[curr_child].pid, SIGCONT);
}


static void
_kernelsim_run()
{
    _kernel_resume();
    
    #define POLL_SIZE (NUM_CHILDREN + 1) // Children + Intercontroller
    #define POLL_IC_IDX (POLL_SIZE - 1)  // Intercontroller Index

    struct pollfd fds[POLL_SIZE];

    // Intercontroller
    fds[POLL_IC_IDX].fd = controller.child.from[READ];
    fds[POLL_IC_IDX].events = POLLIN;

    // Children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        fds[i].fd = children[i].child.from[READ];
        fds[i].events = POLLIN;
    }

    int activity = poll(fds, POLL_SIZE, -1);

    if (activity < 0) {
        perror("[POLL ERROR]");
        exit(3);
    }

    for (int i = 0; i < POLL_SIZE; i++) {
        if (fds[i].revents & POLLIN) {
            int signal;
            int bytes_read = read(fds[i].fd, &signal, 4);
            
            if (bytes_read > 0) {
                // Intercontroller signal
                if (i == POLL_IC_IDX) {
                    printf("[INTERCONTROLLER] IQR%d\n", signal);
                    _kernelsim_handle_iqr(signal);
                }

                // Child syscall
                else {
                    printf("[Child %d] Syscall OP%d\n", i, signal);
                    _kernelsim_handle_syscall(signal);
                }
            }

            else if (bytes_read == 0) printf("[SIGNAL | P%d] Closed Pipe.\n", i);
        }
    }

    #undef POLL_SIZE
    #undef POLL_IC_IDX (POLL_SIZE - 1)
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
            children[i].state   = PCB_STATE_READY;

            // Close Child Unused Pipe Ends
            close(children[i].child.to[READ]);
            close(children[i].child.from[WRITE]);

            // Add to Ready Queue
            queue_put(&children_ready, i);
        }

        else if (pid == 0) {
            // Close Unused Pipe Ends
            close(children[i].child.to[WRITE]);
            close(children[i].child.from[READ]);

            char read_fd_str[16], write_fd_str[16], id_str[16];
            snprintf(read_fd_str, sizeof(read_fd_str), "%d", children[i].child.to[READ]);
            snprintf(write_fd_str, sizeof(write_fd_str), "%d", children[i].child.from[WRITE]);
            snprintf(id_str, sizeof(id_str), "%d", i);

            // execl("./bin/child", "child", id_str, read_fd_str, write_fd_str, NULL);
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


void
kernelsim_init() 
{
    // Build Pipes
    _kernelsim_build_child_pipes();
    _kernelsim_build_controller_pipes();

    // Create Processes
    _kernelsim_exec_child();
    _kernelsim_exec_controller();
}


void 
kernelsim_start() 
{
    // Function to show initial processes states AND FLAGS

    pritnf("Starting Child: %d", queue_get(curr_child));
    _kernelsim_run();
}


void 
kernelsim_state() 
{
    // Shows all states
}


#undef WRITE
#undef READ
