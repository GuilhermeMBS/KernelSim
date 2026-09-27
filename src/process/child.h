/**
 * @file child.h
 * @brief Definitions for application process data and operations.
 *
 * This header defines the system call operations that an application process 
 * can request from the kernel, as well as the shared memory context structure 
 * used to synchronize the Program Counter (PC) and the partner's counter (N).
 */

#ifndef CHILD_H
#define CHILD_H

/**
 * @brief Inter-Process Communication (IPC) operations.
 *
 * Represents the "fake" system calls requested by the application 
 * process to the KernelSim via the write pipe.
 */
typedef enum
{
    CHILD_OP_WRITE = 1, // Request to send the current PC to the partner
    CHILD_OP_READ       // Request to receive the partner's PC into N
} ChildOp;

/**
 * @brief Process context stored in shared memory.
 *
 * Holds the execution state of a child process. KernelSim maps this 
 * structure to read the process PC and update the received N value.
 */
typedef struct
{
    int pc; // Program Counter: Current iteration of the process
    int n;  // Received Counter: Partner's PC retrieved via IPC
} child_data_t;

#endif /* CHILD_H */
