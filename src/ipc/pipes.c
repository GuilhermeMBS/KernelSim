/**
 * @file pipes.c
 * @brief Implementation of bidirectional pipe utilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pipes.h"


DebugRet pipe_make(pipe_t* p) 
{
    if (pipe(p->to) < 0) {
        perror("[PIPE ERROR] Failed to create 'to' pipe");
        return DEBUG_RET_PIPE_ERROR;
    }
    printf("[PID %d] Pipe 'To' Created\n", getpid());
    
    if (pipe(p->from) < 0) {
        perror("[PIPE ERROR] Failed to create 'from' pipe");
        
        // Rollback: close the 'to' pipe since 'from' failed
        close(p->to[PIPE_READ]);
        close(p->to[PIPE_WRITE]);
        
        return DEBUG_RET_PIPE_ERROR;
    }
    printf("[PID %d] Pipe 'From' Created\n", getpid());

    return DEBUG_RET_SUCCESS;
}
