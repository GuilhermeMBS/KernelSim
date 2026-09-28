/**
 * @file intercontroller.c
 * @brief Implementation of the interrupt controller emulator.
 *
 * This file implements the main loop of the InterController Sim process.
 * It simulates a hardware interrupt controller by periodically generating
 * an IRQ0 signal for context switching, and probabilistically generating
 * IRQ1 and IRQ2 signals to emulate the completion of read and write
 * IPC operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "intercontroller.h"

#define PROB_1 10       // Probability of Signal IRQ1 to happen (in %)
#define PROB_2 5        // Probability of Signal IRQ2 to happen (in %)
#define RAND_SEED 1     // Random Seed Flag (1 = True / 0 = False)

/**
 * @brief Sends an interrupt signal to the standard output.
 * 
 * Emulates the triggering of an IRQ by piping its enum value to stdout,
 * forcing a flush to ensure immediate delivery to KernelSim. No newline
 * is appended, expecting the receiver to read exactly 1 byte.
 * 
 * @param signal The interrupt signal to be sent.
 */
static inline void
_intercontroller_send(IntercontrollerSig signal)
{
    fflush(stdout);
    write(STDOUT_FILENO, &signal, sizeof(IntercontrollerSig));
    fflush(stdout);
}

int
main(void)
{
    struct timespec ts;
    
    // Configure the time slice interval
    ts.tv_sec = TIME_SLICE / 1000;
    ts.tv_nsec = (TIME_SLICE % 1000) * 1000000L;
    
#if RAND_SEED
    struct timespec seed_ts;
    clock_gettime(CLOCK_MONOTONIC, &seed_ts);
    unsigned int seed = (unsigned int)(seed_ts.tv_sec ^ seed_ts.tv_nsec);
    srand(seed);
#endif

    raise(SIGSTOP); // Suspends until KernelSim sends SIGCONT
    while (true) {
        nanosleep(&ts, NULL);
        _intercontroller_send(INTERCONTROLLER_SIG_IRQ0);
        
        int prob_recv = (rand() % 100);
        int prob_send = (rand() % 100);

        if (prob_recv < PROB_1) _intercontroller_send(INTERCONTROLLER_SIG_IRQ1);
        if (prob_send < PROB_2) _intercontroller_send(INTERCONTROLLER_SIG_IRQ2);
    }

    return 0;
}
