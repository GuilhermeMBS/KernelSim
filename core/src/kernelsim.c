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

#include "include/intercontroller.h"
#include "include/kernelsim.h"
#include "include/retcode.h"
#include "include/pcb.h"

#define READ  0
#define WRITE 1


static retcode_t
_kernelsim_build_child_pipes(pcb_child_t* procs)
{
    puts("Building Children Pipes...");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pipe_make(&(procs[i].child));
        pipe_make(&(procs[i].brother));
    }
    puts("Children Pipes Ready.");

    return SUCCESS;
}


static retcode_t
_kernelsim_build_controller_pipes(pcb_controller_t* proc)
{
    puts("Building Intercontroller Pipes...");
    pipe_make(&proc->child);
    puts("Intercontroller Pipes Ready.");

    return SUCCESS;
}


static retcode_t
_kernelsim_exec_child(pcb_child_t* procs)
{
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid > 0) {
            // Set PCB Struct
            procs[i].pid   = pid;
            procs[i].time  = 0;
            procs[i].state = WAIT;

            // Close Child Unused Pipe Ends
            close(procs[i].child.to[READ]);
            close(procs[i].child.from[WRITE]);

            // Close Brother Pipe Ends
            close(procs[i].brother.to[READ]);
            close(procs[i].brother.to[WRITE]);
            close(procs[i].brother.from[READ]);
            close(procs[i].brother.from[WRITE]);
        }

        else if (pid == 0) {
            // Close Unused Pipe Ends
            close(procs[i].child.to[WRITE]);
            close(procs[i].child.from[READ]);

            char read_fd_str[16], write_fd_str[16], id_str[16];
            snprintf(read_fd_str, sizeof(read_fd_str), "%d", procs[i].child.to[READ]);
            snprintf(write_fd_str, sizeof(write_fd_str), "%d", procs[i].child.from[WRITE]);
            snprintf(id_str, sizeof(id_str), "%d", i);

            execl("./bin/child", "child", id_str, read_fd_str, write_fd_str, NULL);
        }

        else {
            printf("[PID %d] Child %d Fork Error!\n", pid, i);
            exit(FORK_ERROR);
        }
    }

    return SUCCESS;
}


static retcode_t
_kernelsim_exec_controller(pcb_controller_t* proc)
{
    pid_t pid = fork();

    if (pid > 0) {
        proc->pid = pid;

        // Close Unused Pipe Ends
        close(proc->child.to[READ]);
        close(proc->child.from[WRITE]);
    }

    else if (pid == 0) {
        // Redirects the Read End of (Kernel --> Controller) to STDOUT
        dup2(proc->child.to[READ], STDOUT_FILENO);

        // Redirects the Write End of (Controller --> Kernel) to STDIN
        dup2(proc->child.from[WRITE], STDIN_FILENO);

        // Close Unused Pipe Ends
        close(proc->child.to[WRITE]);
        close(proc->child.from[READ]);

        // Replace process image with the child binary
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d");

        execl("./bin/intercontroller", "controller", id_str, NULL);

        perror("execl failed");
        exit(EXEC_ERROR);
    }

    else {
        printf("[PID %d] Controller Fork Error!\n", getpid());
        exit(FORK_ERROR);
    }

    return SUCCESS;
}


retcode_t 
kernelsim_init() 
{
    // Build Pipes
    _kernelsim_build_child_pipes(processes);
    _kernelsim_build_controller_pipes(&controller);

    // Create Processes
    _kernelsim_exec_child(processes);
    _kernelsim_exec_controller(&controller);

    return SUCCESS;
}


retcode_t 
kernelsim_start() 
{
    // CTRL-Z Handler to Show Status
    // Send Signal to Start in Pipes
    return SUCCESS;
}
