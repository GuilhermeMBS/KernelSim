#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

typedef enum {
    EX_SUCCESS,
    EX_EXEC_ERROR,
    EX_FORK_ERROR
} ExitStatus;


int main() {
    puts("Inter Controller Running");


    return 0;
}