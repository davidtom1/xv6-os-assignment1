#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_ITERS 5

// Basic coroutine test: parent and child alternate via co_yield,
// passing values back and forth. Parent sends 2, child sends 1.
void test_basic(void) {
    printf("=== test_basic ===\n");

    int pid1 = getpid(); // parent PID
    int pid2 = fork();

    if (pid2 < 0) {
        printf("fork failed\n");
        exit(1);
    }

    if (pid2 == 0) {
        // Child: yield to parent NUM_ITERS times
        for (int i = 0; i < NUM_ITERS; i++) {
            int value = co_yield(pid1, 1);
            printf("child received: %d\n", value); // should print 2
        }
        exit(0);
    } else {
        // Parent: yield to child NUM_ITERS times
        for (int i = 0; i < NUM_ITERS; i++) {
            int value = co_yield(pid2, 2);
            printf("parent received: %d\n", value); // should print 1
        }
        wait(0);
    }
    printf("test_basic done\n\n");
}

int main(void) {
    test_basic();
    exit(0);
}
