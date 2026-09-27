#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 1

// Alterar para variáveis com mesmo nome da struct
typedef enum 
{
    DEBUG_RET_SUCCESS = 0,
    DEBUG_RET_EXEC_ERROR,
    DEBUG_RET_FORK_ERROR,
    DEBUG_RET_WAIT_ERROR,
    DEBUG_RET_PIPE_ERROR,
    DEBUG_RET_FULL_QUEUE
} DebugRet;


// Criar funções de log em cada erro

#endif