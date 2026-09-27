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
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "kernelsim.h"
#include "pcb.h"
#include "ipc/intercontroller.h"
#include "process/child.h"
#include "utils/queue.h"
#include "utils/debug.h"

#define READ  0
#define WRITE 1
#define BROTHER_PIPES(id)   brother_p##id
#define BROTHER_IDX(id)     ((id + 1) - 2*(id % 2))

static pcb_child_t children[NUM_CHILDREN];              // Children PCB Array
static pcb_controller_t controller;                     // Intercontroller Pipe
static child_data_t *shared_context[NUM_CHILDREN];      // Context Pointers
static int shmids[NUM_CHILDREN];                        // Shared Memory ID

static int curr_child;                                  // Current Running Child
static bool running = false;                            // Process Running (for DEBUG)
static bool context_triggered = false;                  // CTRL-Z Flag


QUEUE_INIT(controller_sig, 3);                          // Queue for IRQs Recieved
QUEUE_INIT(children_ready, NUM_CHILDREN);               // Queue for Ready Processes
QUEUE_INIT(children_recv, NUM_CHILDREN);                // Queue for Waiting Recv Syscall
QUEUE_INIT(children_send, NUM_CHILDREN);                // Queue for Waiting Send Syscall

// Allocate brother pipes (Kernel fake pipes)
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


void
_kernelsim_state()
{
    print_table_header();

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pcb_child_t child = children[i];

        printf("%-*d %-*d %-*d %-*d %-*s %-*d %-*d\n",
            DEBUG_COL_WIDTH_CHILD,          i,
            DEBUG_COL_WIDTH_PID,            child.pid,
            DEBUG_COL_WIDTH_PC,             child.ctx.pc,
            DEBUG_COL_WIDTH_N,              child.ctx.n,
            DEBUG_COL_WIDTH_STATE,          child.state,
            DEBUG_COL_WIDTH_READ_SYSCALLS,  child.data.nread,
            DEBUG_COL_WIDTH_WRITE_SYSCALLS, child.data.nwrite
        );
    }

    print_table_separator();
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


static inline DebugRet
_kernelsim_alloc_shm()
{
    printf("Allocating %d shared memories...\n", NUM_CHILDREN);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        shmids[i] = shmget(IPC_PRIVATE, sizeof(child_data_t), IPC_CREAT | 0666);
        if (shmids[i] < 0) {
            perror("[SHM] Shmget Error");
            exit(EXIT_FAILURE);
        }

        shared_context[i] = (child_data_t *)shmat(shmids[i], NULL, 0);
        if (shared_context[i] == (void *)-1) {
            perror("[SHM] Shmat Error");
            exit(EXIT_FAILURE);
        }

        shared_context[i]->n = shared_context[i]->pc = 0;
    }

    puts("Shared Memories Ready.");
    return DEBUG_RET_SUCCESS;
}


static inline DebugRet
_kernelsim_save_ctx()
{
    children[curr_child].ctx.pc = (*shared_context[curr_child]).pc;
    children[curr_child].ctx.n = (*shared_context[curr_child]).n;

    return DEBUG_RET_SUCCESS;
}


static void
_kernelsim_handle_iqr(IntercontrollerSig signal)
{
    switch (signal) {
        case INTERCONTROLLER_SIG_IRQ0:
            // Stop child
            kill(children[curr_child].pid, SIGSTOP);
            _kernelsim_save_ctx();
            children[curr_child].state = PCB_STATE_READY;
            queue_put(&children_ready, curr_child);
            // Get next process
            int curr_child = queue_get(&children_ready);
            if (curr_child == DEBUG_RET_EMPTY_QUEUE) return;
            kill(children[curr_child].pid, SIGCONT);
            break;

        // Write
        case INTERCONTROLLER_SIG_IRQ1:
            int child_to_move = queue_get(&children_recv);
            if (child_to_move != DEBUG_RET_EMPTY_QUEUE) {
                queue_put(&children_ready, child_to_move);
                /*
                Old Write in Pipe Method (Before SHM)
                children[child_to_move].state = PCB_STATE_READY;
                write(children[child_to_move].child.to[WRITE],
                    children[child_to_move].brother,
                    sizeof(int)
                );
                */
               (*shared_context[child_to_move]).n = (*shared_context
                                                    [BROTHER_IDX(child_to_move)]
                                                    ).pc;
            }
            break;
        
        // Read
        case INTERCONTROLLER_SIG_IRQ2:
            int child_to_move = queue_get(&children_send);
            if (child_to_move != DEBUG_RET_EMPTY_QUEUE) {
                queue_put(&children_ready, child_to_move);
                queue_put(
                    children[BROTHER_IDX(child_to_move)].brother,
                    (*shared_context[child_to_move]).pc
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
_kernelsim_handle_syscall(ChildOp op)
{
    switch (op) {
        case CHILD_OP_WRITE:
            kill(children[curr_child].pid, SIGSTOP);
            _kernelsim_save_ctx();
            queue_put(&children_send, curr_child);
            children[curr_child].data.nwrite++;
            break;

        case CHILD_OP_READ:
            kill(children[curr_child].pid, SIGSTOP);
            _kernelsim_save_ctx();
            queue_put(&children_recv, curr_child);
            children[curr_child].data.nread++;
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
    if (fds[POLL_IC_IDX].revents & POLLIN) {
        char signal_char;
        // Lê exatamente 1 byte, como enviado pelo printf("%d") do Intercontroller
        int bytes_read = read(fds[POLL_IC_IDX].fd, &signal_char, 1);
        
        if (bytes_read > 0) {
            // Converte o char ASCII de volta para um valor inteiro (0, 1 ou 2)
            int signal = signal_char - '0';
            printf("[INTERCONTROLLER] IRQ%d\n", signal);
            _kernelsim_handle_irq(signal);
        }
        else if (bytes_read == 0) {
            puts("[INTERCONTROLLER] Closed Pipe!");
        }
    }

    // Children Signal
    // Assumindo que curr_child foi definido corretamente antes desta verificação
    if (fds[curr_child].revents & POLLIN) {
        ChildOp op;
        // Aqui sizeof(ChildOp) faz sentido SE a aplicação filha enviou a struct crua via write()
        int bytes_read = read(fds[curr_child].fd, &op, sizeof(ChildOp));
        
        if (bytes_read > 0) {
            printf("[Child %d] Syscall OP%d\n", curr_child, op);
            _kernelsim_handle_syscall(op);
        }
        else if (bytes_read == 0) {
            printf("[SIGNAL | P%d] Closed Pipe.\n", curr_child);
        }
    }

    #undef POLL_SIZE
    #undef POLL_IC_IDX
}


static DebugRet
_kernelsim_exec_child()
{
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid > 0) {
            // Set PCB Struct
            children[i].brother         = brother_pipes[i];
            children[i].pid             = pid;
            children[i].state           = PCB_STATE_READY;
            children[i].data.nread      = 0;
            children[i].data.nwrite     = 0;
            children[i].ctx.n           = 0;
            children[i].ctx.pc          = 0;

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
            exit(0);
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

        perror("[Intercontroller] Exec Failed!");
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
    // Set Handler
    if (signal(SIGTSTP, handle_sig) == SIG_ERR) {
        perror("[Error Registering Signal Handler]");
        exit(1);
    }

    // Allocate Shared Memories
    _kernelsim_alloc_shm();

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


#undef WRITE
#undef READ
