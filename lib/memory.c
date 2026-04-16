#include "../include/memory.h"

static memory_block_t* heap_start = NULL;
static size_t memory_used = 0;
static size_t memory_peak = 0;

void memory_init() {
    heap_start = (memory_block_t*)HEAP_START;
    heap_start->size = HEAP_SIZE - sizeof(memory_block_t);
    heap_start->free = true;
    heap_start->next = NULL;
    memory_used = 0;
    memory_peak = 0;
}

void* kmalloc(size_t size) {
    size = (size + 3) & ~3;
    
    memory_block_t* current = heap_start;
    memory_block_t* best_fit = NULL;
    size_t best_size = 0xFFFFFFFF;
    
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size < best_size) {
                best_size = current->size;
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    if (!best_fit)
        return NULL;
    
    if (best_fit->size >= size + sizeof(memory_block_t) + 4) {
        memory_block_t* new_block = (memory_block_t*)((char*)best_fit + sizeof(memory_block_t) + size);
        new_block->size = best_fit->size - size - sizeof(memory_block_t);
        new_block->free = true;
        new_block->next = best_fit->next;
        
        best_fit->size = size;
        best_fit->next = new_block;
    }
    
    best_fit->free = false;
    memory_used += best_fit->size;
    if (memory_used > memory_peak) {
        memory_peak = memory_used;
    }
    
    return (void*)((char*)best_fit + sizeof(memory_block_t));
}

void kfree(void* ptr) {
    if (!ptr)
        return;
    
    memory_block_t* block = (memory_block_t*)((char*)ptr - sizeof(memory_block_t));
    if (!block->free && memory_used >= block->size) {
        memory_used -= block->size;
    }
    block->free = true;
    
    if (block->next && block->next->free) {
        block->size += sizeof(memory_block_t) + block->next->size;
        block->next = block->next->next;
    }
    
    memory_block_t* current = heap_start;
    while (current && current->next != block)
        current = current->next;
    
    if (current && current->free) {
        current->size += sizeof(memory_block_t) + block->size;
        current->next = block->next;
    }
}

void* memset(void* dest, int value, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    for (size_t i = 0; i < count; i++)
        d[i] = (unsigned char)value;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < count; i++)
        d[i] = s[i];
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    for (size_t i = 0; i < count; i++) {
        if (p1[i] != p2[i])
            return p1[i] - p2[i];
    }
    
    return 0;
}

size_t memory_get_total(void) {
    return HEAP_SIZE;
}

size_t memory_get_used(void) {
    return memory_used;
}

size_t memory_get_free(void) {
    return HEAP_SIZE - memory_used;
}

size_t memory_get_peak(void) {
    return memory_peak;
}
