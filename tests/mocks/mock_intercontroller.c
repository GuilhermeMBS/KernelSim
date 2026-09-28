#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "ipc/intercontroller.h"

int main(void) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000L; // 500ms

    while (1) {
        nanosleep(&ts, NULL);
        
        // Envia IRQ0 em formato binário nativo
        IntercontrollerSig sig = INTERCONTROLLER_SIG_IRQ0;
        write(STDOUT_FILENO, &sig, sizeof(IntercontrollerSig));
        
        // Simula respostas aleatórias do controlador
        if (rand() % 100 < 20) {
            sig = INTERCONTROLLER_SIG_IRQ2;
            write(STDOUT_FILENO, &sig, sizeof(IntercontrollerSig));
        }
    }
    return 0;
}
