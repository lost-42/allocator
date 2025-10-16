#include "alloc.h"

#include <unistd.h>

#include <cassert>
#include <cstring>

char* heap_start = nullptr;

stats* get_malloc_header() {
    return (stats*)heap_start;
}

free_area* find_last_block() {
    free_area* last_block = (free_area*)((char*)heap_start + sizeof(stats));
    while (last_block->next) {
        last_block = last_block->next;
    }
    return last_block;
}

int* add_used_block(size_t size) {
    stats* malloc_header = get_malloc_header();
    while (malloc_header->my_simple_lock)
        sleep(1);
    malloc_header->my_simple_lock = true;

    // reuse from free list
    free_area* block = (free_area*)((char*)heap_start + sizeof(stats));
    free_area* small_block = nullptr;
    free_area* last_block = nullptr;
    while (block != nullptr) {
        assert(block->marker == BLOCK_MARKER);
        if ((block->length + sizeof(free_area)) >= size &&
            block->in_use == false) {
            if (small_block == nullptr || small_block->length > block->length) {
                small_block = block;
            }
        }
        last_block = block;
        block = block->next;
    }

    // extend last free block
    if (small_block == nullptr) {
        free_area* last_block = find_last_block();
        while (last_block->length < size) {
            sbrk(4096);
            last_block->length += 4096;
            malloc_header->amount_of_pages += 1;
        }
        small_block = last_block;
    }

    // create new block
    small_block->in_use = true;
    int must_have_size = small_block->length - size - sizeof(free_area) - 1;
    if (must_have_size <= 0) {
        sbrk(4096);
        malloc_header->amount_of_pages += 1;
        last_block->length += 4096;
        must_have_size = small_block->length - size - sizeof(free_area) - 1;
    }

    int remaining_size = must_have_size + 1;
    malloc_header->amount_of_blocks += 1;

    free_area* new_block =
        (free_area*)((char*)small_block + sizeof(free_area) + size);
    new_block->marker = BLOCK_MARKER;
    new_block->prev = small_block;
    new_block->next = small_block->next;
    if (new_block->next) {
        new_block->next->prev = new_block;
    }
    small_block->next = new_block;
    new_block->length = remaining_size;
    small_block->length = size;
    malloc_header->my_simple_lock = false;
    return (int*)((char*)small_block + sizeof(free_area));
}

int* an_malloc(size_t size) {
    if (heap_start == nullptr) {
        heap_start = (char*)sbrk(0);
        sbrk(4096);
    }
    char* heap_end = (char*)sbrk(0);
    long length = (long)(heap_end - heap_start);

    // init heap
    if (*(heap_start) != MAGICAL_BYTES) {
        *(heap_start) = MAGICAL_BYTES;
        stats* malloc_header = (stats*)heap_start;
        malloc_header->amount_of_blocks = 1;
        malloc_header->amount_of_pages = 1;

        free_area* first_block =
            (free_area*)((char*)heap_start + sizeof(stats));
        first_block->marker = BLOCK_MARKER;
        first_block->prev = nullptr;
        first_block->in_use = false;
        first_block->length = length - sizeof(stats) - sizeof(free_area);
        first_block->next = nullptr;
    }

    return add_used_block(size);
}

free_area* find_previous_used_block(free_area* block) {
    free_area* prev_block = block->prev;
    while (prev_block != nullptr) {
        if (prev_block->in_use == false)
            return prev_block;
        prev_block = prev_block->prev;
    }
    return nullptr;
}

void reduce_heap_size_if_possible() {
    free_area* last_block = find_last_block();
    free_area* prev_block = find_previous_used_block(last_block);
    if (prev_block == nullptr) {
        if (last_block->length > 4096) {
            last_block->length = 4096;
        }
        prev_block = last_block;
    }

    void* new_end = (char*)prev_block + sizeof(free_area) + prev_block->length;
    void* heap_end = sbrk(0);
    while (new_end < (char*)heap_end - 4096) {
        sbrk(-4096);
        heap_end = sbrk(0);
        get_malloc_header()->amount_of_pages -= 1;
    }

    if ((char*)heap_end - (char*)new_end > sizeof(free_area) + 1) {
        free_area* new_unused_block = (free_area*)new_end;
        new_unused_block->in_use = false;
        new_unused_block->marker = BLOCK_MARKER;
        new_unused_block->next = nullptr;
        new_unused_block->prev = prev_block;
        new_unused_block->length =
            (char*)heap_end - (char*)new_end - sizeof(free_area);
    }
}

bool an_free(void* ptr) {
    stats* malloc_header = get_malloc_header();
    while (malloc_header->my_simple_lock) {
        sleep(1);
    }
    malloc_header->my_simple_lock = true;

    free_area* block = (free_area*)((char*)ptr - sizeof(free_area));
    assert(block->marker == BLOCK_MARKER);
    block->in_use = false;
    memset(ptr, 0, block->length);

    // if next block is also not in use, merge with it
    if (block->next != nullptr && block->next->in_use == false) {
        free_area* not_used_next_block = block->next;
        block->next = not_used_next_block->next;
        if (not_used_next_block->next != nullptr) {
            not_used_next_block->next->prev = block;
        }
        block->length += sizeof(free_area) + not_used_next_block->length;
        memset((void*)not_used_next_block, 0,
               sizeof(free_area) + not_used_next_block->length);
        malloc_header->amount_of_blocks -= 1;
    }

    // if prev block is also not in use, merge with it
    if (block->prev != nullptr && block->prev->in_use == false) {
        free_area* prev_block = block->prev;
        prev_block->next = block->next;
        if (block->next != nullptr) {
            block->next->prev = prev_block;
        }

        prev_block->length += sizeof(free_area) + block->length;
        malloc_header->amount_of_blocks -= 1;
        reduce_heap_size_if_possible();
    }

    malloc_header->my_simple_lock = false;
    return false;
}
