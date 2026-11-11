#include <stdint.h>
#include <vmm.h>
#include <pmm.h>

#define PAGE_TABLE_ENTRIES 512

#define PAGE_ENTRY_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define PAGE_2MB_ADDRESS_MASK 0x000FFFFFFFE00000ULL
#define PAGE_1GB_ADDRESS_MASK 0x000FFFFFC0000000ULL

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER (1ULL << 2)
#define PAGE_HUGE (1ULL << 7)

#define PAGE_4KB_OFFSET_MASK 0xFFFULL
#define PAGE_2MB_OFFSET_MASK 0x1FFFFFULL
#define PAGE_1GB_OFFSET_MASK 0x3FFFFFFFULL

#define CR4_LA57 (1ULL << 12)

#define VMM_ALLOWED_FLAGS ( \
    VMM_PAGE_PRESENT | \
    VMM_PAGE_WRITABLE | \
    VMM_PAGE_USER | \
    VMM_PAGE_WRITE_THROUGH | \
    VMM_PAGE_CACHE_DISABLE | \
    VMM_PAGE_ACCESSED | \
    VMM_PAGE_DIRTY | \
    VMM_PAGE_GLOBAL \
)

static uint64_t hhdm_offset;
static int initialized;

static uint64_t read_cr3(void) {
    uint64_t value;

    __asm__ volatile (
        "mov %%cr3, %0"
        : "=r"(value)
    );

    return value;
}

static uint64_t read_cr4(void) {
    uint64_t value;

    __asm__ volatile (
        "mov %%cr4, %0"
        : "=r"(value)
    );

    return value;
}

static void invalidate_page(uint64_t virtual_address) {
    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );
}

static int is_canonical_address(uint64_t address) {
    uint64_t upper_bits = address >> 48;
    uint64_t sign_bit = (address >> 47) & 1;

    if (sign_bit) {
        return upper_bits == 0xFFFF;
    }

    return upper_bits == 0;
}

static int is_page_aligned(uint64_t address) {
    return (address & (VMM_PAGE_SIZE - 1)) == 0;
}

static volatile uint64_t *physical_to_virtual(
    uint64_t physical_address
) {
    if (physical_address > UINT64_MAX - hhdm_offset) {
        return 0;
    }

    uint64_t virtual_address =
        hhdm_offset + physical_address;

    if (!is_canonical_address(virtual_address)) {
        return 0;
    }

    return (volatile uint64_t *)virtual_address;
}

static void zero_page(uint64_t physical_address) {
    volatile uint64_t *page =
        physical_to_virtual(physical_address);

    if (page == 0) {
        return;
    }

    for (uint64_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        page[i] = 0;
    }
}

static volatile uint64_t *get_page_table(
    uint64_t physical_address
) {
    return physical_to_virtual(
        physical_address & PAGE_ENTRY_ADDRESS_MASK
    );
}

static int allocate_page_table(uint64_t *physical_address) {
    uint64_t page = pmm_alloc_page();

    if (page == PMM_FAILURE) {
        return 0;
    }

    volatile uint64_t *virtual_page =
        physical_to_virtual(page);

    if (virtual_page == 0) {
        pmm_free_page(page);
        return 0;
    }

    for (uint64_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        virtual_page[i] = 0;
    }

    *physical_address = page;

    return 1;
}

static int get_or_create_table(
    volatile uint64_t *parent,
    uint64_t index,
    int user,
    volatile uint64_t **child
) {
    uint64_t entry = parent[index];

    if (!(entry & PAGE_PRESENT)) {
        uint64_t new_table;

        if (!allocate_page_table(&new_table)) {
            return 0;
        }

        uint64_t new_entry =
            new_table |
            PAGE_PRESENT |
            PAGE_WRITABLE;

        if (user) {
            new_entry |= PAGE_USER;
        }

        parent[index] = new_entry;

        entry = new_entry;
    } else {
        if (entry & PAGE_HUGE) {
            return 0;
        }

        if (!(entry & PAGE_WRITABLE)) {
            entry |= PAGE_WRITABLE;
        }

        if (user && !(entry & PAGE_USER)) {
            entry |= PAGE_USER;
        }

        parent[index] = entry;
    }

    *child = get_page_table(entry);

    return *child != 0;
}

int vmm_init(uint64_t offset) {
    uint64_t cr4 = read_cr4();

    if (cr4 & CR4_LA57) {
        initialized = 0;
        return 0;
    }

    if (!is_canonical_address(offset)) {
        initialized = 0;
        return 0;
    }

    hhdm_offset = offset;
    initialized = 1;

    return 1;
}

