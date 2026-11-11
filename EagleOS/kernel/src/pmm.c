#include <stdint.h>
#include <stddef.h>
#include <pmm.h>

#define PMM_RESERVED_MEMORY 0x100000ULL

#define PMM_MAX_MEMORY (32ULL * 1024ULL * 1024ULL * 1024ULL)

#define PMM_MAX_PAGES (PMM_MAX_MEMORY / PMM_PAGE_SIZE)

#define PMM_BITMAP_SIZE ((PMM_MAX_PAGES + 7) / 8)

static uint8_t allocation_bitmap[PMM_BITMAP_SIZE];
static uint8_t usable_bitmap[PMM_BITMAP_SIZE];

static uint64_t free_pages = 0;
static uint64_t search_hint = 1;

static int bitmap_test(
    const uint8_t *bitmap,
    uint64_t index
) {
    return (bitmap[index / 8] & (1U << (index % 8))) != 0;
}

static void bitmap_set(
    uint8_t *bitmap,
    uint64_t index
) {
    bitmap[index / 8] |= (uint8_t)(1U << (index % 8));
}

static void bitmap_clear(
    uint8_t *bitmap,
    uint64_t index
) {
    bitmap[index / 8] &= (uint8_t)~(1U << (index % 8));
}

static int page_is_available(uint64_t page) {
    return page < PMM_MAX_PAGES &&
           bitmap_test(usable_bitmap, page) &&
           !bitmap_test(allocation_bitmap, page);
}

static uint64_t find_contiguous_pages(
    uint64_t page_count,
    uint64_t start,
    uint64_t end
) {
    if (page_count == 0 ||
        start >= end ||
        page_count > end - start) {
        return PMM_FAILURE;
    }

    uint64_t candidate = start;
    uint64_t consecutive = 0;

    for (uint64_t page = start; page < end; page++) {
        if (page_is_available(page)) {
            consecutive++;

            if (consecutive == page_count) {
                return candidate;
            }
        } else {
            consecutive = 0;
            candidate = page + 1;
        }
    }

    return PMM_FAILURE;
}

int pmm_init(struct limine_memmap_response *memory_map) {
    if (memory_map == NULL) {
        return 0;
    }

    for (uint64_t i = 0; i < PMM_BITMAP_SIZE; i++) {
        allocation_bitmap[i] = 0xFF;
        usable_bitmap[i] = 0;
    }

    free_pages = 0;
    search_hint = 1;

    for (uint64_t i = 0; i < memory_map->entry_count; i++) {
        struct limine_memmap_entry *entry =
            memory_map->entries[i];

        if (entry == NULL ||
            entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        uint64_t start = entry->base;

        if (start < PMM_RESERVED_MEMORY) {
            start = PMM_RESERVED_MEMORY;
        }

        if (start >= PMM_MAX_MEMORY) {
            continue;
        }

        start = (start + PMM_PAGE_SIZE - 1)
            & ~(PMM_PAGE_SIZE - 1);

        uint64_t end;

        if (entry->length > UINT64_MAX - entry->base) {
            end = PMM_MAX_MEMORY;
        } else {
            end = entry->base + entry->length;
        }

        if (end > PMM_MAX_MEMORY) {
            end = PMM_MAX_MEMORY;
        }

        end &= ~(PMM_PAGE_SIZE - 1);

        if (start >= end) {
            continue;
        }

        for (uint64_t address = start;
             address < end;
             address += PMM_PAGE_SIZE) {

            uint64_t page = address / PMM_PAGE_SIZE;

            if (page >= PMM_MAX_PAGES) {
                break;
            }

            if (!bitmap_test(usable_bitmap, page)) {
                bitmap_set(usable_bitmap, page);
                bitmap_clear(allocation_bitmap, page);
                free_pages++;
            }
        }
    }

    return free_pages > 0;
}

uint64_t pmm_alloc_contiguous_pages(uint64_t page_count) {
    if (page_count == 0 ||
        page_count > PMM_MAX_PAGES ||
        free_pages < page_count) {
        return PMM_FAILURE;
    }

    uint64_t first_page = find_contiguous_pages(
        page_count,
        search_hint,
        PMM_MAX_PAGES
    );

    if (first_page == PMM_FAILURE && search_hint > 1) {
        first_page = find_contiguous_pages(
            page_count,
            1,
            search_hint
        );
    }

    if (first_page == PMM_FAILURE) {
        return PMM_FAILURE;
    }

    for (uint64_t i = 0; i < page_count; i++) {
        bitmap_set(allocation_bitmap, first_page + i);
    }

    free_pages -= page_count;

    search_hint = first_page + page_count;

    if (search_hint >= PMM_MAX_PAGES) {
        search_hint = 1;
    }

    return first_page * PMM_PAGE_SIZE;
}

uint64_t pmm_alloc_page(void) {
    return pmm_alloc_contiguous_pages(1);
}

int pmm_free_page(uint64_t physical_address) {
    if (physical_address == 0 ||
        physical_address % PMM_PAGE_SIZE != 0) {
        return 0;
    }

    if (physical_address < PMM_RESERVED_MEMORY) {
        return 0;
    }

    uint64_t page = physical_address / PMM_PAGE_SIZE;

    if (page >= PMM_MAX_PAGES) {
        return 0;
    }

    if (!bitmap_test(usable_bitmap, page)) {
        return 0;
    }

    if (!bitmap_test(allocation_bitmap, page)) {
        return 0;
    }

    bitmap_clear(allocation_bitmap, page);

    free_pages++;

    if (page < search_hint) {
        search_hint = page;
    }

    return 1;
}

uint64_t pmm_get_free_pages(void) {
    return free_pages;
}