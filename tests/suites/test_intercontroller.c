/**
 * @file test_intercontroller.c
 * @brief Integration tests for the InterController Sim emulator.
 *
 * This test suite validates the behavior of the interrupt controller emulator
 * by verifying its startup state, the correct emission frequency of the time-slice
 * interrupt (IRQ0), and the probabilistic generation of IPC interrupts (IRQ1, IRQ2).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/wait.h>

#include "ipc/intercontroller.h"

#define INTERCONTROLLER_PATH "./bin/mock_ipc"

/**
 * @brief Retrieves the current monotonic time.
 * 
 * @return The current time in milliseconds.
 */
static long long
current_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

/**
 * @brief Waits for one valid byte (signal) to become available on the file descriptor.
 *
 * Uses select() to wait for data within a specified timeout. Reads exactly 1 byte.
 *
 * @param fd The file descriptor to read from (pipe).
 * @param signal Pointer to store the read signal.
 * @param timeout_ms Maximum time to wait in milliseconds.
 * 
 * @return 1 if a byte was read, 0 on timeout, -1 on error.
 */
static int
read_signal(int fd, IntercontrollerSig *signal, int timeout_ms)
{
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result;
    do {
        result = select(fd + 1, &readfds, NULL, NULL, &timeout);
    } while (result < 0 && errno == EINTR); // Retry if interrupted by system call/signal

    if (result < 0) {
        perror("select");
        return -1;
    }

    // Timeout: Nothing became available
    if (result == 0) return 0; 

    // Select found something on fd
    ssize_t bytes = read(fd, signal, sizeof(IntercontrollerSig));
    if (bytes == sizeof(IntercontrollerSig)) return 1;

    return -1;
}

/**
 * @brief Forks and executes the InterController Sim process.
 *
 * Sets up an anonymous pipe, redirecting the child's STDOUT to the pipe's
 * write end, so the parent can read the generated interrupts.
 *
 * @param read_fd Pointer to store the read end of the pipe.
 * @return The PID of the child process.
 */
static pid_t
start_intercontroller(int *read_fd)
{
    int pipefd[2];
    assert(pipe(pipefd) == 0);

    pid_t pid = fork();
    assert(pid >= 0);

    // Child process
    if (pid == 0) {
        close(pipefd[0]);
        assert(dup2(pipefd[1], STDOUT_FILENO) >= 0);
        close(pipefd[1]);

        execl(INTERCONTROLLER_PATH, INTERCONTROLLER_PATH, NULL);

        perror("execl");
        _exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[1]);
    *read_fd = pipefd[0];

    return pid;
}

/**
 * @brief Terminates the InterController Sim process safely.
 *
 * @param pid The PID of the process to terminate.
 * @param fd The file descriptor (pipe) to close.
 */
static void
stop_intercontroller(pid_t pid, int fd)
{
    kill(pid, SIGTERM);
    close(fd);
    waitpid(pid, NULL, 0);
}

/**
 * @brief Tests if the controller starts in a stopped state.
 *
 * Verifies that the intercontroller.c process successfully calls 
 * raise(SIGSTOP) before entering its main loop.
 */
static void
test_starts_stopped(void)
{
    printf("\ttest_starts_stopped... ");

    int fd;
    pid_t pid = start_intercontroller(&fd);
    int status;

    // WUNTRACED -> Returns when the process gets stopped
    pid_t result = waitpid(pid, &status, WUNTRACED);

    assert(result == pid);
    assert(WIFSTOPPED(status)); // Verify if it was explicitly stopped by a signal

    kill(pid, SIGCONT);
    stop_intercontroller(pid, fd);

    printf("PASS\n");
}

/**
 * @brief Tests the frequency of the time-slice interrupt (IRQ0).
 *
 * Asserts that the IRQ0 signal ('0') is emitted exactly after the
 * configured TIME_SLICE interval.
 */
static void
test_iqr_zero(void)
{
    printf("\ttest_iqr_zero... ");

    int fd;
    pid_t pid = start_intercontroller(&fd);
    int status;

    assert(waitpid(pid, &status, WUNTRACED) == pid);

    long long start = current_time_ms();
    kill(pid, SIGCONT);

    IntercontrollerSig signal;
    int result = read_signal(fd, &signal, TIME_SLICE + 250);
    
    long long elapsed = current_time_ms() - start;

    assert(result == 1);
    assert(signal == INTERCONTROLLER_SIG_IRQ0);
    
    // Validate timing boundaries with a 100ms tolerance for OS scheduling
    assert(elapsed >= TIME_SLICE - 100);
    assert(elapsed < TIME_SLICE + 100);

    stop_intercontroller(pid, fd);

    printf("PASS (%lld ms)\n", elapsed);
}

/**
 * @brief Tests the validity and distribution of generated signals.
 *
 * Runs the controller for multiple iterations to ensure it only emits
 * valid signals ('0', '1', or '2'). To effectively verify IRQ1/IRQ2 
 * distribution, the probabilities (PROB_1, PROB_2) in intercontroller.c 
 * should temporarily be increased.
 */
static void
test_valid_signals(void)
{
    printf("\ttest_valid_signals... ");

    int fd;
    pid_t pid = start_intercontroller(&fd);
    int status;

    assert(waitpid(pid, &status, WUNTRACED) == pid);

    int signals[3] = {0, 0, 0};
    kill(pid, SIGCONT);

    for (int i = 0; i < 100; i++) {
        IntercontrollerSig signal;
        int result = read_signal(fd, &signal, TIME_SLICE + 500);

        assert(result == 1);
        assert(signal == INTERCONTROLLER_SIG_IRQ0 || signal == INTERCONTROLLER_SIG_IRQ1 || signal == INTERCONTROLLER_SIG_IRQ2);

        if (signal == INTERCONTROLLER_SIG_IRQ0) signals[0]++;
        else if (signal == INTERCONTROLLER_SIG_IRQ1) signals[1]++;
        else signals[2]++;
    }

    printf("\n");
    for (int i = 0; i < 3; i++) {
        printf("\t\tIRQ%d: %d\n", i, signals[i]);
        assert(signals[i] != 0);
    }

    printf("\t\t\tPASS\n");
    stop_intercontroller(pid, fd);
    
    for (int i = 0; i < 3; i++) {
        printf("\t\tSignal %d: %d occurrences\n", i, signals[i]);
    }
}

/**
 * @brief Main execution entry point for the test suite.
 */
int
main(void)
{
    printf("Running intercontroller tests...\n\n");

    test_starts_stopped();
    test_iqr_zero();
    test_valid_signals();

    printf("\nAll intercontroller tests passed.\n");

    return 0;
}
