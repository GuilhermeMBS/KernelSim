/**
 * @file debug.h
 * @brief Debugging utilities, error codes, and state visualization.
 *
 * This header defines system-wide error codes and provides X-Macros to 
 * dynamically generate the layout for the process state visualization table.
 */

#ifndef DEBUG_H
#define DEBUG_H

/**
 * @brief Global debug flag.
 * Set to 1 to enable debugging outputs and state tables, 0 to disable.
 */
#define DEBUG 1

/**
 * @brief X-Macro list for the debug table columns.
 * 
 * Defines the layout of the state visualization table.
 * Format: X(ID, "Column Title", width_in_characters)
 */
#define PRINT_COLS_TABLE                                 \
    X(CHILD,          "CHILD",           6)              \
    X(PID,            "PID",             8)              \
    X(PC,             "PC",              6)              \
    X(N,              "N",               6)              \
    X(STATE,          "STATE",          10)              \
    X(READ_SYSCALLS,  "READ SYSCALLS",  16)              \
    X(WRITE_SYSCALLS, "WRITE SYSCALLS", 16)

/**
 * @brief Calculates the total width of the table.
 * 
 * Uses the X-Macro list to sum up all column widths at compile time.
 */
#define X(id, name, size) + (size)
enum { TOTAL_TABLE_WIDTH = 0 PRINT_COLS_TABLE };
#undef X

/**
 * @brief Enumeration of table column widths.
 * 
 * Expands the X-Macro list to create specific constants for each column's 
 * width (e.g., DEBUG_COL_WIDTH_PID).
 */
typedef enum 
{
#define X(id, name, size) DEBUG_COL_WIDTH_##id = (size),
    PRINT_COLS_TABLE
#undef X
} DebugColWidth;

/**
 * @brief Standardized system return codes.
 * 
 * Used across the kernel and child processes to indicate success 
 * or specific points of failure.
 */
typedef enum 
{
    DEBUG_RET_SUCCESS = 0,      // Operation completed successfully
    DEBUG_RET_EXEC_ERROR,       // Failed to execute a binary (execl)
    DEBUG_RET_FORK_ERROR,       // Failed to fork a new process
    DEBUG_RET_WAIT_ERROR,       // Failed to wait for a child process
    DEBUG_RET_PIPE_ERROR,       // Failed to create or configure a pipe
    DEBUG_RET_FULL_QUEUE,       // Attempted to insert into a full queue
    DEBUG_RET_EMPTY_QUEUE,      // Attempted to read from an empty queue
    DEBUG_RET_ERR_SYSCALL,      // Error during a simulated system call
    DEBUG_RET_ERR_SHM           // Failed to attach or allocate shared memory
} DebugRet;

/**
 * @brief Prints the table separator line.
 * 
 * Generates a dashed line matching the TOTAL_TABLE_WIDTH to separate 
 * table headers and rows.
 */
void debug_print_table_separator(void);

/**
 * @brief Prints the formatted table header.
 * 
 * Uses the PRINT_COLS_TABLE X-Macro to dynamically print the column titles 
 * with their corresponding widths.
 */
void debug_print_table_header(void);

#endif /* DEBUG_H */
