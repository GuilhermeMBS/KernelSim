#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "process/child.h"

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    int read_pipe = atoi(argv[1]);
    int write_pipe = atoi(argv[2]);

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 50000000L; // 50ms para andar mais rápido que o time-slice

    while (1) {
        nanosleep(&ts, NULL);

        // Intercala um WRITE e um READ aleatoriamente para testar ambos (OP1 e OP2)
        ChildOp op = (rand() % 2 == 0) ? CHILD_OP_WRITE : CHILD_OP_READ;
        write(write_pipe, &op, sizeof(ChildOp));

        // Se for um READ (OP2), ele simula o processo real e bloqueia esperando a resposta!
        if (op == CHILD_OP_READ) {
            int dummy_n;
            read(read_pipe, &dummy_n, sizeof(dummy_n));
        }
    }
    return 0;
}
