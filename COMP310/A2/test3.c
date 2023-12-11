#include "sut.h"
#include <stdio.h>

void hello0()
{
    int i;
    for (i = 0; i < 100; i++)
    {
        printf("Hello world!, this is SUT-Zero : %d \n", i);
        sut_yield();
    }
    sut_exit();
}

void hello1()
{
    int i;
    for (i = 0; i < 100; i++)
    {
        printf("Hello world!, this is SUT-One : %d \n", i);
        sut_yield();
    }
    sut_exit();
}

void hello2()
{
    int i;
    for (i = 0; i < 100; i++)
    {
        printf("Hello world!, this is SUT-Two : %d \n", i);
        sut_yield();
    }
    sut_exit();
}

void hello3()
{
    int i;
    for (i = 0; i < 10; i++)
    {
        printf("Hello world!, this is SUT-Three : %d \n", i);
        sut_yield();
        sut_create(hello0);
    }
    sut_exit();
}

int main()
{
    printf("Hello world!, this is SUT-Main \n");
    sut_init();
    sut_create(hello1);
    sut_create(hello2);
    sut_create(hello3);
    while (1)
    {
    }

    sut_shutdown();
}
