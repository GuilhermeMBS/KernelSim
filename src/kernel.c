#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define NUM_CHILDS 6 // Number of processes managed by the KernelSim

typedef enum {
    EX_SUCCESS,
    EX_EXEC_ERROR,
    EX_FORK_ERROR
} ExitStatus;


int main() {
    puts("KernelSim Running");

    pid_t pids[NUM_CHILDS];
    
    return 0;
}