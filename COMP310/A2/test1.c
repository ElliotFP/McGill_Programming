#include "sut.h"
#include <stdio.h>

void hello1()
{
    int i;
    for (i = 0; i < 100; i++)
    {
        printf("Hello world!, this is SUT-One \n");
        sut_yield();
        printf("%d \n", i);
    }
    printf("hello 1 done \n");
    sut_exit();
}

void hello2()
{
    int i;
    for (i = 0; i < 100; i++)
    {
        printf("Hello world!, this is SUT-Two and my code finally works, let's goooooooooooooo\n");
        sut_yield();
        printf("%d \n", i);
    }
    printf("hello 2 done \n");
    sut_exit();
}

int main()
{
    sut_init();
    sut_create(hello1);
    // printf("1 2 3\n");
    sut_create(hello2);
    while (1)
    {
    };
    sut_shutdown();
    return 0;
}
