/**
 * @file pipes.h
 * @brief Bidirectional pipe definitions and utilities.
 *
 * This header defines a bidirectional pipe structure using standard UNIX pipes.
 * It is used to establish two-way inter-process communication (IPC) between 
 * paired application processes.
 */

#ifndef PIPES_H
#define PIPES_H

#include "utils/debug.h"

/** @def PIPE_READ
 *  @brief Array index for the read end of a pipe. */
#define PIPE_READ  0

/** @brief Array index for the write end of a pipe. */
#define PIPE_WRITE 1

/**
 * @brief Structure representing a bidirectional pipe.
 * 
 * Encapsulates two standard unidirectional Unix pipes to enable 
 * two-way communication.
 */
typedef struct
{
    int to[2];   // Pipe for Parent -> Child communication
    int from[2]; // Pipe for Child -> Parent communication
} pipe_t;

/**
 * @brief Initializes a bidirectional pipe.
 * 
 * Creates two internal pipes (`to` and `from`). If any creation fails, 
 * it cleans up previously created file descriptors and returns an error.
 * 
 * @param p Pointer to the pipe_t structure to be initialized.
 * @return DEBUG_RET_SUCCESS on success, DEBUG_RET_PIPE_ERROR on failure.
 */
DebugRet
pipe_make(pipe_t* p);

#endif /* PIPES_H */
