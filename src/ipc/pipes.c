/**
 * @file pipes.c
 * @brief Implementation of bidirectional pipe utilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pipes.h"


DebugRet
pipe_make(pipe_t* p) 
{
    if (pipe(p->to) < 0) {
        perror("[PIPE ERROR] Failed to create 'to' pipe");
        return DEBUG_RET_PIPE_ERROR;
    }
    puts("Pipe 'To' Created");
    
    if (pipe(p->from) < 0) {
        perror("[PIPE ERROR] Failed to create 'from' pipe");
        
        // Rollback: close the 'to' pipe since 'from' failed
        close(p->to[PIPE_READ]);
        close(p->to[PIPE_WRITE]);
        
        return DEBUG_RET_PIPE_ERROR;
    }
    puts("Pipe 'From' Created");

    return DEBUG_RET_SUCCESS;
}