uint64_t vmm_translate(uint64_t virtual_address) {
    if (!initialized) {
        return VMM_FAILURE;
    }

    if (!is_canonical_address(virtual_address)) {
        return VMM_FAILURE;
    }

    uint64_t cr3 = read_cr3();

    uint64_t pml4_physical =
        cr3 & PAGE_ENTRY_ADDRESS_MASK;

    volatile uint64_t *pml4 =
        get_page_table(pml4_physical);

    if (pml4 == 0) {
        return VMM_FAILURE;
    }

    uint64_t pml4_index =
        (virtual_address >> 39) & 0x1FF;

    uint64_t pdpt_index =
        (virtual_address >> 30) & 0x1FF;

    uint64_t pd_index =
        (virtual_address >> 21) & 0x1FF;

    uint64_t pt_index =
        (virtual_address >> 12) & 0x1FF;

    uint64_t pml4_entry = pml4[pml4_index];

    if (!(pml4_entry & PAGE_PRESENT)) {
        return VMM_FAILURE;
    }

    volatile uint64_t *pdpt =
        get_page_table(pml4_entry);

    if (pdpt == 0) {
        return VMM_FAILURE;
    }

    uint64_t pdpt_entry = pdpt[pdpt_index];

    if (!(pdpt_entry & PAGE_PRESENT)) {
        return VMM_FAILURE;
    }

    if (pdpt_entry & PAGE_HUGE) {
        uint64_t physical_base =
            pdpt_entry & PAGE_1GB_ADDRESS_MASK;

        uint64_t offset =
            virtual_address & PAGE_1GB_OFFSET_MASK;

        return physical_base + offset;
    }

    volatile uint64_t *pd =
        get_page_table(pdpt_entry);

    if (pd == 0) {
        return VMM_FAILURE;
    }

    uint64_t pd_entry = pd[pd_index];

    if (!(pd_entry & PAGE_PRESENT)) {
        return VMM_FAILURE;
    }

    if (pd_entry & PAGE_HUGE) {
        uint64_t physical_base =
            pd_entry & PAGE_2MB_ADDRESS_MASK;

        uint64_t offset =
            virtual_address & PAGE_2MB_OFFSET_MASK;

        return physical_base + offset;
    }

    volatile uint64_t *pt =
        get_page_table(pd_entry);

    if (pt == 0) {
        return VMM_FAILURE;
    }

    uint64_t pt_entry = pt[pt_index];

    if (!(pt_entry & PAGE_PRESENT)) {
        return VMM_FAILURE;
    }

    uint64_t physical_base =
        pt_entry & PAGE_ENTRY_ADDRESS_MASK;

    uint64_t offset =
        virtual_address & PAGE_4KB_OFFSET_MASK;

    return physical_base + offset;
}

int vmm_map_page(
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t flags
) {
    if (!initialized) {
        return 0;
    }

    if (!is_page_aligned(virtual_address) ||
        !is_page_aligned(physical_address)) {
        return 0;
    }

    if (!is_canonical_address(virtual_address)) {
        return 0;
    }

    if (physical_address & ~PAGE_ENTRY_ADDRESS_MASK) {
        return 0;
    }

    if (flags & ~VMM_ALLOWED_FLAGS) {
        return 0;
    }

    flags |= PAGE_PRESENT;

    int user = (flags & PAGE_USER) != 0;

    uint64_t cr3 = read_cr3();

    uint64_t pml4_physical =
        cr3 & PAGE_ENTRY_ADDRESS_MASK;

    volatile uint64_t *pml4 =
        get_page_table(pml4_physical);

    if (pml4 == 0) {
        return 0;
    }

    uint64_t pml4_index =
        (virtual_address >> 39) & 0x1FF;

    uint64_t pdpt_index =
        (virtual_address >> 30) & 0x1FF;

    uint64_t pd_index =
        (virtual_address >> 21) & 0x1FF;

    uint64_t pt_index =
        (virtual_address >> 12) & 0x1FF;

    volatile uint64_t *pdpt;

    if (!get_or_create_table(
        pml4,
        pml4_index,
        user,
        &pdpt
    )) {
        return 0;
    }

    volatile uint64_t *pd;

    if (!get_or_create_table(
        pdpt,
        pdpt_index,
        user,
        &pd
    )) {
        return 0;
    }

    volatile uint64_t *pt;

    if (!get_or_create_table(
        pd,
        pd_index,
        user,
        &pt
    )) {
        return 0;
    }

    if (pt[pt_index] & PAGE_PRESENT) {
        return 0;
    }

    pt[pt_index] =
        physical_address | flags;

    invalidate_page(virtual_address);

    return 1;
}

