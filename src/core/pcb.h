/**
 * @file pcb.h
 * @brief Process Control Block (PCB) definitions for KernelSim.
 *
 * This header defines the data structures used by the KernelSim to manage
 * the state, context, and inter-process communication of the simulated
 * application processes and the interrupt controller.
 */

#ifndef PCB_H
#define PCB_H

#include <sys/types.h>
#include <signal.h>

#include "ipc/pipes.h"
#include "process/child.h"
#include "utils/debug.h"
#include "utils/queue.h"

// Total number of simulated application processes.
#define NUM_CHILDREN 6

// Maximum capacity of the internal kernel queue for peer communication.
#define BROTHER_PIPE_SIZE 16

/**
 * @brief X-Macro list of all children indices.
 * 
 * Ensures compile-time generation of exactly NUM_CHILDREN items.
 */
#define CHILDREN_LIST   \
    X(0)                \
    X(1)                \
    X(2)                \
    X(3)                \
    X(4)                \
    X(5)

/**
 * @brief Execution states of an application process.
 */
typedef enum
{
    PCB_STATE_READY = 0,  // Process is ready to run
    PCB_STATE_WAIT_RECV,  // Process is blocked waiting for a read (recv)
    PCB_STATE_WAIT_SEND,  // Process is blocked waiting for a write (send)
    PCB_STATE_RUNNING,    // Process is currently executing
    PCB_STATE_DONE        // Process has finished execution
} PcbState;

/**
 * @brief System call statistics for a process.
 */
typedef struct
{
    int nread;            // Total number of read syscalls executed
    int nwrite;           // Total number of write syscalls executed
} pcb_data_t;

/**
 * @brief Process Control Block (PCB) for application processes.
 */
typedef struct
{
    pipe_t child;         // Kernel-to-Child bidirectional IPC pipe
    queue_t* brother;     // Kernel internal buffer simulating the peer-to-peer pipe
    pcb_data_t data;      // Statistics on performed system calls
    child_data_t ctx;     // Saved execution context (PC and N)
    pid_t pid;            // Native OS process ID
    PcbState state;       // Current scheduling state
} pcb_child_t;

/**
 * @brief Process Control Block (PCB) for the InterController.
 */
typedef struct
{
    pipe_t child;         // Kernel-to-Controller bidirectional IPC pipe
    pid_t pid;            // Native OS process ID of the controller
} pcb_controller_t;

#endif /* PCB_H */
