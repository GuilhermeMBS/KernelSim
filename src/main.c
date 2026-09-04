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
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define NUM_CHILDS 6 // Number of processes managed by the KernelSim

typedef enum {
    EX_SUCCESS,
    EX_EXEC_ERROR,
    EX_FORK_ERROR,
    EX_WAIT_ERROR
} ExitStatus;


int main(void) {
    puts("[KernelSim Running]");

    pid_t controller_pid;
    int controller_status;

    // Inter Controller
    puts("Creating InterC Controller...");
    controller_pid = fork();

    if (controller_pid == 0) {
        int exec_ret = 0;
        exec_ret = execve("intercontroller", NULL, NULL);

        if (exec_ret == -1) {
            perror("Error while executing the Inter Controller!");
            exit(EX_EXEC_ERROR);
        }
    }

    else if (controller_pid < 0) {
        perror("Error while creating the Inter Controller!");
        exit(EX_FORK_ERROR);
    }

    pid_t pids[NUM_CHILDS];

    // Kernel waits for Controller
    pid_t controller_end_pid = waitpid(controller_pid, controller_status, 0);
    if (controller_end_pid == -1) {
        perror("Controller Waitpid Failed!");
        exit(EX_WAIT_ERROR);
    }

    return 0;
}
