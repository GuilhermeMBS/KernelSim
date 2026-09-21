#ifndef INTERCONTROLLER_H
#define INTERCONTROLLER_H

// Frequency for IQR0 (in ms)
#define TIME_SLICE 500
// Probability of Signal IQR1 to Happen
#define PROB_1 0.10
// Probability of Signal IQR2 to Happen
#define PROB_2 0.05
// Random Seed Flag (1 = True / 0 = False)
#define RAND_SEED 0


typedef enum
{
    IQR0 = 3, // Time Slice
    IQR1 = 5, // recv()
    IQR2 = 7  // send()
} INTERCONTROLLER_IQR;

# endif