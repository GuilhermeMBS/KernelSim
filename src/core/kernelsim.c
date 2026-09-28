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

static int curr_child = DEBUG_RET_EMPTY_QUEUE;          // Current Running Child
static bool running = false;                            // Process Running (for DEBUG)
static bool context_triggered = false;                  // CTRL-Z Flag

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


static void
_handle_sig(int sig)
{
    if (sig == SIGTSTP) {
        context_triggered = true;
        signal(SIGTSTP, _handle_sig); // Reset signal handler
    }
    // Other signals should kill all processes as well as the kernelsim
    else {
        for (int i = 0; i < NUM_CHILDREN; i++) kill(children[i].pid, SIGKILL);
        kill(controller.pid, SIGKILL);
        exit(0);
    }
}


void
_kernelsim_state(void)
{
    printf("[CTRL-Z SIGNAL RECIEVED] Current Child: %d\n", curr_child);
    debug_print_table_header();

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pcb_child_t child = children[i];

        // Must follow the order and use pcb_state_strings
        printf("%-*d%-*d%-*d%-*d%-*s%-*d%-*d\n",
            DEBUG_COL_WIDTH_CHILD,          i,
            DEBUG_COL_WIDTH_PID,            child.pid,
            DEBUG_COL_WIDTH_PC,             child.ctx.pc,
            DEBUG_COL_WIDTH_N,              child.ctx.n,
            DEBUG_COL_WIDTH_STATE,          pcb_state_strings[child.state],
            DEBUG_COL_WIDTH_READ_SYSCALLS,  child.data.nread,
            DEBUG_COL_WIDTH_WRITE_SYSCALLS, child.data.nwrite
        );
    }

    debug_print_table_separator();
}


static inline DebugRet
_kernelsim_build_child_pipes(void)
{
    printf("Building %d Children Pipes...\n", NUM_CHILDREN);
    for (int i = 0; i < NUM_CHILDREN; i++) pipe_make(&(children[i].child));
    puts("Children Pipes Ready.");

    return DEBUG_RET_SUCCESS;
}


static inline DebugRet
_kernelsim_build_controller_pipes(void)
{
    puts("Building Intercontroller Pipes...");
    pipe_make(&controller.child);
    puts("Intercontroller Pipes Ready.");

    return DEBUG_RET_SUCCESS;
}


static inline DebugRet
_kernelsim_alloc_shm(void)
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
_kernelsim_save_ctx(void)
{
    children[curr_child].ctx.pc = (*shared_context[curr_child]).pc;
    children[curr_child].ctx.n = (*shared_context[curr_child]).n;

    return DEBUG_RET_SUCCESS;
}


static void
_kernelsim_handle_irq(IntercontrollerSig signal)
{
    switch (signal) {
        case INTERCONTROLLER_SIG_IRQ0:
        {
            if (curr_child != DEBUG_RET_EMPTY_QUEUE
                && children[curr_child].state == PCB_STATE_RUNNING) {
                kill(children[curr_child].pid, SIGSTOP);
                _kernelsim_save_ctx();
                children[curr_child].state = PCB_STATE_READY;
                queue_put(&children_ready, curr_child);
            }
            
            curr_child = queue_get(&children_ready);
            
            if (curr_child != DEBUG_RET_EMPTY_QUEUE) {
                children[curr_child].state = PCB_STATE_RUNNING;
                kill(children[curr_child].pid, SIGCONT);
            }
            break;
        }

        case INTERCONTROLLER_SIG_IRQ1:
        {
            int child_to_move = queue_get(&children_recv);

            if (child_to_move != DEBUG_RET_EMPTY_QUEUE) {
                int partner_pc = queue_get(children[BROTHER_IDX(child_to_move)].brother);
                if (partner_pc == DEBUG_RET_EMPTY_QUEUE) partner_pc = 0; // NO_WAIT

                write(children[child_to_move].child.to[WRITE], &partner_pc, sizeof(partner_pc));

                children[child_to_move].state = PCB_STATE_READY;
                queue_put(&children_ready, child_to_move);
            }
            break;
        }
        
        case INTERCONTROLLER_SIG_IRQ2:
        {
            int child_to_move = queue_get(&children_send);
            if (child_to_move != DEBUG_RET_EMPTY_QUEUE) {
                queue_put(
                    children[child_to_move].brother, 
                    (*shared_context[child_to_move]).pc
                );

                children[child_to_move].state = PCB_STATE_READY;
                queue_put(&children_ready, child_to_move);
            }
            break;
        }

        case INTERCONTROLLER_SIG_ERROR: exit(10);

        default: puts("[Undefined Intercontroller Signal]");
    }
}


static void
_kernelsim_handle_syscall(ChildOp op)
{
    switch (op) {
        case CHILD_OP_WRITE:
        {
            kill(children[curr_child].pid, SIGSTOP);
            _kernelsim_save_ctx();
            queue_put(&children_send, curr_child);
            children[curr_child].data.nwrite++;
            children[curr_child].state = PCB_STATE_WAIT_SEND;
            break;
        }

        case CHILD_OP_READ:
        {
            kill(children[curr_child].pid, SIGSTOP);
            _kernelsim_save_ctx();
            queue_put(&children_recv, curr_child);
            children[curr_child].data.nread++;
            children[curr_child].state = PCB_STATE_WAIT_RECV;
            break;
        }

        default: puts("[Undefined Child OP Signal]");
    }

    // Don't lose a cycle
    curr_child = queue_get(&children_ready);
    if (curr_child != DEBUG_RET_EMPTY_QUEUE) {
        children[curr_child].state = PCB_STATE_RUNNING;
        kill(children[curr_child].pid, SIGCONT);
    }
}


