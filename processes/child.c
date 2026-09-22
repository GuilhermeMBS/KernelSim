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
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_ITER 10000
#define MIN_ITER 5000


static int PC = 0; // Process Counter
static int N = 0;  // Brother Counter


inline int
generate_iterations(pid_t pid)
{
    srand(pid); // Generates a random seed

    int iterations = (rand() % (MAX_ITER - MIN_ITER + 1)) + MIN_ITER;
    printf("Max Iterations of Process %d: %d\n", pid, iterations);

    return iterations;
};


int
main(int argc, char *argv[])
{
    pid_t pid = atoi(argv[1]);
    printf("[Process %d Running]\n", pid);

    int kernel_pipe = atoi(argv[2]);
    int brother_pipe = atoi(argv[3]);

    int max_iterations = generate_iterations(pid);

    return 0;
}
