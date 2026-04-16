#include "mm.h"
#include <stdint.h>
#include <stddef.h>

// Physical memory manager
static pmm_t pmm;
static uint64_t next_free_page = 0x100000; // Start at 1MB

// Kernel page tables
static pml4_table_t *kernel_pml4;

void mm_init(uint64_t mem_size) {
    // Initialize physical memory manager
    pmm.total_pages = mem_size / PAGE_SIZE;
    pmm.free_pages = pmm.total_pages;
    pmm.bitmap_size = (pmm.total_pages + 63) / 64;
    pmm.bitmap = (uint64_t*)0x10000; // Bitmap at 64KB

    // Clear bitmap
    for (uint64_t i = 0; i < pmm.bitmap_size; i++) {
        pmm.bitmap[i] = 0;
    }

    // Mark first 1MB as used
    for (uint64_t i = 0; i < 256; i++) {
        pmm.bitmap[i / 64] |= (1ULL << (i % 64));
        pmm.free_pages--;
    }

    // Create kernel page tables
    kernel_pml4 = (pml4_table_t*)pmm_alloc_page();

    // Identity map first 4GB
    for (uint64_t addr = 0; addr < 0x100000000ULL; addr += 0x200000) {
        vmm_map_page(NULL, addr, addr, PAGE_PRESENT | PAGE_WRITE);
    }

    // Load CR3
    __asm__ volatile("mov %0, %%cr3" :: "r"(kernel_pml4));
}

void *pmm_alloc_page(void) {
    if (pmm.free_pages == 0) return NULL;

    // Find free page in bitmap
    for (uint64_t i = 0; i < pmm.bitmap_size; i++) {
        if (pmm.bitmap[i] != 0xFFFFFFFFFFFFFFFFULL) {
            for (int j = 0; j < 64; j++) {
                if (!(pmm.bitmap[i] & (1ULL << j))) {
                    pmm.bitmap[i] |= (1ULL << j);
                    pmm.free_pages--;
                    uint64_t page = (i * 64 + j) * PAGE_SIZE;

                    // Clear page
                    uint64_t *ptr = (uint64_t*)page;
                    for (int k = 0; k < 512; k++) ptr[k] = 0;

                    return (void*)page;
                }
            }
        }
    }

    return NULL;
}

void pmm_free_page(void *page) {
    uint64_t addr = (uint64_t)page;
    uint64_t page_num = addr / PAGE_SIZE;

    pmm.bitmap[page_num / 64] &= ~(1ULL << (page_num % 64));
    pmm.free_pages++;
}

int vmm_map_page(address_space_t *as, uint64_t vaddr, uint64_t paddr, uint32_t flags) {
    pml4_table_t *pml4 = as ? as->pml4 : kernel_pml4;

    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdp_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint64_t pt_idx = (vaddr >> 12) & 0x1FF;

    // Get or create PDP table
    if (!(pml4->entries[pml4_idx] & PAGE_PRESENT)) {
        void *pdp = pmm_alloc_page();
        if (!pdp) return -1;
        pml4->entries[pml4_idx] = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    pdp_table_t *pdp = (pdp_table_t*)(pml4->entries[pml4_idx] & ~0xFFF);

    // Get or create PD table
    if (!(pdp->entries[pdp_idx] & PAGE_PRESENT)) {
        void *pd = pmm_alloc_page();
        if (!pd) return -1;
        pdp->entries[pdp_idx] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    page_directory_t *pd = (page_directory_t*)(pdp->entries[pdp_idx] & ~0xFFF);

    // Get or create PT table
    if (!(pd->entries[pd_idx] & PAGE_PRESENT)) {
        void *pt = pmm_alloc_page();
        if (!pt) return -1;
        pd->entries[pd_idx] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    page_table_t *pt = (page_table_t*)(pd->entries[pd_idx] & ~0xFFF);

    // Map page
    pt->entries[pt_idx] = paddr | flags;

    // Flush TLB
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");

    return 0;
}

void *kmalloc(size_t size) {
    // Simple bump allocator for now
    static uint64_t heap_ptr = 0x200000; // Start at 2MB

    size = (size + 15) & ~15; // Align to 16 bytes
    void *ptr = (void*)heap_ptr;
    heap_ptr += size;

    return ptr;
}

void kfree(void *ptr) {
    // TODO: Implement proper free
}

void page_fault_handler(uint64_t error_code, uint64_t faulting_address) {
    // Check if it's a COW fault
    if (error_code & 0x2) { // Write fault
        // TODO: Handle COW
    }

    // Check if it's a demand paging fault
    if (!(error_code & 0x1)) { // Not present
        // Allocate page
        void *page = pmm_alloc_page();
        if (page) {
            vmm_map_page(NULL, faulting_address & ~0xFFF, (uint64_t)page,
                        PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            return;
        }
    }

    // Unhandled page fault - panic
    __asm__ volatile("cli; hlt");
}
