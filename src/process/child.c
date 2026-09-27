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
#include <unistd.h>

#include "child.h"


#define MAX_ITER        10000
#define MIN_ITER        5000
#define SLEEP_TIME      5e5     // Sleep time in micro seconds
#define SYSCALL_PROB    15      // Probability of a syscall in percentage


static int PC = 0; // Process iteration counter
static int N = 0;  // Brother iteration counter
static int read_pipe;
static int write_pipe;


inline int
_generate_iterations(pid_t pid)
{
    srand(pid); // Generates a random seed

    int iterations = (rand() % (MAX_ITER - MIN_ITER + 1)) + MIN_ITER;
    printf("[CHILD %d] Max Iterations: %d\n", pid, iterations);

    return iterations;
};


int
_child_syscall(ChildOp OP)
{
    child_data_t data = {.op = OP, .pc = PC};
    ssize_t bytes_written = write(write_pipe, &data, sizeof(data));

    if (bytes_written == -1) {
        printf("[SYSCALL | PC %d] Write Failed", PC);
        return -1;
    }
    else printf("[SYSCALL | PC %d] Write OP %d Successful", PC, OP);

    return 1;
};


int
_child_loop(int max_iterations)
{
    while (PC < max_iterations) {
        PC++;
        usleep(SLEEP_TIME);

        int prob = rand() % 100;
        if (prob < SYSCALL_PROB) {
            // Read Syscall
            if (prob % 2) {
                _child_syscall(CHILD_OP_READ);
                int brother_n;
                ssize_t bytes_read = read(read_pipe, &brother_n, sizeof(brother_n));
                if (bytes_read == -1) {
                    printf("[PIPE | PC %d] Read Failed", PC);
                    return -1;
                }
                
                if (brother_n == 0) printf("[PIPE | PC %d] Read Pipe Empty", PC);
                else N = brother_n;
            }
            // Write Syscall
            else _child_syscall(CHILD_OP_WRITE);
        }
        
        usleep(SLEEP_TIME);
    }
}


int
main(int argc, char *argv[])
{
    pid_t pid = atoi(argv[1]);
    printf("[Process %d Running]\n", pid);

    read_pipe = atoi(argv[2]);
    write_pipe = atoi(argv[3]);

    int max_iterations = _generate_iterations(pid);
    printf("[Process %d] Max Iterations: %d\n", pid, max_iterations);

    raise(SIGSTOP);
    _child_loop(max_iterations);

    return 0;
}
