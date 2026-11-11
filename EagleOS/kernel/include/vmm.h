#ifndef EAGLEOS_VMM_H
#define EAGLEOS_VMM_H

#include <stdint.h>

#define VMM_FAILURE UINT64_MAX

#define VMM_PAGE_SIZE 4096ULL

#define VMM_PAGE_PRESENT       (1ULL << 0)
#define VMM_PAGE_WRITABLE      (1ULL << 1)
#define VMM_PAGE_USER          (1ULL << 2)
#define VMM_PAGE_WRITE_THROUGH (1ULL << 3)
#define VMM_PAGE_CACHE_DISABLE (1ULL << 4)
#define VMM_PAGE_ACCESSED      (1ULL << 5)
#define VMM_PAGE_DIRTY         (1ULL << 6)
#define VMM_PAGE_GLOBAL        (1ULL << 8)

#define VMM_PAGE_KERNEL_RW \
    (VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE)

#define VMM_PAGE_KERNEL_RO \
    (VMM_PAGE_PRESENT)

#define VMM_PAGE_MMIO \
    (VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE | \
     VMM_PAGE_CACHE_DISABLE | VMM_PAGE_WRITE_THROUGH)

int vmm_init(uint64_t hhdm_offset);

uint64_t vmm_translate(uint64_t virtual_address);

int vmm_map_page(
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t flags
);

int vmm_unmap_page(uint64_t virtual_address);

int vmm_map_range(
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t page_count,
    uint64_t flags
);

int vmm_unmap_range(
    uint64_t virtual_address,
    uint64_t page_count
);

#endif