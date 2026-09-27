#ifndef INTERCONTROLLER_H
#define INTERCONTROLLER_H

typedef enum
{
    INTERCONTROLLER_SIG_IQR0,   // Time Slice
    INTERCONTROLLER_SIG_IQR1,   // recv()
    INTERCONTROLLER_SIG_IQR2,   // send()
    INTERCONTROLLER_SIG_ERROR
} IntercontrollerSig;

# endif
