#ifndef RETCODE_H
#define RETCODE_H

// Alterar para variáveis com mesmo nome da struct
typedef enum 
{
    SUCCESS = 0,
    EXEC_ERROR,
    FORK_ERROR,
    WAIT_ERROR,
    PIPE_ERROR
} retcode_t;


// Criar funções de log em cada erro

#endif