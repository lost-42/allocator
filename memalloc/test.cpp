#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

#include "alloc.h"

void unit_test(void (*test_func)(), const char* msg) {
    pid_t pid = fork();
    if (pid == 0) {
        test_func();
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFSIGNALED(status)) {
            std::cout << msg << " crashed with signal;" << status << std::endl;
        } else {
            std::cout << msg << " passed\n";
        }
    }
}

void test_basic() {
    int* p = an_malloc(sizeof(int));
    stats* heap_header = get_malloc_header();
    assert(heap_header->amount_of_blocks == 2);
    assert(heap_header->amount_of_pages == 1);

    *p = 10;
    assert(*p == 10);
}

void test_malloc_more() {
    void* p = an_malloc(5000);
    stats* heap_header = get_malloc_header();
    assert(heap_header->amount_of_pages == 2);
}

void test_basic_free() {
    void* p = an_malloc(1000);
    stats* heap_header = get_malloc_header();
    assert(heap_header->amount_of_blocks == 2);
    assert(heap_header->amount_of_pages == 1);

    an_free(p);
    assert(heap_header->amount_of_blocks == 1);
    assert(heap_header->amount_of_pages == 1);
}

void test_malloc_in_middle() {
    void* p1 = an_malloc(100);
    stats* heap_header = get_malloc_header();
    assert(heap_header->amount_of_blocks == 2);
    assert(heap_header->amount_of_pages == 1);

    void* p2 = an_malloc(100);
    void* p3 = an_malloc(100);

    an_free(p2);
    p2 = an_malloc(50);
    assert(heap_header->amount_of_blocks == 5);
    assert(heap_header->amount_of_pages == 1);
    assert(((char*)p2 - (char*)p1) / sizeof(char) == 100 + sizeof(free_area));
}

int main() {
    unit_test(test_basic, "basic test");
    unit_test(test_malloc_more, "malloc more");
    unit_test(test_basic_free, "basic free");
    unit_test(test_malloc_in_middle, "malloc in middle");
}