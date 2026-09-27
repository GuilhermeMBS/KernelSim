#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 1

#define PRINT_COLS_TABLE                        \
    X(CHILD,          "CHILD",          6)      \
    X(PID,            "PID",            8)      \
    X(PC,             "PC",             6)      \
    X(N,              "N",              6)      \
    X(STATE,          "STATE",         10)      \
    X(READ_SYSCALLS,  "READ SYSCALLS", 16)      \
    X(WRITE_SYSCALLS, "WRITE SYSCALLS", 16)

#define TOTAL_TABLE_WIDTH (0                    \
    #define X(name, size) + (size)              \
    PRINT_COLS_TABLE                            \
    #undef X                                    \
)

typedef enum {
#define X(id, name, size) DEBUG_COL_WIDTH_##id = (size),
    PRINT_COLS_TABLE
#undef X
} DebugColWidth;


typedef enum 
{
    DEBUG_RET_SUCCESS = 0,
    DEBUG_RET_EXEC_ERROR,
    DEBUG_RET_FORK_ERROR,
    DEBUG_RET_WAIT_ERROR,
    DEBUG_RET_PIPE_ERROR,
    DEBUG_RET_FULL_QUEUE,
    DEBUG_RET_ERR_SYSCALL,
    DEBUG_RET_ERR_SHM
} DebugRet;


static inline void
debug_print_table_separator();

static inline void
debug_print_table_header();

#endif