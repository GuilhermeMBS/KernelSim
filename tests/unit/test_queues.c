/**
 * @file test_queues.c
 * @brief Unit tests for the circular queue implementation.
 *
 * This test suite validates the initialization, insertion, retrieval, 
 * overflow (full), and underflow (empty) behaviors of the queue structure.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils/queue.h"
#include "utils/debug.h"

/**
 * @brief Tests basic queue initialization properties.
 */
static void test_create_queue(void)
{
    printf("\ttest_create_queue... ");

    QUEUE_INIT(test, 5);

    assert(test.size == 5);
    assert(test.start == 0);
    assert(test.end == 0);
    assert(test.qtd == 0);

    printf("PASS\n");
}

/**
 * @brief Tests the insertion of a single element.
 */
static void test_queue_put_single_element(void)
{
    printf("\ttest_queue_put_single_element... ");

    QUEUE_INIT(test, 5);

    assert(queue_put(&test, 10) == DEBUG_RET_SUCCESS);

    assert(test.buffer[0] == 10); 
    assert(test.start == 0); 
    assert(test.end == 1);
    assert(test.qtd == 1);

    printf("PASS\n");
}

/**
 * @brief Tests the sequential insertion of multiple elements.
 */
static void test_queue_put_multiple_elements(void)
{
    printf("\ttest_queue_put_multiple_elements... ");

    QUEUE_INIT(test, 5);

    int values[] = {10, 6, 3, 2};
    int qtd = sizeof(values) / sizeof(int);
    
    for (int i = 0; i < qtd; i++) {
        assert(queue_put(&test, values[i]) == DEBUG_RET_SUCCESS);
        for (int j = 0; j <= i; j++) {
            assert(test.buffer[j] == values[j]);
        }
    }
    
    assert(test.start == 0); 
    assert(test.end == 4);
    assert(test.qtd == 4);

    printf("PASS\n");
}

/**
 * @brief Tests the retrieval of a single element.
 */
static void test_queue_get_single_element(void)
{
    printf("\ttest_queue_get_single_element... ");

    QUEUE_INIT(test, 5);
    queue_put(&test, 5); 
    queue_put(&test, 3);

    assert(queue_get(&test) == 5); 
    assert(test.start == 1); 
    assert(test.end == 2);
    assert(test.qtd == 1);

    printf("PASS\n");
}

/**
 * @brief Tests the sequential retrieval of multiple elements and wrap-around.
 */
static void test_queue_get_multiple_elements(void)
{
    printf("\ttest_queue_get_multiple_elements... ");

    QUEUE_INIT(test, 5);
    int values[] = {1, 3, 5, 10};
    int qtd = sizeof(values) / sizeof(int);
    
    for (int i = 0; i < qtd; i++) {
        queue_put(&test, values[i]);
    }

    for (int i = 0; i < qtd; i++) {
        assert(queue_get(&test) == values[i]);
        assert(test.start == 1 + i);
    }

    // Trigger circular wrap-around
    queue_put(&test, 999);
    assert(test.end == 0);
    assert(test.qtd == 1);

    printf("PASS\n");
}

/**
 * @brief Tests overflow prevention when attempting to insert into a full queue.
 */
static void test_queue_full(void)
{
    printf("\ttest_queue_full...\n\t\t");

    QUEUE_INIT(test, 3);
    int values[] = {1, 3, 5};
    int qtd = sizeof(values) / sizeof(int);
    
    for (int i = 0; i < qtd; i++) {
        queue_put(&test, values[i]);
    }

    // Attempt to overflow
    assert(queue_put(&test, 7) == DEBUG_RET_FULL_QUEUE);
    assert(test.qtd == 3);
    
    // Drain queue to verify integrity
    for (int i = 0 ; i < qtd; i++) {
        assert(queue_get(&test) == values[i]);
    }
    
    assert(test.qtd == 0);

    printf("\t\tPASS\n");
}

/**
 * @brief Tests underflow prevention when attempting to read from an empty queue.
 */
static void test_queue_empty(void)
{
    printf("\ttest_queue_empty...\n\t\t");

    QUEUE_INIT(test, 3);
    int values[] = {1, 3, 5};
    int qtd = sizeof(values) / sizeof(int);
    
    for (int i = 0; i < qtd; i++) {
        queue_put(&test, values[i]);
    }

    // Drain queue
    queue_get(&test); 
    queue_get(&test); 
    queue_get(&test);

    // Attempt to underflow
    assert(queue_get(&test) == -1);
    assert(test.qtd == 0);

    printf("\t\tPASS\n");
}


int main(void)
{
    printf("Running queue tests...\n\n");

    test_create_queue();
    test_queue_put_single_element();
    test_queue_put_multiple_elements();
    test_queue_get_single_element();
    test_queue_get_multiple_elements();
    test_queue_full();
    test_queue_empty();

    printf("\nAll queue tests passed.\n");

    return 0;
}
