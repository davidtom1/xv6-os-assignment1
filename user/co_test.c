#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Error test a: yield to a non-existent PID
void test_nonexistent_pid(void) {
    printf("=== test_nonexistent_pid ===\n");
    int result = co_yield(99999, 1);
    if (result == -1)
        printf("PASS: co_yield to non-existent PID returned -1\n\n");
    else {
        printf("FAIL: expected -1, got %d\n", result);
        exit(1);
    }
}

// Error test b: yield to a killed process
void test_killed_process(void) {
    printf("=== test_killed_process ===\n");

    int pid = fork();
    if (pid < 0) {
        printf("FAIL: fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        // Child: just sleep so parent can kill it
        sleep(100);
        exit(0);
    } else {
        // Parent: kill the child then try to co_yield to it
        kill(pid);
        // Give the scheduler a moment to process the kill
        sleep(1);
        int result = co_yield(pid, 1);
        wait(0);
        if (result == -1)
            printf("PASS: co_yield to killed process returned -1\n\n");
        else {
            printf("FAIL: expected -1, got %d\n", result);
            exit(1);
        }
    }
}

// Error test c: yield to self
void test_self_yield(void) {
    printf("=== test_self_yield ===\n");
    int self = getpid();
    int result = co_yield(self, 1);
    if (result == -1)
        printf("PASS: co_yield to self returned -1\n\n");
    else {
        printf("FAIL: expected -1, got %d\n", result);
        exit(1);
    }
}

// Error test d: invalid arguments
void test_invalid_arguments(void) {
    printf("=== test_invalid_arguments ===\n");

    int result = co_yield(0, 1);
    if (result == -1)
        printf("PASS: co_yield to PID 0 returned -1\n");
    else {
        printf("FAIL: expected -1 for PID 0, got %d\n", result);
        exit(1);
    }

    result = co_yield(99999, -1);
    if (result == -1)
        printf("PASS: co_yield with negative value returned -1\n\n");
    else {
        printf("FAIL: expected -1 for negative value, got %d\n", result);
        exit(1);
    }
}

int main(void) {
    // Error/edge-case tests first (finite running time)
    // Any failure calls exit(1) and prevents reaching the infinite loop below
    test_nonexistent_pid();
    test_killed_process();
    test_self_yield();
    test_invalid_arguments();
    printf("all error tests done\n\n");

    // Main coroutine test from the assignment (infinite loop)
    printf("=== coroutine test (Ctrl-A X to exit) ===\n");
    int pid1 = getpid(); // Parent PID
    int pid2 = fork();   // Child PID

    if (pid2 == 0) { // Child
        for (;;) {
            int value = co_yield(pid1, 1);
            printf("Child received: %d\n", value); // Should print 2
        }
    } else { // Parent
        for (;;) {
            int value = co_yield(pid2, 2);
            printf("parent received: %d\n", value); // Should print 1
        }
    }

    exit(0);
}