static inline void
_kernelsim_pause(void)
{
    kill(controller.pid, SIGSTOP);
    if (curr_child != DEBUG_RET_EMPTY_QUEUE) kill(children[curr_child].pid, SIGSTOP);
}


static inline void
_kernelsim_resume(void)
{
    kill(controller.pid, SIGCONT);
    if (curr_child != DEBUG_RET_EMPTY_QUEUE) kill(children[curr_child].pid, SIGCONT);
}


static DebugRet
_kernelsim_exec_child(void)
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
            setpgid(0, 0); // Isolate from signals CTRL-Z and CTRL-C from terminal
            
            // Close Unused Pipe Ends
            close(children[i].child.to[WRITE]);
            close(children[i].child.from[READ]);

            // Close Brother Pipe Ends 
            for (int j = 0; j < i; j++) {
                close(children[j].child.to[READ]);
                close(children[j].child.from[WRITE]);
            }

            char read_fd_str[16], write_fd_str[16];
            snprintf(read_fd_str, sizeof(read_fd_str), "%d", children[i].child.to[READ]);
            snprintf(write_fd_str, sizeof(write_fd_str), "%d", children[i].child.from[WRITE]);

            execl("./bin/child", "child", read_fd_str, write_fd_str, NULL);
            perror("[EXEC ERROR] Child");
            exit(DEBUG_RET_EXEC_ERROR);
        }

        else {
            printf("[PID %d] Child %d Fork Error!\n", pid, i);
            exit(DEBUG_RET_FORK_ERROR);
        }
    }

    return DEBUG_RET_SUCCESS;
}


static DebugRet
_kernelsim_exec_controller(void)
{
    pid_t pid = fork();

    if (pid > 0) {
        setpgid(0, 0); // Isolate from signals CTRL-Z and CTRL-C from terminal
        controller.pid = pid;

        // Close Unused Pipe Ends
        close(controller.child.to[READ]);
        close(controller.child.from[WRITE]);
    }

    else if (pid == 0) {
        // Redirects the Pipes to STDIN and STDOUT
        dup2(controller.child.to[READ], STDIN_FILENO);
        dup2(controller.child.from[WRITE], STDOUT_FILENO);

        // Close Unused Pipe Ends
        close(controller.child.to[WRITE]);
        close(controller.child.from[READ]);

        execl("./bin/intercontroller", "controller", NULL);

        perror("[Intercontroller] Exec Failed!");
        exit(DEBUG_RET_EXEC_ERROR);
    }

    else {
        printf("[PID %d] Controller Fork Error!\n", getpid());
        exit(DEBUG_RET_FORK_ERROR);
    }

    return DEBUG_RET_SUCCESS;
}


static void
_kernelsim_engine(void)
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
        IntercontrollerSig signal;
        int bytes_read = read(fds[POLL_IC_IDX].fd, &signal, sizeof(IntercontrollerSig));
        
        if (bytes_read > 0) {
            printf("[INTERCONTROLLER] IRQ%d\n", signal);
            _kernelsim_handle_irq(signal);
        }
        else if (bytes_read == 0) puts("[INTERCONTROLLER] Closed Pipe!");
    }

    // Children Signal
    if (curr_child != DEBUG_RET_EMPTY_QUEUE && (fds[curr_child].revents & POLLIN)) {
        ChildOp op;
        int bytes_read = read(fds[curr_child].fd, &op, sizeof(ChildOp));
        
        if (bytes_read > 0) {
            if (children[curr_child].state == PCB_STATE_RUNNING) {
                printf("[Child %d] Syscall OP%d\n", curr_child, op);
                _kernelsim_handle_syscall(op);
            }
        }
        else if (bytes_read == 0) printf("[SIGNAL | P%d] Closed Pipe.\n", curr_child);
    }

    #undef POLL_SIZE
    #undef POLL_IC_IDX
}


void
kernelsim_init(void) 
{
    // Set Handlers
    if (signal(SIGTSTP, _handle_sig) == SIG_ERR) {
        perror("[Error Registering CTRL-Z Signal Handler]");
        exit(1);
    }

    if (signal(SIGINT, _handle_sig) == SIG_ERR) {
        perror("[Error Registering CTRL-C Signal Handler]");
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
kernelsim_start(void)
{
    running = true;
    // Function to show initial processes states AND FLAGS

    curr_child = queue_get(&children_ready);
    printf("[Starting Child: %d]\n", curr_child);
    if (curr_child != DEBUG_RET_EMPTY_QUEUE) {
        children[curr_child].state = PCB_STATE_RUNNING;
        kill(children[curr_child].pid, SIGCONT);
    }
    
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
                running = true;
                _kernelsim_resume();
            }
            #else
            _kernelsim_state();
            #endif

            context_triggered = false;
        }

        _kernelsim_engine();
    }
}


#undef WRITE
#undef READ
