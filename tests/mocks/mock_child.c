#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "process/child.h"

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    int read_pipe = atoi(argv[1]);
    int write_pipe = atoi(argv[2]);

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 50000000L; // 50ms (Faster than usual)

    raise(SIGSTOP);

    while (1) {
        nanosleep(&ts, NULL);

        if (rand() % 100 < 15) {
            ChildOp op = (rand() % 2 == 0) ? CHILD_OP_WRITE : CHILD_OP_READ;
            write(write_pipe, &op, sizeof(ChildOp));

            if (op == CHILD_OP_READ) {
                int partner_pc;
                read(read_pipe, &partner_pc, sizeof(partner_pc));
            }
        }
    }
    return 0;
}
