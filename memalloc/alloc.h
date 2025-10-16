#include <cstddef>
#include <cstdint>

inline constexpr uint8_t MAGICAL_BYTES = 0x55;
inline constexpr uint8_t BLOCK_MARKER = 0xDD;

struct free_area {
    uint8_t marker;
    free_area* prev;
    bool in_use;
    uint32_t length;
    free_area* next;
};

struct stats {
    int magical_bytes;
    bool my_simple_lock;
    uint32_t amount_of_blocks;
    uint64_t amount_of_pages;
};

int* an_malloc(size_t size);
bool an_free(void* ptr);
stats* get_malloc_header();