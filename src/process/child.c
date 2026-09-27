/**
 * @file child.c
 * @brief Application process implementation.
 *
 * This process simulates a user application running under a time-sharing 
 * system managed by KernelSim. It performs a random number of iterations 
 * and probabilistically requests IPC read/write operations, relying on 
 * blocking pipe reads to simulate the suspension of system calls.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "process/child.h"
#include "utils/debug.h"

#define MAX_ITER        10000   // Maximum number of loop iterations
#define MIN_ITER        5000    // Minimum number of loop iterations
#define SLEEP_TIME      500     // Half-cycle sleep time in ms
#define SYSCALL_PROB    15      // Low probability of a syscall in percentage

static int read_pipe;
static int write_pipe;
static child_data_t data = { .pc = 0, .n = 0 };

/**
 * @brief Generates a random number of iterations for the process lifespan.
 * 
 * @param pid Process ID used as the seed.
 * @return A random integer between MIN_ITER and MAX_ITER.
 */
static inline int _generate_iterations(pid_t pid)
{
    srand(pid); 
    return (rand() % (MAX_ITER - MIN_ITER + 1)) + MIN_ITER;
}

/**
 * @brief Triggers a system call to the KernelSim.
 * 
 * Sends the requested operation to the kernel via the write pipe.
 * 
 * @param op The requested ChildOp (CHILD_OP_WRITE or CHILD_OP_READ).
 * @return DEBUG_RET_SUCCESS on successful write, exits on failure.
 */
static DebugRet _child_syscall(ChildOp op)
{
    printf("[SYSCALL | PC %d] Requesting OP %d\n", data.pc, op);

    ChildOp tmp = op;
    ssize_t bytes_written = write(write_pipe, &tmp, sizeof(tmp));
    
    if (bytes_written == -1) {
        perror("[SYSCALL] Write Failed");
        exit(DEBUG_RET_ERR_SYSCALL);
    }

    return DEBUG_RET_SUCCESS;
}

/**
 * @brief Main execution loop of the application process.
 * 
 * @param max_iterations Total number of iterations before the process exits.
 * @return 0 upon successful completion.
 */
static int _child_loop(int max_iterations)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = SLEEP_TIME * 1000000L; // 500 ms in nanoseconds

    while (data.pc < max_iterations) {
        data.pc++;
        nanosleep(&ts, NULL);

        int prob = rand() % 100;
        if (prob < SYSCALL_PROB) {
            if (prob % 2) {
                // Read Syscall
                _child_syscall(CHILD_OP_READ);
                
                int brother_n;
                ssize_t bytes_read = read(read_pipe, &brother_n, sizeof(brother_n));
                
                if (bytes_read == -1) {
                    printf("[PIPE | PC %d] Read Failed\n", data.pc);
                    return -1;
                }
                
                if (brother_n == 0) {
                    printf("[PIPE | PC %d] Read Pipe Empty (NO_WAIT)\n", data.pc);
                } else {
                    data.n = brother_n;
                    printf("[PIPE | PC %d] Received N = %d\n", data.pc, data.n);
                }
            }
            
            else _child_syscall(CHILD_OP_WRITE);
        }
        
        nanosleep(&ts, NULL);
    }
    return 0;
}

/**
 * @brief Entry point for the child application process.
 * 
 * @param argc Argument count.
 * @param argv Argument vector (expects read_pipe, write_pipe, and shm_id).
 * @return 0 upon successful termination.
 */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: ./child <read_pipe> <write_pipe>\n");
        exit(DEBUG_RET_EXEC_ERROR);
    }
    
    pid_t pid = getpid();
    printf("[Process %d Running]\n", pid);

    read_pipe  = atoi(argv[1]);
    write_pipe = atoi(argv[2]);

    int max_iterations = _generate_iterations(pid);
    printf("[Process %d] Max Iterations: %d\n", pid, max_iterations);

    raise(SIGSTOP); // Suspends until KernelSim sends SIGCONT
    _child_loop(max_iterations);

    return 0;
}
