/**
 * This file is the Kernel in our simulation. It it forks the
 * Inter Controller to give the signals described in the spec
 * as IQR0, IQR1 and IQR2.
 * After that, it forks the processes that will run in
 * parallel, bounded two by two with pipes, also created by
 * our "fake" Kernel.
*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>

#include "kernelsim.h"
#include "pcb.h"
#include "ipc/intercontroller.h"
#include "process/child.h"
#include "utils/queue.h"
#include "utils/debug.h"

#define READ  0
#define WRITE 1
#define BROTHER_PIPES(id) brother_p##id
#define BROTHER_IDX(id) ((id + 1) - 2*(id % 2))

static pcb_child_t children[NUM_CHILDREN];
static pcb_controller_t controller;
static int curr_child;
static bool running = false;
static bool context_triggered = false;

QUEUE_INIT(controller_sig, 3);
QUEUE_INIT(children_ready, NUM_CHILDREN);
QUEUE_INIT(children_recv, NUM_CHILDREN);
QUEUE_INIT(children_send, NUM_CHILDREN);

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


static void handle_sig(int signal)
{
    context_triggered = true;
}


static inline DebugRet
_kernelsim_build_child_pipes()
{
    printf("Building %d Children Pipes...\n", NUM_CHILDREN);
    for (int i = 0; i < NUM_CHILDREN; i++) pipe_make(&(children[i].child));
    puts("Children Pipes Ready.");

    return DEBUG_RET_SUCCESS;
}


static inline DebugRet
_kernelsim_build_controller_pipes()
{
    puts("Building Intercontroller Pipes...");
    pipe_make(&controller.child);
    puts("Intercontroller Pipes Ready.");

    return DEBUG_RET_SUCCESS;
}


static void
_kernelsim_handle_iqr(IntercontrollerSig signal)
{
    switch (signal) {
        case INTERCONTROLLER_SIG_IRQ0:
            kill(children[curr_child].pid, SIGSTOP);
            queue_put(&children_ready, curr_child);
            int curr_child = queue_get(&children_ready);
            if (curr_child == DEBUG_RET_EMPTY_QUEUE) return;
            kill(children[curr_child].pid, SIGCONT);
            break;

        // Write
        case INTERCONTROLLER_SIG_IRQ1:
            int child_to_move = queue_get(&children_recv);
            if (child_to_move != DEBUG_RET_EMPTY_QUEUE) {
                queue_put(&children_ready, child_to_move);
                write(children[child_to_move].child.to[WRITE],
                    children[child_to_move].brother,
                    sizeof(int)
                );
            }
            break;
        
        // Read
        case INTERCONTROLLER_SIG_IRQ2:
            int child_to_move = queue_get(&children_send);
            if (child_to_move != DEBUG_RET_EMPTY_QUEUE) {
                queue_put(&children_ready, child_to_move);
                queue_put(
                    children[BROTHER_IDX(child_to_move)].brother,
                    children[child_to_move].data
                );
            }
            break;

        case INTERCONTROLLER_SIG_ERROR:
            exit(10);

        default:
            puts("[Undefined Intercontroller Signal]");
    }
}


static void
_kernelsim_handle_syscall(child_data_t data)
{
    switch (data.op) {
        case CHILD_OP_WRITE:
            kill(children[curr_child].pid, SIGSTOP);
            queue_put(&children_send, curr_child);
            children[curr_child].data = data.pc;
            break;

        case CHILD_OP_READ:
            kill(children[curr_child].pid, SIGSTOP);
            queue_put(&children_recv, curr_child);
            break;

        default:
            puts("[Undefined Child OP Signal]");
    }
}


inline void
_kernelsim_pause()
{
    kill(controller.pid, SIGSTOP);
    kill(children[curr_child].pid, SIGSTOP);
}


inline void
_kernelsim_resume()
{
    kill(controller.pid, SIGCONT);
    kill(children[curr_child].pid, SIGCONT);
}


static void
_kernelsim_engine()
{    
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
        if (errno == EINTR) return; // Ignores the CTRL-Z Signal
        perror("[POLL ERROR]");
        exit(3);
    }

    // Intercontroller Signal
    int signal;
    int bytes_read = read(fds[POLL_IC_IDX].fd, &signal, sizeof(int));
    if (fds[POLL_IC_IDX].revents & POLLIN) {
        if (bytes_read > 0) {
            printf("[INTERCONTROLLER] IQR%d\n", signal);
            _kernelsim_handle_iqr(signal);
        }
        else if (bytes_read == 0) puts("[INTERCONTROLLER] Closed Pipe!");
    }

    // Children Signal
    child_data_t data;
    int bytes_read = read(fds[curr_child].fd, &data, sizeof(child_data_t));
    if (fds[curr_child].revents & POLLIN) {
        if (bytes_read > 0) {
            printf("[Child %d] Syscall OP%d\n", curr_child, data);
            _kernelsim_handle_syscall(data);
        }
        else if (bytes_read == 0) printf("[SIGNAL | P%d] Closed Pipe.\n", curr_child);
    }

    #undef POLL_SIZE
    #undef POLL_IC_IDX (POLL_SIZE - 1)
}


static DebugRet
_kernelsim_exec_child()
{
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid > 0) {
            // Set PCB Struct
            children[i].brother = brother_pipes[i];
            children[i].pid     = pid;
            children[i].data    = 0;
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
            exit(DEBUG_RET_FORK_ERROR);
        }
    }

    return DEBUG_RET_SUCCESS;
}


static DebugRet
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
        exit(DEBUG_RET_EXEC_ERROR);
    }

    else {
        printf("[PID %d] Controller Fork Error!\n", getpid());
        exit(DEBUG_RET_FORK_ERROR);
    }

    return DEBUG_RET_SUCCESS;
}


void
kernelsim_init() 
{
    if (signal(SIGTSTP, handle_sig) == SIG_ERR) {
        perror("[Error Registering Signal Handler]");
        exit(1);
    }

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

    printf("[Starting Child: %d]", queue_get(curr_child));
    _kernelsim_resume();
    running = true;
    
    while (true) {
        // Checks CTRL-Z Signal
        if (context_triggered) {
            #if DEBUG
            if (running) {
                _kernelsim_pause();
                puts("[Kernel Simulation Paused]");
                running = false;

                _kernelsim_state();
                puts("[Press CTRL-Z Again to Resume Simulation]");
            }

            else {
                puts("[Resuming Kernel Simulation...]");
                _kernelsim_resume();

                running = true;
            }
            #else
            kernelsim_state();
            #endif

            context_triggered = false;
        }

        _kernelsim_engine();
    }
}


void 
_kernelsim_state() 
{
    // Shows all states
}


#undef WRITE
#undef READ
