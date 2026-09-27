#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils/queue.h"
#include "utils/debug.h"


static void
test_create_queue()
{
    printf("\ttest_Create_queue...");

    QUEUE_INIT(test, 5);

    assert(test.size == 5);
    assert(test.start == 0);
    assert(test.end == 0);
    assert(test.qtd == 0);

    printf(" PASS\n");
}

static void
test_queue_put_1Element()
{
    printf("\ttest_queue_put_1Element...");

    QUEUE_INIT(test, 5);

    assert(queue_put(&test, 10) == DEBUG_RET_SUCCESS);

    assert(test.buffer[0] == 10); 
    assert(test.start == 0); assert(test.end == 1);
    assert(test.qtd == 1);

    printf(" PASS\n");
}

static void
test_queue_put_Elements()
{
    printf("\ttest_queue_put_Elements...");

    QUEUE_INIT(test, 5);

    int values[] = {10, 6, 3, 2};
    int qtd = sizeof(values)/ sizeof(int);
    for (int i = 0; i < qtd; i++)
    {
        assert(queue_put(&test, values[i]) == DEBUG_RET_SUCCESS);
        for (int j = 0; j <= i; j++) assert(test.buffer[j] == values[j]);
    }
    assert(test.start == 0); assert(test.end == 4);
    assert(test.qtd == 4);

    printf(" PASS\n");
}


static void
test_queue_get_1Element()
{
    printf("\ttest_queue_get_1Element...");

    QUEUE_INIT(test, 5);
    queue_put(&test, 5); queue_put(&test, 3);

    assert(queue_get(&test) == 5); 
    assert(test.start == 1); assert(test.end == 2);
    assert(test.qtd == 1);

    printf(" PASS\n");
}


static void
test_queue_get_Elements()
{
    printf("\ttest_queue_get_Elements...");

    QUEUE_INIT(test, 5);
    int values[] = {1, 3, 5, 10};
    int qtd = sizeof(values) / sizeof(int);
    for (int i = 0; i < qtd; i++) queue_put(&test, values[i]);

    for(int i = 0; i < qtd; i++)
    {
        assert(queue_get(&test) == values[i]);
        assert(test.start == 1 + i);
    }

    queue_put(&test, 999);
    assert(test.end == 0);
    assert(test.qtd == 1);

    printf(" PASS\n");
}


static void
test_queue_full()
{
    printf("\ttest_queue_full...\n\t\t");

    QUEUE_INIT(test, 3);
    int values[] = {1, 3, 5};
    int qtd = sizeof(values) / sizeof(int);
    for (int i = 0; i < qtd; i++) queue_put(&test, values[i]);

    assert(queue_put(&test, 7) == DEBUG_RET_FULL_QUEUE);
    
    assert(test.qtd == 3);
    for (int i = 0 ; i < qtd; i++)
        assert(queue_get(&test) == values[i]);
    assert(test.qtd == 0);


    printf("\n\t\t\t  PASS\n");
}


static void
test_queue_empty()
{
    printf("\ttest_queue_empty...\n\t\t");

    QUEUE_INIT(test, 3);
    int values[] = {1, 3, 5};
    int qtd = sizeof(values) / sizeof(int);
    for (int i = 0; i < qtd; i++) queue_put(&test, values[i]);

    queue_get(&test); queue_get(&test); queue_get(&test);

    assert(queue_get(&test) == -1);
    assert(test.qtd == 0);

    printf("\n\t\t\t  PASS\n");
}


int
main(void)
{
    printf("Running queue tests...\n\n");

    test_create_queue();
    test_queue_put_1Element();
    test_queue_put_Elements();
    test_queue_get_1Element();
    test_queue_get_Elements();
    test_queue_full();
    test_queue_empty();

    printf("\nAll queue tests passed\n");

    return 0;
}