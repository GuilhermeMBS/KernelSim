/**
 * @file debug.c
 * @brief Implementation of debugging utilities and state visualization.
 *
 * Provides the implementations for generating the formatted console 
 * table used to display the internal state of the simulated processes.
 */

#include <stdio.h>

#include "utils/debug.h"

void debug_print_table_separator(void)
{
    // Utilizes the pre-calculated TOTAL_TABLE_WIDTH from the header
    for (int i = 0; i < TOTAL_TABLE_WIDTH; i++) {
        putchar('-');
    }
    putchar('\n');
}

void debug_print_table_header(void)
{
    debug_print_table_separator();

    // Expands the X-Macro to print each column name with its specified width
    #define X(id, name, size) printf("%-*s", (size), (name));
    PRINT_COLS_TABLE
    #undef X
    
    putchar('\n');

    debug_print_table_separator();
}
