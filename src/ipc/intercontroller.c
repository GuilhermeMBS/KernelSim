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

#include "intercontroller.h"


#define PROB_1 10     // Probability of Signal IQR1 to Happen (in percent)
#define PROB_2 5      // Probability of Signal IQR2 to Happen (in percent)
#define RAND_SEED 0     // Random Seed Flag (1 = True / 0 = False)
#define TRUE  1
#define FALSE 0


const int prob_1 = PROB_1;
const int prob_2 = PROB_2;


static int
_intercontroller_generate_probability()
{
    #if RAND_SEED
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    unsigned int seed = (unsigned int)(ts.tv_sec ^ ts.tv_nsec);
    srand(seed);
    #endif

    return (rand() % 100);
}


inline static void
_intercontroller_send(IntercontrollerSig signal)
{
    fflush(stdout);
    printf("%d", signal);
    fflush(stdout);
}


int
main(void)
{
    struct timespec ts;
    ts.tv_sec = TIME_SLICE / 1000;
    ts.tv_nsec = (TIME_SLICE % 1000) * 1e6;
    
    raise(SIGSTOP);
    
    while (TRUE) {
        nanosleep(&ts, NULL);

        _intercontroller_send(INTERCONTROLLER_SIG_IQR0);
        
        int prob = _intercontroller_generate_probability();

        if (prob < prob_2) _intercontroller_send(INTERCONTROLLER_SIG_IQR2);
        if (prob < prob_1) _intercontroller_send(INTERCONTROLLER_SIG_IQR1);
    }

    return 0;
}
