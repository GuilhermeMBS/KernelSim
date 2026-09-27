#include <stdio.h>

#include "debug.h"


static inline void
debug_print_table_separator(void)
{
    int total_width = 0;
    #define X(id, name, size) total_width += (size);
    PRINT_COLS_TABLE
    #undef X

    for (int i = 0; i < total_width; i++) putchar('-');
    putchar('\n');
}


static inline void
debug_print_table_header(void)
{
    print_table_separator();

    #define X(id, name, size) printf("%-*s", (size), (name));
    PRINT_COLS_TABLE
    #undef X
    putchar('\n');

    print_table_separator();
}