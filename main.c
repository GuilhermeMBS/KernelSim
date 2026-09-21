#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "include/kernelsim.h"

#define TRUE 1
#define FALSE 0


volatile sig_atomic_t started = FALSE;
volatile sig_atomic_t running = FALSE;


void
handle_sig(int signal)
{
    if (signal == SIGTSTP) {
        if (!started) {
            puts("[Wait for the Simulation to be Ready]");
            exit(0);
        }

        if (running) {
            puts("[Pausing Kernel Simulation...]");
            kernelsim_pause();
            puts("[Kernel Simulation Paused]");

            running = FALSE;
            exit(0);
        }
        else {
            puts("[Resuming Kernel Simulation...]");
            kernelsim_resume();

            running = TRUE;
            exit(0);
        }
    }
}


void
wait_enter(const char *msg)
{
    if (msg) {
        puts(msg);
        fflush(stdout);
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


int 
main(void)
{
    puts("[Building Kernel Simulation]");
    kernelsim_init();
    puts("[Kernel Simulation Ready]");

    kernelsim_show();
    wait_enter("[Press ENTER to Start Simulation]");

    puts("[Starting Kernel Simulation]");
    kernelsim_start();
    started = TRUE;

    return 0;
}
