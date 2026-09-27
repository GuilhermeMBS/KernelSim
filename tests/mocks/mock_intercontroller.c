#include <stdio.h>
#include <unistd.h>

int main(void) {
    while (1) {
        usleep(500000); // 500ms
        printf("0");    // Envia IRQ0
        fflush(stdout);
    }
    return 0;
}
