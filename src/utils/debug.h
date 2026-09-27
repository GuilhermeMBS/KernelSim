#ifndef DEBUG_H
#define DEBUG_H

// Alterar para variáveis com mesmo nome da struct
typedef enum 
{
    SUCCESS = 0,
    EXEC_ERROR,
    FORK_ERROR,
    WAIT_ERROR,
    PIPE_ERROR
} debug_t;


// Criar funções de log em cada erro

#endif