/**
 * @file test_child.c
 * @brief Unit tests for the application process (child).
 *
 * Validates the initialization and correct emission of system call 
 * requests via bidirectional pipes, ensuring deadlocks are avoided 
 * by mocking KernelSim responses.
 */

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

#include "process/child.h"

#define CHILD_PATH "./bin/mock_child"

static int
create_shm()
{
    int shmid = shmget(IPC_PRIVATE, sizeof(child_data_t), IPC_CREAT | 0666);
    assert(shmid >= 0);
    return shmid;
}


static child_data_t*
attach_shm(int shmid)
{
    child_data_t* mem = (child_data_t *)shmat(shmid, NULL, 0);
    assert(mem != (void*)-1);
    return mem;
}

static void
detach_shm(int shmid, child_data_t* mem)
{
    shmdt(mem);
    shmctl(shmid, IPC_RMID, NULL);
}

/**
 * @brief Forks and executes the child process for testing.
 * 
 * @param pipe_fd Array containing the read and write file descriptors.
 * @return The PID of the newly created child process.
 */
static pid_t
start_child(int pipe_fd[2], int shmid)
{
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        char pipe1[16];
        snprintf(pipe1, sizeof(pipe1), "%d", pipe_fd[0]);
        char pipe2[16];
        snprintf(pipe2, sizeof(pipe2), "%d", pipe_fd[1]);
        char shm[16];
        snprintf(shm, sizeof(shm), "%d", shmid);

        execl(CHILD_PATH, CHILD_PATH, pipe1, pipe2, shm, NULL);

        perror("execl");
        _exit(EXIT_FAILURE);
    }

    return pid;
}

/**
 * @brief Safely terminates a child process.
 * 
 * @param pid The process ID to terminate.
 */
static void
stop_child(pid_t pid)
{
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
}

static void
test_starts_stopped(void)
{
    printf("\ttest_starts_stopped...");

    int pipe_fd[2];
    assert(pipe(pipe_fd) == 0);

    int shmid = create_shm();
    child_data_t* values = attach_shm(shmid);

    pid_t pid = start_child(pipe_fd, shmid);

    int status;
    pid_t result = waitpid(pid, &status, WUNTRACED);

    assert(result == pid);
    assert(WIFSTOPPED(status));

    stop_child(pid);
    close(pipe_fd[0]); close(pipe_fd[1]);
    detach_shm(shmid, values);

    printf(" PASS\n");
}

/**
 * @brief Tests the emission of the WRITE system call.
 */
static void
test_child_syscall_write(void)
{
    printf("\ttest_child_syscall_write... ");

    int c2p[2]; // Child to Parent
    int p2c[2]; // Parent to Child
    assert(pipe(c2p) == 0);
    assert(pipe(p2c) == 0);

    int shmid = create_shm();
    child_data_t* values = attach_shm(shmid);

    // Child reads from p2c[0] and writes to c2p[1]
    int pipe_args[] = {p2c[0], c2p[1]};
    pid_t pid = start_child(pipe_args, shmid);

    close(p2c[0]); 
    close(c2p[1]);

    int status;
    pid_t result = waitpid(pid, &status, WUNTRACED);
    assert(result == pid);
    assert(WIFSTOPPED(status));

    kill(pid, SIGCONT);
    
    const int attempts = 100;
    ChildOp op_code;
    char has_written = 0;
    
    for (int i = 0; i < attempts; i++) {
        ssize_t bytes_read = read(c2p[0], &op_code, sizeof(op_code));
        
        if (bytes_read == sizeof(op_code)) {
            if (op_code == CHILD_OP_WRITE) {
                assert(values->pc > 0);
                has_written = 1;
                break;
            } 
            else if (op_code == CHILD_OP_READ) {
                // DEADLOCK PREVENTION: Child is blocked waiting for an answer.
                // We must reply so it can proceed to the next iteration.
                int mock_n = 42;
                write(p2c[1], &mock_n, sizeof(mock_n));
            }
        }
    }

    assert(has_written == 1);
    stop_child(pid);
    close(c2p[0]);
    close(p2c[1]);

    detach_shm(shmid, values);

    printf("PASS\n");
}

/**
 * @brief Tests the emission of the READ system call and blocking behavior.
 */
static void
test_child_syscall_read(void)
{
    printf("\ttest_child_syscall_read... ");

    int c2p[2]; // Child to Parent
    int p2c[2]; // Parent to Child
    assert(pipe(c2p) == 0);
    assert(pipe(p2c) == 0);

    int shmid = create_shm();
    child_data_t* values = (child_data_t*) attach_shm(shmid);

    // Child reads from p2c[0] and writes to c2p[1]
    int pipe_args[] = {p2c[0], c2p[1]};
    pid_t pid = start_child(pipe_args, shmid);
    
    close(p2c[0]); 
    close(c2p[1]);
    
    int status;
    pid_t result = waitpid(pid, &status, WUNTRACED);
    assert(result == pid);
    assert(WIFSTOPPED(status));

    kill(pid, SIGCONT);

    const int attempts = 100;
    ChildOp op_code;
    
    for (int i = 0; i < attempts; i++) {
        ssize_t bytes_read = read(c2p[0], &op_code, sizeof(op_code));
        
        if (bytes_read == sizeof(op_code)) {
            if (op_code == CHILD_OP_READ) {
                // Mocking KernelSim answering with partner's N.
                int mock_n = 42;
                write(p2c[1], &mock_n, sizeof(mock_n));
                break;
            }
        }
    }

    sleep(1);
    assert(values->n == 42);

    stop_child(pid);
    close(c2p[0]); 
    close(p2c[1]);

    detach_shm(shmid, values);

    printf("PASS\n");
}


int
main(void)
{
    printf("Running child tests...\n\n");

    test_starts_stopped();
    test_child_syscall_write();
    test_child_syscall_read();

    printf("\nAll child tests passed.\n");

    return 0;
}
