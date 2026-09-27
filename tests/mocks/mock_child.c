#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 4) return 1;
    
    // argv[3] é o write_fd_str mapeado no seu _kernelsim_exec_child
    int write_fd = atoi(argv[3]); 
    int op = 1; // 1 = CHILD_OP_WRITE, 2 = CHILD_OP_READ (Ajuste conforme seu enum)

    while (1) {
        usleep(700000); // Demora um pouco mais para dar tempo de o Kernel agir
        write(write_fd, &op, sizeof(int));
    }
    return 0;
}
