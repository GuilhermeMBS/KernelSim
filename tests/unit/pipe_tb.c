#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipc/pipes.h"

static void
test_pipe_make()
{
    printf("\ttest_pipe_make...");
    pipe_t pipe;

    DebugRet result = pipe_make(&pipe);

    assert(result == DEBUG_RET_SUCCESS);

    close(pipe.from[0]); close(pipe.from[1]);
    close(pipe.to[0]); close(pipe.to[1]);

    printf(" PASS\n");
}

int
main(void)
{
    printf("Running pipe tests...\n\n");

    test_pipe_make();

    printf("\nAll pipes tests passed\n");

    return 0;
}