int vmm_unmap_page(uint64_t virtual_address) {
    if (!initialized) {
        return 0;
    }

    if (!is_page_aligned(virtual_address)) {
        return 0;
    }

    if (!is_canonical_address(virtual_address)) {
        return 0;
    }

    uint64_t cr3 = read_cr3();

    uint64_t pml4_physical =
        cr3 & PAGE_ENTRY_ADDRESS_MASK;

    volatile uint64_t *pml4 =
        get_page_table(pml4_physical);

    if (pml4 == 0) {
        return 0;
    }

    uint64_t pml4_index =
        (virtual_address >> 39) & 0x1FF;

    uint64_t pdpt_index =
        (virtual_address >> 30) & 0x1FF;

    uint64_t pd_index =
        (virtual_address >> 21) & 0x1FF;

    uint64_t pt_index =
        (virtual_address >> 12) & 0x1FF;

    uint64_t pml4_entry = pml4[pml4_index];

    if (!(pml4_entry & PAGE_PRESENT)) {
        return 0;
    }

    volatile uint64_t *pdpt =
        get_page_table(pml4_entry);

    if (pdpt == 0) {
        return 0;
    }

    uint64_t pdpt_entry = pdpt[pdpt_index];

    if (!(pdpt_entry & PAGE_PRESENT) ||
        (pdpt_entry & PAGE_HUGE)) {
        return 0;
    }

    volatile uint64_t *pd =
        get_page_table(pdpt_entry);

    if (pd == 0) {
        return 0;
    }

    uint64_t pd_entry = pd[pd_index];

    if (!(pd_entry & PAGE_PRESENT) ||
        (pd_entry & PAGE_HUGE)) {
        return 0;
    }

    volatile uint64_t *pt =
        get_page_table(pd_entry);

    if (pt == 0) {
        return 0;
    }

    uint64_t pt_entry = pt[pt_index];

    if (!(pt_entry & PAGE_PRESENT)) {
        return 0;
    }

    pt[pt_index] = 0;

    invalidate_page(virtual_address);

    return 1;
}

int vmm_map_range(
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t page_count,
    uint64_t flags
) {
    if (page_count == 0) {
        return 0;
    }

    if (!is_page_aligned(virtual_address) ||
        !is_page_aligned(physical_address)) {
        return 0;
    }

    if (page_count > UINT64_MAX / VMM_PAGE_SIZE) {
        return 0;
    }

    uint64_t total_size =
        page_count * VMM_PAGE_SIZE;

    if (virtual_address > UINT64_MAX - total_size ||
        physical_address > UINT64_MAX - total_size) {
        return 0;
    }

    if (!is_canonical_address(virtual_address) ||
        !is_canonical_address(
            virtual_address + total_size - 1
        )) {
        return 0;
    }

    uint64_t mapped = 0;

    for (; mapped < page_count; mapped++) {
        uint64_t current_virtual =
            virtual_address + mapped * VMM_PAGE_SIZE;

        uint64_t current_physical =
            physical_address + mapped * VMM_PAGE_SIZE;

        if (!vmm_map_page(
            current_virtual,
            current_physical,
            flags
        )) {
            break;
        }
    }

    if (mapped == page_count) {
        return 1;
    }

    while (mapped > 0) {
        mapped--;

        uint64_t current_virtual =
            virtual_address + mapped * VMM_PAGE_SIZE;

        vmm_unmap_page(current_virtual);
    }

    return 0;
}

int vmm_unmap_range(
    uint64_t virtual_address,
    uint64_t page_count
) {
    if (!initialized || page_count == 0) {
        return 0;
    }

    if (!is_page_aligned(virtual_address)) {
        return 0;
    }

    if (page_count > UINT64_MAX / VMM_PAGE_SIZE) {
        return 0;
    }

    uint64_t total_size =
        page_count * VMM_PAGE_SIZE;

    if (virtual_address > UINT64_MAX - total_size) {
        return 0;
    }

    if (!is_canonical_address(virtual_address) ||
        !is_canonical_address(
            virtual_address + total_size - 1
        )) {
        return 0;
    }

    for (uint64_t i = 0; i < page_count; i++) {
        uint64_t address =
            virtual_address + i * VMM_PAGE_SIZE;

        if (vmm_translate(address) == VMM_FAILURE) {
            return 0;
        }
    }

    for (uint64_t i = 0; i < page_count; i++) {
        uint64_t address =
            virtual_address + i * VMM_PAGE_SIZE;

        if (!vmm_unmap_page(address)) {
            return 0;
        }
    }

    return 1;
}