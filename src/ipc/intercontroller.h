#ifndef INTERCONTROLLER_H
#define INTERCONTROLLER_H

typedef enum
{
    INTERCONTROLLER_SIG_IRQ0,   // Time Slice
    INTERCONTROLLER_SIG_IRQ1,   // recv()
    INTERCONTROLLER_SIG_IRQ2,   // send()
    INTERCONTROLLER_SIG_ERROR
} IntercontrollerSig;

# endif
