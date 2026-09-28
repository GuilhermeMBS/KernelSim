/**
 * @file queue.h
 * @brief Circular queue data structure and utilities.
 *
 * This header defines a static circular queue (FIFO) designed to manage 
 * integer elements, such as process IDs or ready queue indexes. It provides 
 * a macro for static memory allocation, eliminating the need for dynamic 
 * memory management (malloc/free).
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "utils/debug.h"

/**
 * @brief Circular queue structure.
 * 
 * Manages an array-based circular buffer with pointers for the start 
 * and end positions, along with capacity constraints.
 */
typedef struct
{
    int* buffer; /**< Pointer to the underlying integer array */
    int start;   /**< Index of the front element */
    int end;     /**< Index of the next available insertion slot */
    int size;    /**< Maximum capacity of the queue */
    int qtd;     /**< Current number of elements in the queue */
} queue_t;

/**
 * @brief Macro implementation for queue initialization.
 * 
 * Generates a block-scoped or file-scoped static array and initializes 
 * the queue_t structure to point to it.
 */
#define _QUEUE_INIT_IMPL(name, q_size)      \
    static int _##name##_buffer[q_size];    \
    static queue_t name = {                 \
        .buffer = _##name##_buffer,         \
        .start  = 0,                        \
        .end    = 0,                        \
        .size   = q_size,                   \
        .qtd    = 0                         \
    }

/**
 * @brief Initializes a new static circular queue.
 * 
 * This macro handles the complete creation of a queue by first allocating 
 * a static integer array of the specified size to act as the underlying buffer. 
 * It then initializes a queue_t structure with the provided name and binds 
 * the allocated buffer to it, setting the initial states (start, end, qtd) to zero.
 * 
 * @note This is a convenience macro designed specifically for generating queues 
 * backed by integer buffers.
 * 
 * @param name The name of the queue_t variable to be created.
 * @param q_size The maximum number of elements the queue can hold.
 */
#define QUEUE_INIT(name, q_size) _QUEUE_INIT_IMPL(name, q_size)

/**
 * @brief Inserts an element into the back of the queue.
 * 
 * @param q Pointer to the queue structure.
 * @param e The integer element to be inserted.
 * @return DEBUG_RET_SUCCESS on success, DEBUG_RET_FULL_QUEUE if full.
 */
DebugRet
queue_put(queue_t *q, int e);

/**
 * @brief Removes and returns the element at the front of the queue.
 * 
 * @param q Pointer to the queue structure.
 * @return The retrieved integer, or -1 if the queue is empty.
 */
DebugRet
queue_get(queue_t *q);

#endif /* QUEUE_H */
