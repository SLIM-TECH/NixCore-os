#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define HEAP_START 0x100000
#define HEAP_SIZE 0x100000

typedef struct memory_block {
    size_t size;
    bool free;
    struct memory_block* next;
} memory_block_t;

void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* memset(void* dest, int value, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* ptr1, const void* ptr2, size_t count);
size_t memory_get_total(void);
size_t memory_get_used(void);
size_t memory_get_free(void);
size_t memory_get_peak(void);

#endif
