#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <stddef.h>

// Page size
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

// Page flags
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITE      (1 << 1)
#define PAGE_USER       (1 << 2)
#define PAGE_ACCESSED   (1 << 5)
#define PAGE_DIRTY      (1 << 6)
#define PAGE_GLOBAL     (1 << 8)

// Virtual memory areas
#define KERNEL_VIRT_BASE    0xFFFFFFFF80000000ULL
#define USER_VIRT_BASE      0x0000000000400000ULL
#define USER_STACK_TOP      0x00007FFFFFFFE000ULL
#define HEAP_START          0x0000000000800000ULL

// Page table entry
typedef uint64_t pte_t;
typedef uint64_t pde_t;
typedef uint64_t pdpe_t;
typedef uint64_t pml4e_t;

// Page table structures
typedef struct {
    pte_t entries[512];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

typedef struct {
    pde_t entries[512];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

typedef struct {
    pdpe_t entries[512];
} __attribute__((aligned(PAGE_SIZE))) pdp_table_t;

typedef struct {
    pml4e_t entries[512];
} __attribute__((aligned(PAGE_SIZE))) pml4_table_t;

// Virtual memory area
typedef struct vm_area {
    uint64_t start;
    uint64_t end;
    uint32_t flags;
    struct vm_area *next;
} vm_area_t;

// Address space
typedef struct {
    pml4_table_t *pml4;
    vm_area_t *vm_areas;
    uint64_t heap_start;
    uint64_t heap_end;
    uint64_t stack_start;
} address_space_t;

// Physical memory management
typedef struct {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t *bitmap;
    uint64_t bitmap_size;
} pmm_t;

// Swap management
typedef struct {
    uint64_t swap_device;
    uint64_t swap_size;
    uint64_t *swap_bitmap;
    uint64_t swap_pages;
} swap_t;

// Function prototypes
void mm_init(uint64_t mem_size);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
void *vmm_alloc_pages(address_space_t *as, uint64_t vaddr, size_t count, uint32_t flags);
void vmm_free_pages(address_space_t *as, uint64_t vaddr, size_t count);
int vmm_map_page(address_space_t *as, uint64_t vaddr, uint64_t paddr, uint32_t flags);
void vmm_unmap_page(address_space_t *as, uint64_t vaddr);
address_space_t *vmm_create_address_space(void);
void vmm_destroy_address_space(address_space_t *as);
void vmm_switch_address_space(address_space_t *as);
void *kmalloc(size_t size);
void kfree(void *ptr);
void swap_init(uint64_t device, uint64_t size);
int swap_out_page(uint64_t vaddr);
int swap_in_page(uint64_t vaddr, uint64_t swap_offset);

// Page fault handler
void page_fault_handler(uint64_t error_code, uint64_t faulting_address);

// Copy-on-write
int cow_page_fault(address_space_t *as, uint64_t vaddr);

// Memory mapped files
int mmap_file(address_space_t *as, uint64_t vaddr, int fd, uint64_t offset, size_t length);
int munmap(address_space_t *as, uint64_t vaddr, size_t length);

#endif
