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

#include "include/kernelsim.h"


// Signal Handler...


int 
main(void)
{
    puts("[Building Kernel Simulation]");
    ks_init();
    puts("[Kernel Simulation Ready]");

    ks_show_controller();
    ks_show_child();

    // Build Signal Handler
    puts("[Press ENTER to Start Simulation]");
    // Wait for ENTER

    puts("[Starting Kernel Simulation]");
    ks_start();
    // If signal of kill, clean
    puts("[End of Kernel Simulation]");

    return 0;
}
