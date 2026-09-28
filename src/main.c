#include <stdio.h>
#include <unistd.h>

#include "core/kernelsim.h"
#include "utils/debug.h"


static inline void
_wait_enter(const char *msg)
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
    sleep(1);
    puts("[Kernel Simulation Ready]");

    #if DEBUG
    _wait_enter("[Press ENTER to Start Simulation]");
    #endif

    puts("[Starting Kernel Simulation]");
    kernelsim_start();

    return 0;
}
