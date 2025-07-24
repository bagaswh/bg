#include <stdio.h>
#include <unistd.h>

int
main(void)
{
    size_t a = -1;
    ssize_t b = -1;
    printf("a=%zu b=%zd %d\n", a, b, ((ssize_t) a) == b);
}