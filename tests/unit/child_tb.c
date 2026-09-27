#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <time.h>

#include "process/child.h"


#define CHILD_PATH "./bin/child"


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


static pid_t
start_child(int pipe_fd[2], int shmid)
{
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        /*
         * child.c expects:
         * argv[1] = read_pipe
         * argv[2] = write_pipe
         * argv[3] = shmid
        */

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


// To test, raise the chance of callsys inside "child.c"
static void
test_child_syscall_Write()
{
    printf("\ttest_child_syscall_Write...");

    int pipe_fd[2];
    assert(pipe(pipe_fd) == 0);

    int shmid = create_shm();
    child_data_t* values = attach_shm(shmid);

    pid_t pid = start_child(pipe_fd, shmid);

    int status;
    pid_t result = waitpid(pid, &status, WUNTRACED);
    assert(result == pid);
    
    close(pipe_fd[1]);

    kill(pid, SIGCONT);
    
    const int attemps = 100;
    ChildOp Op_code;
    for (int i = 0; i < attemps; i++)
    {
        ssize_t bytes_read = read(pipe_fd[0], &Op_code, sizeof(Op_code));
        assert(bytes_read == sizeof(Op_code));

        if (Op_code == CHILD_OP_WRITE)
        {
            assert(values->pc > 0);
            break;
        }
    }

    stop_child(pid);
    close(pipe_fd[0]);

    printf(" PASS\n");
    
    detach_shm(shmid, values);
}


static void
test_child_syscall_Read()
{
    printf("\ttest_child_syscall_Read...");

    int c2p[2];
    int p2c[2];
    assert(pipe(c2p) == 0);
    assert(pipe(p2c) == 0);

    
    int pipe_fd[] = {p2c[0], c2p[1]};
    
    int shmid = create_shm();
    child_data_t* values = attach_shm(shmid);
    
    pid_t pid = start_child(pipe_fd, shmid);
    
    close(p2c[0]); close(c2p[1]);
    
    int status;
    pid_t result = waitpid(pid, &status, WUNTRACED);
    assert(result == pid);

    int qtd_n = 100;
    ssize_t bytes_written = write(p2c[1], &qtd_n, sizeof(qtd_n));
    assert(bytes_written != -1);

    kill(pid, SIGCONT);
    
    const int attemps = 100;
    ChildOp Op_code;
    for (int i = 0; i < attemps; i++)
    {
        ssize_t bytes_read = read(c2p[0], &Op_code, sizeof(Op_code));
        assert(bytes_read == sizeof(Op_code));

        if (Op_code == CHILD_OP_READ)
        {
            struct timespec ts;
            ts.tv_sec = 1;
            ts.tv_nsec = 0;
            nanosleep(&ts, NULL);
            break;
        }
    }
    assert(values->n == 100);

    stop_child(pid);
    close(pipe_fd[0]); close(pipe_fd[1]);

    printf(" PASS\n");
    
    detach_shm(shmid, values);
}


int
main(void)
{
    printf("Running child tests...\n\n");

    test_starts_stopped();
    test_child_syscall_Write();
    test_child_syscall_Read();

    printf("\nAll child tests passed.\n");

    return 0;
}