#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Error test a: yield to a non-existent PID
void test_nonexistent_pid(void) {
    printf("=== test_nonexistent_pid ===\n");
    int result = co_yield(99999, 1);
    if (result == -1)
        printf("PASS: co_yield to non-existent PID returned -1\n");
    else
        printf("FAIL: expected -1, got %d\n", result);
    printf("\n");
}

// Error test b: yield to a killed process
void test_killed_process(void) {
    printf("=== test_killed_process ===\n");

    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
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
        if (result == -1)
            printf("PASS: co_yield to killed process returned -1\n");
        else
            printf("FAIL: expected -1, got %d\n", result);
        wait(0);
    }
    printf("\n");
}

// Error test c: yield to self
void test_self_yield(void) {
    printf("=== test_self_yield ===\n");
    int self = getpid();
    int result = co_yield(self, 1);
    if (result == -1)
        printf("PASS: co_yield to self returned -1\n");
    else
        printf("FAIL: expected -1, got %d\n", result);
    printf("\n");
}

void test_invalid_arguments(void) {
    printf("=== test_invalid_arguments ===\n");

    int result = co_yield(0, 1);
    if (result == -1)
        printf("PASS: co_yield to PID 0 returned -1\n");
    else
        printf("FAIL: expected -1 for PID 0, got %d\n", result);

    result = co_yield(99999, -1);
    if (result == -1)
        printf("PASS: co_yield with negative value returned -1\n");
    else
        printf("FAIL: expected -1 for negative value, got %d\n", result);

    printf("\n");
}

int main(void) {
    test_nonexistent_pid();
    test_killed_process();
    test_self_yield();
    test_invalid_arguments();
    printf("all tests done\n");
    exit(0);
}
