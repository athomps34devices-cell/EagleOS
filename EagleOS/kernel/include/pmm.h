#ifndef EAGLEOS_PMM_H
#define EAGLEOS_PMM_H

#include <stdint.h>
#include <limine.h>

#define PMM_FAILURE UINT64_MAX

#define PMM_PAGE_SIZE 4096ULL

int pmm_init(struct limine_memmap_response *memory_map);

uint64_t pmm_alloc_page(void);

uint64_t pmm_alloc_contiguous_pages(uint64_t page_count);

int pmm_free_page(uint64_t physical_address);

uint64_t pmm_get_free_pages(void);

#endif