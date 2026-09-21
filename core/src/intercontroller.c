/**
 * Um IRQ0 (TimeSlice) a cada 500 ms (use sleep() dentro do corpo do loop) 
 * Um IRQ1 com probabilidade P_1 = 0.1 (a cada 500 ms)
 * probabilidade de um recv() ser concluído
 * Um IRQ2 com probabilidade P_2 = 0.05 (a cada 500 ms)
 * probabilidade de um send() terminar
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "include/intercontroller.h"

#define TRUE  1
#define FALSE 0


const float p_1 = PROB_1;
const float p_2 = PROB_2;
const uint64_t time_slice = (TIME_SLICE) * 1e6; // Converts Time Slice to nseconds 


static int
_intercontroller_generate_probability()
{
    #if RAND_SEED
    struct timespec ts;
    unsigned int seed = (unsigned int)(ts.tv_sec ^ ts.tv_nsec);
    srand(seed);
    #endif

    return (rand() % 100);
}


int
main(void)
{
    printf("[Intercontroller Running]");

    while (TRUE) {
        usleep(time_slice);
        int prob = _intercontroller_generate_probability();

        if (prob < 5) {
            // IQR2
        }

        if (prob < 15) {
            // IQR1
        }

        // send IQR0
    }

    return 0;
}
