/**
 * @file kernelsim.h
 * @brief Public interface for the Kernel Simulator.
 *
 * This header exposes the main lifecycle functions of the KernelSim.
 * It provides the routines required to bootstrap the simulated OS environment
 * and to start the main event-driven scheduling loop.
 */

#ifndef KERNELSIM_H
#define KERNELSIM_H

/**
 * @brief Initializes the kernel simulation environment.
 * 
 * This function bootstraps the system by registering signal handlers 
 * (for CTRL-C and CTRL-Z), allocating POSIX Shared Memory segments, 
 * building the bidirectional IPC pipes, and forking both the 
 * application processes (children) and the Intercontroller.
 */
void
kernelsim_init(void);

/**
 * @brief Starts the kernel simulation loop.
 * 
 * Awakens the initially suspended processes (the Intercontroller and 
 * the first scheduled child) and enters an infinite event loop using `poll()`.
 * It continuously handles clock interrupts (IRQ0), IPC responses (IRQ1, IRQ2), 
 * and system call requests, while also managing the debug visualizer state.
 */
void
kernelsim_start(void);

#endif /* KERNELSIM_H */