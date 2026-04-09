#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    printf("memsize: %d\n", memsize());
    int* x = malloc(20000);
    printf("memsize after malloc: %d\n", memsize());
    free(x);
    printf("memsize after free: %d\n", memsize());
    exit(0);
}
