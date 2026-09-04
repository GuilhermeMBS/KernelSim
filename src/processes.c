/**
 * This process aims to run in parallel with the KernelSim.
 * It will do a random number of iterations ranging from
 * 5.000 to 10.000, using a time-sharing system made by the
 * Inter Controller module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

#define MAX_ITER 10000
#define MIN_ITER 5000

typedef enum {
    EX_SUCCESS,
    EX_EXEC_ERROR,
    EX_FORK_ERROR,
    EX_WAIT_ERROR
} ExitStatus;


inline int generate_iterations(pid_t pid) {
    srand(time(NULL)); // Generates a random seed

    int iterations = (rand() % (MAX_ITER - MIN_ITER + 1)) + MIN_ITER;
    printf("Max Iterations of Process %d: %d\n", pid, iterations);

    return iterations;
};

int main(int argc, char *argv[]) {
    pid_t my_pid = argv[0];
    printf("[Process %d Running]\n", my_pid);

    int max_iterations = generate_iterations(my_pid);

    return 0;
}
