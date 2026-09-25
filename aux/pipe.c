#include "pipe.h"
#include "retcode.h"


retcode_t pipe_make(pipe_t* p) {
    if(pipe(p->to) < 0) {
        printf("[PID %d] Pipe To Error!\n", getpid());
        exit(PIPE_ERROR);
    }
    else printf("[PID %d] Pipe To Created\n", getpid());
    
    if(pipe(p->from) < 0) {
        printf("[PID %d] Pipe From Error!\n", getpid());
        exit(PIPE_ERROR);
    }
    else 

    return SUCCESS;
}
