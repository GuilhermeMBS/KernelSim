#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "ipc/intercontroller.h"

int
main(void)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000L; // 500ms (Time-slice do Round Robin)

    while (1) {
        nanosleep(&ts, NULL);
        
        // Envia interrupção de relógio
        IntercontrollerSig sig = INTERCONTROLLER_SIG_IRQ0;
        write(STDOUT_FILENO, &sig, sizeof(sig));
        
        // Simula resolução de IPC pelo sistema em background
        if (rand() % 100 < 30) {
            sig = INTERCONTROLLER_SIG_IRQ1;
            write(STDOUT_FILENO, &sig, sizeof(sig));
        }
        if (rand() % 100 < 30) {
            sig = INTERCONTROLLER_SIG_IRQ2;
            write(STDOUT_FILENO, &sig, sizeof(sig));
        }
    }
    return 0;
}
