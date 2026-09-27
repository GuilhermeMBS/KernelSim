#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "ipc/intercontroller.h"


#define INTERCONTROLLER_PATH "./bin/intercontroller"


static long long current_time_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1e6;
}


/*
 * Wait for one byte to become available on fd.
 *
 * Returns:
 *   1  -> byte available and read
 *   0  -> timeout
 *  -1 -> error
 */
static int 
read_signal(
    int fd, char *signal, int timeout_ms
)
{
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds); // Empty set
    FD_SET(fd, &readfds); // Add fd to the file descriptor set

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    // Check if something appears on the pipe with the time limit of "timeout"
    int result = select(
        fd + 1, 
        &readfds, // Just care for reading
        NULL, // Don't care for write, nor exceptions
        NULL, 
        &timeout
    );

    if (result < 0) {
        if (errno == EINTR) // Received a sys call/signal and interrupted
            return 0;

        perror("select");
        return -1;
    }

    // Nothing became available
    if (result == 0)
        return 0;

    // Select found something on fd
    ssize_t bytes = read(fd, signal, 1);

    if (bytes == 1)
        return 1;

    return -1;
}


static pid_t start_intercontroller(int *read_fd)
{
    int pipefd[2];

    assert(pipe(pipefd) == 0);

    pid_t pid = fork();

    assert(pid >= 0);

    if (pid == 0) {
        close(pipefd[0]);

        assert(dup2(pipefd[1], STDOUT_FILENO) >= 0);

        close(pipefd[1]);

        execl( INTERCONTROLLER_PATH, INTERCONTROLLER_PATH, NULL);

        perror("execl");
        _exit(EXIT_FAILURE);
    }

    // Parent.
    close(pipefd[1]);

    *read_fd = pipefd[0];

    return pid;
}


static void stop_intercontroller(pid_t pid, int fd)
{
    kill(pid, SIGKILL);

    close(fd);

    waitpid(pid, NULL, 0);
}


/*
 * Test that the intercontroller starts stopped.
 *
 * intercontroller.c calls raise(SIGSTOP) before entering
 * its main loop.
 */
static void test_starts_stopped(void)
{
    printf("\ttest_starts_stopped... ");

    int fd;
    pid_t pid = start_intercontroller(&fd);

    int status;

    // WUNTRACED -> Returns when the process gets stopped
    pid_t result = waitpid(pid, &status, WUNTRACED);

    assert(result == pid);
    assert(WIFSTOPPED(status)); // Verify if it was stopped by a signal

    stop_intercontroller(pid, fd);

    printf("PASS\n");
}


// Test if IQR0 is generated every time slice.
static void 
test_iqr0(void)
{
    printf("\ttest_iqr0... ");

    int fd;
    pid_t pid = start_intercontroller(&fd);

    int status;

    assert(waitpid(pid, &status, WUNTRACED) == pid);

    long long start = current_time_ms();

    kill(pid, SIGCONT);

    char signal;
    int result = read_signal(
        fd,
        &signal,
        TIME_SLICE + 250
    );

    long long elapsed = current_time_ms() - start;

    assert(result == 1);

    assert(signal == '0');

    assert(elapsed >= TIME_SLICE - 100);
    assert(elapsed < TIME_SLICE + 100);

    stop_intercontroller(pid, fd);

    printf("PASS (%lld ms)\n", elapsed);
}


// Para verificar o IQR1/2, aumentar a chance no "intercontroller.c"
static void 
test_valid_signals(void)
{
    printf("\ttest_valid_signals... ");

    int fd;
    pid_t pid = start_intercontroller(&fd);

    int status;

    assert(waitpid(pid, &status, WUNTRACED) == pid);

    int signals[3] = {0, 0 ,0};

    kill(pid, SIGCONT);

    for (int i = 0; i < 20; i++) {
        char signal;

        int result = read_signal(
            fd,
            &signal,
            TIME_SLICE + 500
        );

        assert(result == 1);

        assert(
            signal == '0' ||
            signal == '1' ||
            signal == '2'
        );

        switch(signal)
        {
            case '0':
                signals[0]++;
                break;
            case '1':
                signals[1]++;
                break;
            default:
                signals[2]++;
        }
    }
    printf("PASS\n");

    stop_intercontroller(pid, fd);
    
    for (int i = 0; i < 3; i++) printf("\t\tSignal %d: %d\n", i, signals[i]);
}


int 
main(void)
{
    printf("Running intercontroller tests...\n\n");

    test_starts_stopped();
    test_iqr0();
    test_valid_signals();

    printf("\nAll intercontroller tests passed.\n");

    return 0;
}