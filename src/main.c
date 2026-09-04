#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

typedef enum {
    EX_SUCCESS,
    EX_EXEC_ERROR,
    EX_FORK_ERROR,
    EX_WAIT_ERROR
} ExitStatus;


int main() {
    pid_t controller_pid, kernel_pid;
    int controller_status, kernel_status;

    // Inter Controller
    puts("Creating InterC Controller...");
    controller_pid = fork();

    if (controller_pid == 0) {
        int exec_ret = 0;
        exec_ret = execve("intercontroller", NULL, NULL);

        if (exec_ret == -1) {
            perror("Error while executing the Inter Controller");
            exit(EX_EXEC_ERROR);
        }
    }

    else if (controller_pid < 0) {
        perror("Error while creating the Inter Controller");
        exit(EX_FORK_ERROR);
    }

    // Kernel Sim
    puts("Creating KernelSim...");
    kernel_pid = fork();

    if (kernel_pid == 0) {
        int exec_ret = 0;
        
        puts("Executing KernelSim...");
        exec_ret = execve("kernel", NULL, NULL);

        if (exec_ret == -1) {
            perror("Error while executing the KernelSim");
            exit(EX_EXEC_ERROR);
        }
    }

    else if (kernel_pid < 0) {
        perror("Error while creating the KernelSim");
        exit(EX_FORK_ERROR);
    }

    // Main waits for Controller
    pid_t controller_end_pid = waitpid(controller_pid, controller_status, 0);
    if (controller_end_pid == -1) {
        perror("Controller Waitpid Failed");
        exit(EX_WAIT_ERROR);
    }

    // Main waits for Kernel
    pid_t kernel_end_pid = waitpid(kernel_pid, kernel_status, 0);
    if (kernel_end_pid == -1) {
        perror("Kernel Waitpid Failed");
        exit(EX_WAIT_ERROR);
    }

    return 0;
}