#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pipes.h"


DebugRet pipe_make(pipe_t* p) {
    if(pipe(p->to) < 0) {
        printf("[PID %d] Pipe To Error!\n", getpid());
        exit(DEBUG_RET_PIPE_ERROR);
    }
    else printf("[PID %d] Pipe To Created\n", getpid());
    
    if(pipe(p->from) < 0) {
        printf("[PID %d] Pipe From Error!\n", getpid());
        exit(DEBUG_RET_PIPE_ERROR);
    }
    else 

    return DEBUG_RET_SUCCESS;
}
