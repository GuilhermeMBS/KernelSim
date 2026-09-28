/**
 * @file test_pipes.c
 * @brief Unit tests for the bidirectional pipe implementation.
 *
 * This test suite verifies the correct creation and initialization of 
 * the pipe_t structure, ensuring that file descriptors are properly 
 * allocated and can be safely closed manually.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipc/pipes.h"

/**
 * @brief Tests the creation and destruction of a bidirectional pipe.
 *
 * Calls pipe_make() to allocate the pipe and asserts that it returns
 * DEBUG_RET_SUCCESS. Afterwards, it manually closes all allocated 
 * file descriptors to prevent resource leaks during the test.
 */
static void
test_pipe_make(void)
{
    printf("\ttest_pipe_make... ");
    
    pipe_t p;
    DebugRet result = pipe_make(&p);

    // Verify successful creation
    assert(result == DEBUG_RET_SUCCESS);

    // Manually clean up the allocated file descriptors
    close(p.from[PIPE_READ]); 
    close(p.from[PIPE_WRITE]);
    close(p.to[PIPE_READ]); 
    close(p.to[PIPE_WRITE]);

    printf("PASS\n");
}


int
main(void)
{
    printf("Running pipe tests...\n\n");
    test_pipe_make();
    printf("\nAll pipe tests passed.\n");

    return 0;
}
