/**
 * This process aims to run in parallel with the KernelSim.
 * It will do a random number of iterations ranging from
 * 5.000 to 10.000, using a time-sharing system made by the
 * Inter Controller module.
*/

/**
 * Loop que dura 1 seg
 * Probabilidade aleatória de pedir send ou recv
 * Levantar recv sem ter dado na pipe para receber vai retornar 0
 * e entra num NO_WAIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <time.h>

#include "process/child.h"
#include "utils/debug.h"

#define MAX_ITER        10000
#define MIN_ITER        5000
#define SLEEP_TIME      500     // Time in ms
#define SYSCALL_PROB    85      // Probability of a syscall in percentage


static int shm_id;
static child_data_t *own_data;
static int read_pipe;
static int write_pipe;


static inline int
_generate_iterations(pid_t pid)
{
    srand(pid); // Generates a random seed
    int iterations = (rand() % (MAX_ITER - MIN_ITER + 1)) + MIN_ITER;
    return iterations;
}


static DebugRet
_child_syscall(ChildOp OP)
{
    printf("[SYSCALL | PC %d] Write OP %d Successful\n", own_data->pc, OP);

    ChildOp tmp = OP;
    ssize_t bytes_written = write(write_pipe, &tmp, sizeof(tmp));
    if (bytes_written == -1)
    {
        perror("[SYSCALL] Write/Read Failed\n");
        exit(DEBUG_RET_ERR_SYSCALL);
    }

    return DEBUG_RET_SUCCESS;
}


static int
_child_loop(int max_iterations)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = SLEEP_TIME * 1e6;

    while (own_data->pc < max_iterations) {
        own_data->pc++;
        nanosleep(&ts, NULL);

        int prob = rand() % 100;
        if (prob < SYSCALL_PROB) {
            // Read Syscall
            if (prob % 2) {
                _child_syscall(CHILD_OP_READ);
                int brother_n;
                ssize_t bytes_read = read(read_pipe, &brother_n, sizeof(brother_n));
                if (bytes_read == -1) {
                    printf("[PIPE | PC %d] Read Failed\n", own_data->pc);
                    return -1;
                }
                
                if (brother_n == 0) printf("[PIPE | PC %d] Read Pipe Empty\n", own_data->pc);
                else own_data->n = brother_n;
            }
            // Write Syscall1
            else _child_syscall(CHILD_OP_WRITE);
        }
        
        nanosleep(&ts, NULL);
    }
    return 0;
}


int
main(int argc, char *argv[])
{
    if (argc != 4)
    {
        perror("Wrong amount of arguments.\n\t.\\child pipe_fd[0] pipe_fd[1] shm_id");
        exit(DEBUG_RET_EXEC_ERROR);
    }
    
    pid_t pid = getpid();
    printf("[Process %d Running]\n", pid);

    read_pipe   = atoi(argv[1]);
    write_pipe  = atoi(argv[2]);
    shm_id      = atoi(argv[3]);

    own_data = (child_data_t *)shmat(shm_id, NULL, 0);
    if (own_data == (void*)-1)
    {
        perror("[SHM] Shmat error");
        exit(DEBUG_RET_ERR_SHM);
    }

    int max_iterations = _generate_iterations(pid);
    printf("[Process %d] Max Iterations: %d\n", pid, max_iterations);

    raise(SIGSTOP);
    _child_loop(max_iterations);

    return 0;
}
