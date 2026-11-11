#include <stdint.h>
#include <stddef.h>
#include <limine.h>
#include <pmm.h>
#include <vmm.h>
#include <idt.h>
#include <pic.h>
#include <irq.h>
#include <pit.h>
#include <clock.h>
#include "rtc.h"
#include <keyboard.h>
#include <pci.h>
#include <ahci.h>
#include <blockdev.h>
#include <iso9660.h>
#include <filesystem.h>

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile uint64_t base_revision[] =
    LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memory_map_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

static const uint8_t font[26][7] = {
    {14,17,17,31,17,17,17},
    {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31},
    {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},
    {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17},
    {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},
    {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13},
    {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},
    {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10},
    {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},
    {31,1,2,4,8,16,31}
};

static struct limine_framebuffer *framebuffer;

#define SERIAL_PORT 0x3F8
#define PMM_PAGE_SIZE 4096ULL
#define PAGE_TABLE_ENTRIES 512
#define MAX_PT_ENTRIES_TO_PRINT 20

#define PAGE_ENTRY_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define PAGE_2MB_ADDRESS_MASK 0x000FFFFFFFE00000ULL
#define PAGE_1GB_ADDRESS_MASK 0x000FFFFFC0000000ULL

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER (1ULL << 2)
#define PAGE_HUGE (1ULL << 7)
#define PAGE_NO_EXECUTE (1ULL << 63)

#define VMM_TEST_VIRTUAL_BASE 0xFFFF900000000000ULL
#define VMM_TEST_RANGE_PAGES 3ULL

static uint64_t hhdm_offset;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;

    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

static void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x03);
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);
}

static void serial_write_char(char character) {
    uint32_t timeout = 1000000;

    while (!(inb(SERIAL_PORT + 5) & 0x20)) {
        if (--timeout == 0) return;
    }

    outb(SERIAL_PORT, (uint8_t)character);
}

static void serial_write(const char *text) {
    while (*text) {
        if (*text == '\n') {
            serial_write_char('\r');
        }

        serial_write_char(*text++);
    }
}

static void serial_write_u64(uint64_t value) {
    char buffer[21];
    uint32_t index = 0;

    if (value == 0) {
        serial_write_char('0');
        return;
    }

    while (value > 0) {
        buffer[index++] = '0' + (value % 10);
        value /= 10;
    }

    while (index > 0) {
        serial_write_char(buffer[--index]);
    }
}

static void serial_write_hex(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";

    serial_write("0x");

    int started = 0;

    for (int shift = 60; shift >= 0; shift -= 4) {
        uint8_t digit = (value >> shift) & 0xF;

        if (digit || started || shift == 0) {
            serial_write_char(digits[digit]);
            started = 1;
        }
    }
}

static void log_message(const char *level, const char *message) {
    serial_write("[EagleOS] [");
    serial_write(level);
    serial_write("] ");
    serial_write(message);
    serial_write("\n");
}

static void log_info(const char *message) {
    log_message("INFO", message);
}

static void log_success(const char *message) {
    log_message("SUCCESS", message);
}

static void log_warning(const char *message) {
    log_message("WARNING", message);
}

static void log_error(const char *message) {
    log_message("ERROR", message);
}

static void log_debug(const char *message) {
    log_message("DEBUG", message);
}

__attribute__((noreturn))
static void halt_system(void) {
    for (;;) {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}

static uint32_t rgb(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t r = (red >> (8 - framebuffer->red_mask_size))
        << framebuffer->red_mask_shift;

    uint32_t g = (green >> (8 - framebuffer->green_mask_size))
        << framebuffer->green_mask_shift;

    uint32_t b = (blue >> (8 - framebuffer->blue_mask_size))
        << framebuffer->blue_mask_shift;

    return r | g | b;
}

static void put_pixel(uint64_t x, uint64_t y, uint32_t color) {
    if (x >= framebuffer->width || y >= framebuffer->height) return;

    uint32_t *row = (uint32_t *)(
        (uint8_t *)framebuffer->address + y * framebuffer->pitch
    );

    row[x] = color;
}

static void fill_screen(uint32_t color) {
    for (uint64_t y = 0; y < framebuffer->height; y++) {
        for (uint64_t x = 0; x < framebuffer->width; x++) {
            put_pixel(x, y, color);
        }
    }
}

static void fill_rect(
    uint64_t x,
    uint64_t y,
    uint64_t width,
    uint64_t height,
    uint32_t color
) {
    for (uint64_t row = y; row < y + height; row++) {
        for (uint64_t column = x; column < x + width; column++) {
            put_pixel(column, row, color);
        }
    }
}

static void draw_char(
    char character,
    uint64_t x,
    uint64_t y,
    uint32_t scale,
    uint32_t color
) {
    if (character < 'A' || character > 'Z') return;

    const uint8_t *glyph = font[character - 'A'];

    for (uint32_t row = 0; row < 7; row++) {
        for (uint32_t column = 0; column < 5; column++) {
            if (glyph[row] & (1 << (4 - column))) {
                fill_rect(
                    x + column * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

static void draw_text(
    const char *text,
    uint64_t x,
    uint64_t y,
    uint32_t scale,
    uint32_t color
) {
    while (*text) {
        draw_char(*text++, x, y, scale, color);
        x += 6 * scale;
    }
}

static void draw_boot_screen(void) {
    uint32_t background = rgb(8, 13, 24);
    uint32_t primary = rgb(139, 92, 246);
    uint32_t white = rgb(245, 245, 250);
    uint32_t muted = rgb(150, 160, 180);

    fill_screen(background);

    uint64_t center_x = framebuffer->width / 2;
    uint64_t center_y = framebuffer->height / 2;

    fill_rect(center_x - 35, center_y - 105, 70, 8, primary);
    fill_rect(center_x - 35, center_y - 97, 8, 55, primary);
    fill_rect(center_x + 27, center_y - 97, 8, 55, primary);
    fill_rect(center_x - 35, center_y - 42, 70, 8, primary);

    draw_text("EAGLEOS", center_x - 105, center_y - 10, 5, white);
    draw_text("KERNEL STARTED", center_x - 84, center_y + 45, 2, muted);

    fill_rect(center_x - 100, center_y + 85, 200, 3, primary);
}

static const char *memory_type_name(uint64_t type) {
    switch (type) {
        case LIMINE_MEMMAP_USABLE: return "USABLE";
        case LIMINE_MEMMAP_RESERVED: return "RESERVED";
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE: return "ACPI_RECLAIMABLE";
        case LIMINE_MEMMAP_ACPI_NVS: return "ACPI_NVS";
        case LIMINE_MEMMAP_BAD_MEMORY: return "BAD_MEMORY";
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: return "BOOTLOADER_RECLAIMABLE";
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: return "KERNEL_AND_MODULES";
        case LIMINE_MEMMAP_FRAMEBUFFER: return "FRAMEBUFFER";
        default: return "UNKNOWN";
    }
}

static void detect_memory(void) {
    log_info("Requesting physical memory map...");

    if (memory_map_request.response == NULL) {
        log_warning("Memory map unavailable.");
        return;
    }

    struct limine_memmap_response *response = memory_map_request.response;

    uint64_t usable_memory = 0;
    uint64_t total_memory = 0;

    log_success("Memory map received.");

    serial_write("[EagleOS] Memory map entries: ");
    serial_write_u64(response->entry_count);
    serial_write("\n");

    for (uint64_t i = 0; i < response->entry_count; i++) {
        struct limine_memmap_entry *entry = response->entries[i];

        if (entry == NULL) continue;

        total_memory += entry->length;

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            usable_memory += entry->length;
        }

        serial_write("[EagleOS]   ");
        serial_write(memory_type_name(entry->type));
        serial_write(" | Base: ");
        serial_write_u64(entry->base);
        serial_write(" | Size: ");
        serial_write_u64(entry->length / 1024);
        serial_write(" KB\n");
    }

    log_info("Physical memory detection complete.");

    serial_write("[EagleOS] Total mapped memory: ");
    serial_write_u64(total_memory / (1024 * 1024));
    serial_write(" MB\n");

    serial_write("[EagleOS] Usable memory: ");
    serial_write_u64(usable_memory / (1024 * 1024));
    serial_write(" MB\n");

    log_success("Memory map processed.");
}

static uint64_t make_virtual_address(
    uint64_t pml4_index,
    uint64_t pdpt_index,
    uint64_t pd_index,
    uint64_t pt_index
) {
    uint64_t address =
        (pml4_index << 39) |
        (pdpt_index << 30) |
        (pd_index << 21) |
        (pt_index << 12);

    if (address & (1ULL << 47)) {
        address |= 0xFFFF000000000000ULL;
    }

    return address;
}

static void print_page_flags(uint64_t entry) {
    serial_write(" | Writable: ");
    serial_write((entry & PAGE_WRITABLE) ? "YES" : "NO");

    serial_write(" | User: ");
    serial_write((entry & PAGE_USER) ? "YES" : "NO");

    serial_write(" | NX: ");
    serial_write((entry & PAGE_NO_EXECUTE) ? "YES" : "NO");
}

static void inspect_page_table(
    uint64_t *total_pt_entries,
    uint64_t *total_pt_tables,
    uint64_t *printed_pt_entries,
    uint64_t *omitted_pt_entries,
    uint64_t pml4_index,
    uint64_t pdpt_index,
    uint64_t pd_index,
    uint64_t pt_physical
) {
    volatile uint64_t *pt =
        (volatile uint64_t *)(hhdm_offset + pt_physical);

    (*total_pt_tables)++;

    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        uint64_t entry = pt[i];

        if (!(entry & PAGE_PRESENT)) {
            continue;
        }

        (*total_pt_entries)++;

        if (*printed_pt_entries >= MAX_PT_ENTRIES_TO_PRINT) {
            (*omitted_pt_entries)++;
            continue;
        }

        uint64_t physical =
            entry & PAGE_ENTRY_ADDRESS_MASK;

        uint64_t virtual_address = make_virtual_address(
            pml4_index,
            pdpt_index,
            pd_index,
            i
        );

        serial_write("[EagleOS]       PT[");
        serial_write_u64(i);
        serial_write("]");

        serial_write(" | Entry: ");
        serial_write_hex(entry);

        serial_write(" | Virtual: ");
        serial_write_hex(virtual_address);

        serial_write(" | Physical: ");
        serial_write_hex(physical);

        print_page_flags(entry);

        serial_write("\n");

        (*printed_pt_entries)++;
    }
}

static void inspect_pml4(void) {
    log_info("Inspecting all four levels of the active page tables...");

    uint64_t cr3;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    uint64_t pml4_physical = cr3 & PAGE_ENTRY_ADDRESS_MASK;

    volatile uint64_t *pml4 =
        (volatile uint64_t *)(hhdm_offset + pml4_physical);

    uint64_t pml4_present_entries = 0;
    uint64_t total_pdpt_entries = 0;
    uint64_t total_pd_entries = 0;
    uint64_t total_pt_entries = 0;
    uint64_t total_pt_tables = 0;
    uint64_t total_1gb_pages = 0;
    uint64_t total_2mb_pages = 0;
    uint64_t printed_pt_entries = 0;
    uint64_t omitted_pt_entries = 0;

    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        uint64_t pml4_entry = pml4[i];

        if (!(pml4_entry & PAGE_PRESENT)) {
            continue;
        }

        pml4_present_entries++;

        uint64_t pdpt_physical =
            pml4_entry & PAGE_ENTRY_ADDRESS_MASK;

        volatile uint64_t *pdpt =
            (volatile uint64_t *)(hhdm_offset + pdpt_physical);

        serial_write("\n[EagleOS] PML4[");
        serial_write_u64(i);
        serial_write("] = ");
        serial_write_hex(pml4_entry);
        serial_write("\n");

        uint64_t pdpt_present_entries = 0;

        for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            uint64_t pdpt_entry = pdpt[j];

            if (!(pdpt_entry & PAGE_PRESENT)) {
                continue;
            }

            pdpt_present_entries++;
            total_pdpt_entries++;

            if (pdpt_entry & PAGE_HUGE) {
                total_1gb_pages++;

                serial_write("[EagleOS]   PDPT[");
                serial_write_u64(j);
                serial_write("] | 1 GiB PAGE | Physical: ");

                serial_write_hex(
                    pdpt_entry & PAGE_1GB_ADDRESS_MASK
                );

                serial_write("\n");

                continue;
            }

            uint64_t pd_physical =
                pdpt_entry & PAGE_ENTRY_ADDRESS_MASK;

            volatile uint64_t *pd =
                (volatile uint64_t *)(hhdm_offset + pd_physical);

            uint64_t pd_present_entries = 0;
            uint64_t pd_huge_pages = 0;
            uint64_t pd_page_tables = 0;

            for (uint32_t k = 0; k < PAGE_TABLE_ENTRIES; k++) {
                uint64_t pd_entry = pd[k];

                if (!(pd_entry & PAGE_PRESENT)) {
                    continue;
                }

                pd_present_entries++;
                total_pd_entries++;

                if (pd_entry & PAGE_HUGE) {
                    pd_huge_pages++;
                    total_2mb_pages++;
                    continue;
                }

                uint64_t pt_physical =
                    pd_entry & PAGE_ENTRY_ADDRESS_MASK;

                pd_page_tables++;

                inspect_page_table(
                    &total_pt_entries,
                    &total_pt_tables,
                    &printed_pt_entries,
                    &omitted_pt_entries,
                    i,
                    j,
                    k,
                    pt_physical
                );
            }

            serial_write("[EagleOS]     PDPT[");
            serial_write_u64(j);
            serial_write("] | Present PD entries: ");
            serial_write_u64(pd_present_entries);

            serial_write(" | 2 MiB pages: ");
            serial_write_u64(pd_huge_pages);

            serial_write(" | PTs: ");
            serial_write_u64(pd_page_tables);

            serial_write("\n");
        }

        serial_write("[EagleOS]   Present PDPT entries: ");
        serial_write_u64(pdpt_present_entries);
        serial_write(" / 512\n");
    }

    serial_write("\n");
    serial_write("[EagleOS] ==================================\n");
    serial_write("[EagleOS] FOUR-LEVEL PAGE TABLE SUMMARY\n");
    serial_write("[EagleOS] ==================================\n");

    serial_write("[EagleOS] Present PML4 entries: ");
    serial_write_u64(pml4_present_entries);
    serial_write(" / 512\n");

    serial_write("[EagleOS] Present PDPT entries: ");
    serial_write_u64(total_pdpt_entries);
    serial_write("\n");

    serial_write("[EagleOS] Present PD entries: ");
    serial_write_u64(total_pd_entries);
    serial_write("\n");

    serial_write("[EagleOS] Page Tables inspected: ");
    serial_write_u64(total_pt_tables);
    serial_write("\n");

    serial_write("[EagleOS] Present PT entries (4 KiB mappings): ");
    serial_write_u64(total_pt_entries);
    serial_write("\n");

    serial_write("[EagleOS] Printed PT entries: ");
    serial_write_u64(printed_pt_entries);
    serial_write("\n");

    serial_write("[EagleOS] Omitted PT entries: ");
    serial_write_u64(omitted_pt_entries);
    serial_write("\n");

    serial_write("[EagleOS] Total 1 GiB pages: ");
    serial_write_u64(total_1gb_pages);
    serial_write("\n");

    serial_write("[EagleOS] Total 2 MiB pages: ");
    serial_write_u64(total_2mb_pages);
    serial_write("\n");

    if (omitted_pt_entries > 0) {
        log_warning("Additional 4 KiB mappings omitted from terminal output.");
    }

    if (pml4_present_entries == 0) {
        log_error("No present PML4 entries detected.");
        halt_system();
    }

    log_success("Complete four-level page table inspection finished.");
}

static void inspect_virtual_memory(void) {
    log_info("Inspecting current virtual memory configuration...");

    uint64_t cr3;
    uint64_t cr4;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));

    uint64_t pml4_physical = cr3 & PAGE_ENTRY_ADDRESS_MASK;

    serial_write("[EagleOS] CR3 register: ");
    serial_write_hex(cr3);
    serial_write("\n");

    serial_write("[EagleOS] Active PML4 physical address: ");
    serial_write_hex(pml4_physical);
    serial_write("\n");

    serial_write("[EagleOS] CR4 register: ");
    serial_write_hex(cr4);
    serial_write("\n");

    if (cr4 & (1ULL << 12)) {
        log_success("Five-level paging is enabled.");
    } else {
        log_success("Four-level paging is enabled.");
    }

    uint32_t eax;
    uint32_t edx;

    __asm__ volatile (
        "mov $0xC0000080, %%ecx\n"
        "rdmsr"
        : "=a"(eax), "=d"(edx)
        :
        : "ecx"
    );

    uint64_t efer = ((uint64_t)edx << 32) | eax;

    serial_write("[EagleOS] EFER register: ");
    serial_write_hex(efer);
    serial_write("\n");

    if (hhdm_request.response == NULL) {
        log_error("Limine HHDM response unavailable.");
        halt_system();
    }

    hhdm_offset = hhdm_request.response->offset;

    serial_write("[EagleOS] HHDM virtual offset: ");
    serial_write_hex(hhdm_offset);
    serial_write("\n");

    serial_write("[EagleOS] PML4 virtual address through HHDM: ");
    serial_write_hex(hhdm_offset + pml4_physical);
    serial_write("\n");

    log_success("Virtual memory configuration inspected.");

    inspect_pml4();
}

static void test_pmm(void) {
    log_info("Testing physical page allocation...");

    uint64_t initial_free_pages = pmm_get_free_pages();
    uint64_t test_pages[3];

    for (uint32_t i = 0; i < 3; i++) {
        test_pages[i] = pmm_alloc_page();

        if (test_pages[i] == PMM_FAILURE) {
            log_error("Physical page allocation failed.");
            halt_system();
        }

        serial_write("[EagleOS] Allocated page ");
        serial_write_u64(i + 1);
        serial_write(": ");
        serial_write_u64(test_pages[i]);
        serial_write("\n");
    }

    log_success("Three physical pages allocated.");

    if (test_pages[0] == test_pages[1] ||
        test_pages[0] == test_pages[2] ||
        test_pages[1] == test_pages[2]) {
        log_error("Allocator returned duplicate physical pages.");
        halt_system();
    }

    log_success("All allocated pages have unique addresses.");

    for (uint32_t i = 0; i < 3; i++) {
        if (test_pages[i] % PMM_PAGE_SIZE != 0) {
            log_error("Allocated page is not properly aligned.");
            halt_system();
        }
    }

    log_success("All allocated pages are 4 KiB aligned.");

    for (uint32_t i = 0; i < 3; i++) {
        if (!pmm_free_page(test_pages[i])) {
            log_error("Physical page release failed.");
            halt_system();
        }
    }

    log_success("All three physical pages released.");

    if (pmm_free_page(test_pages[0])) {
        log_error("PMM accepted an invalid double-free!");
        halt_system();
    }

    log_success("Double-free protection passed.");

    uint64_t final_free_pages = pmm_get_free_pages();

    serial_write("[EagleOS] Initial free pages: ");
    serial_write_u64(initial_free_pages);
    serial_write("\n");

    serial_write("[EagleOS] Final free pages: ");
    serial_write_u64(final_free_pages);
    serial_write("\n");

    if (initial_free_pages != final_free_pages) {
        log_error("Physical memory accounting mismatch.");
        halt_system();
    }

    log_success("All PMM tests passed.");
}

static void test_vmm(void) {
    log_info("Testing virtual address translation...");

    uint64_t physical_address = 0x1000;

    uint64_t virtual_address =
        hhdm_offset + physical_address;

    uint64_t translated =
        vmm_translate(virtual_address);

    serial_write("[EagleOS] Test virtual address: ");
    serial_write_hex(virtual_address);
    serial_write("\n");

    serial_write("[EagleOS] Translated physical address: ");
    serial_write_hex(translated);
    serial_write("\n");

    if (translated != physical_address) {
        log_error("4 KiB virtual address translation failed.");
        halt_system();
    }

    log_success("4 KiB virtual address translation passed.");

    physical_address = 0x40000000ULL;

    virtual_address =
        hhdm_offset + physical_address;

    translated = vmm_translate(virtual_address);

    serial_write("[EagleOS] 2 MiB mapping test: ");
    serial_write_hex(translated);
    serial_write("\n");

    if (translated != physical_address) {
        log_error("2 MiB virtual address translation failed.");
        halt_system();
    }

    log_success("2 MiB virtual address translation passed.");

    translated = vmm_translate(0x0000400000000000ULL);

    if (translated != VMM_FAILURE) {
        log_warning("Unmapped-address test address was mapped.");
    } else {
        log_success("Unmapped-address detection passed.");
    }

    log_success("All existing VMM translation tests passed.");
}

static void test_vmm_mapping(void) {
    log_info("Testing dynamic virtual memory mapping...");

    uint64_t initial_free_pages = pmm_get_free_pages();

    serial_write("[EagleOS] VMM test virtual base: ");
    serial_write_hex(VMM_TEST_VIRTUAL_BASE);
    serial_write("\n");

    log_info("Testing single-page mapping...");

    uint64_t physical_page = pmm_alloc_page();

    if (physical_page == PMM_FAILURE) {
        log_error("Could not allocate physical page for VMM test.");
        halt_system();
    }

    serial_write("[EagleOS] Allocated physical page: ");
    serial_write_hex(physical_page);
    serial_write("\n");

    if (!vmm_map_page(
        VMM_TEST_VIRTUAL_BASE,
        physical_page,
        VMM_PAGE_KERNEL_RW
    )) {
        log_error("Single-page virtual mapping failed.");
        pmm_free_page(physical_page);
        halt_system();
    }

    log_success("Single physical page mapped.");

    volatile uint64_t *mapped_memory =
        (volatile uint64_t *)VMM_TEST_VIRTUAL_BASE;

    const uint64_t test_pattern = 0x4541474C454F5331ULL;

    mapped_memory[0] = test_pattern;
    mapped_memory[1] = 0x123456789ABCDEF0ULL;

    if (mapped_memory[0] != test_pattern ||
        mapped_memory[1] != 0x123456789ABCDEF0ULL) {
        log_error("Mapped memory read/write verification failed.");
        halt_system();
    }

    log_success("Mapped memory read/write verification passed.");

    uint64_t translated =
        vmm_translate(VMM_TEST_VIRTUAL_BASE);

    if (translated != physical_page) {
        log_error("Mapped virtual address translated incorrectly.");
        halt_system();
    }

    log_success("Mapped virtual-to-physical translation passed.");

    if (!vmm_unmap_page(VMM_TEST_VIRTUAL_BASE)) {
        log_error("Single-page unmapping failed.");
        halt_system();
    }

    log_success("Single virtual page unmapped.");

    if (vmm_translate(VMM_TEST_VIRTUAL_BASE) != VMM_FAILURE) {
        log_error("Unmapped virtual address still has a translation.");
        halt_system();
    }

    log_success("Single-page unmapped-address detection passed.");

    if (!pmm_free_page(physical_page)) {
        log_error("Could not release single-page test allocation.");
        halt_system();
    }

    log_success("Single-page physical allocation released.");

    log_info("Testing multiple-page range mapping...");

    uint64_t range_physical_base =
        pmm_alloc_contiguous_pages(VMM_TEST_RANGE_PAGES);

    if (range_physical_base == PMM_FAILURE) {
        log_error("Could not allocate contiguous physical pages for range test.");
        halt_system();
    }

    serial_write("[EagleOS] Range physical base: ");
    serial_write_hex(range_physical_base);
    serial_write("\n");

    serial_write("[EagleOS] Range page count: ");
    serial_write_u64(VMM_TEST_RANGE_PAGES);
    serial_write("\n");

    if (!vmm_map_range(
        VMM_TEST_VIRTUAL_BASE,
        range_physical_base,
        VMM_TEST_RANGE_PAGES,
        VMM_PAGE_KERNEL_RW
    )) {
        log_error("Multiple-page range mapping failed.");

        for (uint64_t i = 0; i < VMM_TEST_RANGE_PAGES; i++) {
            pmm_free_page(range_physical_base + i * PMM_PAGE_SIZE);
        }

        halt_system();
    }

    log_success("Multiple-page virtual range mapped.");

    for (uint64_t i = 0; i < VMM_TEST_RANGE_PAGES; i++) {
        volatile uint64_t *page = (volatile uint64_t *)(
            VMM_TEST_VIRTUAL_BASE + i * PMM_PAGE_SIZE
        );

        uint64_t expected = 0x4541474C00000000ULL | (i + 1);

        page[0] = expected;
        page[1] = ~expected;

        if (page[0] != expected || page[1] != ~expected) {
            log_error("Range mapping memory verification failed.");
            halt_system();
        }

        translated = vmm_translate(
            VMM_TEST_VIRTUAL_BASE + i * PMM_PAGE_SIZE
        );

        uint64_t expected_physical =
            range_physical_base + i * PMM_PAGE_SIZE;

        if (translated != expected_physical) {
            log_error("Range mapping translation verification failed.");
            halt_system();
        }
    }

    log_success("All range memory read/write tests passed.");
    log_success("All range virtual-to-physical translations passed.");

    if (!vmm_unmap_range(
        VMM_TEST_VIRTUAL_BASE,
        VMM_TEST_RANGE_PAGES
    )) {
        log_error("Multiple-page range unmapping failed.");
        halt_system();
    }

    log_success("Multiple-page virtual range unmapped.");

    for (uint64_t i = 0; i < VMM_TEST_RANGE_PAGES; i++) {
        uint64_t address =
            VMM_TEST_VIRTUAL_BASE + i * PMM_PAGE_SIZE;

        if (vmm_translate(address) != VMM_FAILURE) {
            log_error("Range address remained mapped after unmapping.");
            halt_system();
        }
    }

    log_success("All range unmapped-address checks passed.");

    for (uint64_t i = 0; i < VMM_TEST_RANGE_PAGES; i++) {
        if (!pmm_free_page(range_physical_base + i * PMM_PAGE_SIZE)) {
            log_error("Could not release range physical allocation.");
            halt_system();
        }
    }

    log_success("All range physical allocations released.");

    uint64_t final_free_pages = pmm_get_free_pages();

    serial_write("[EagleOS] Free pages before VMM tests: ");
    serial_write_u64(initial_free_pages);
    serial_write("\n");

    serial_write("[EagleOS] Free pages after VMM tests: ");
    serial_write_u64(final_free_pages);
    serial_write("\n");

    if (final_free_pages > initial_free_pages) {
        log_error("VMM test unexpectedly increased free page count.");
        halt_system();
    }

    uint64_t retained_page_tables =
        initial_free_pages - final_free_pages;

    serial_write("[EagleOS] Page-table pages retained: ");
    serial_write_u64(retained_page_tables);
    serial_write("\n");

    log_success("VMM physical memory accounting completed.");
    log_success("All dynamic VMM mapping tests passed.");
}

static void test_pit(void) {
    log_info("Testing PIT hardware interrupts...");

    __asm__ volatile ("sti" ::: "memory");

    while (pit_get_ticks() < 10) {
        __asm__ volatile ("hlt" ::: "memory");
    }

    __asm__ volatile ("cli" ::: "memory");

    log_success("PIT hardware interrupts are working.");

    serial_write("[EagleOS] Timer ticks received: ");
    serial_write_u64(pit_get_ticks());
    serial_write("\n");
}

static void test_clock(void) {
    log_info("Testing system clock...");

    uint64_t starting_ticks = clock_get_ticks();
    uint64_t starting_uptime = clock_get_uptime_ms();

    __asm__ volatile ("sti" ::: "memory");

    if (!clock_sleep_ms(250)) {
        __asm__ volatile ("cli" ::: "memory");
        log_error("Clock sleep test failed.");
        halt_system();
    }

    __asm__ volatile ("cli" ::: "memory");

    uint64_t ending_ticks = clock_get_ticks();
    uint64_t ending_uptime = clock_get_uptime_ms();

    if (ending_ticks <= starting_ticks ||
        ending_uptime - starting_uptime < 250) {
        log_error("System clock test failed.");
        halt_system();
    }

    log_success("System clock is working.");

    serial_write("[EagleOS] Timer ticks: ");
    serial_write_u64(ending_ticks);
    serial_write("\n");

    serial_write("[EagleOS] System uptime: ");
    serial_write_u64(ending_uptime);
    serial_write(" ms\n");

    serial_write("[EagleOS] System uptime: ");
    serial_write_u64(clock_get_uptime_seconds());
    serial_write(" seconds\n");
}

static void test_rtc(void) {
    log_info("Testing real-time clock...");

    struct rtc_time current_time;

    if (!rtc_get_time(&current_time)) {
        log_error("Could not read real-time clock.");
        return;
    }

    log_success("Real-time clock is working.");

    serial_write("[EagleOS] RTC date: ");

    serial_write_u64(current_time.year);
    serial_write("-");

    if (current_time.month < 10) {
        serial_write("0");
    }

    serial_write_u64(current_time.month);
    serial_write("-");

    if (current_time.day < 10) {
        serial_write("0");
    }

    serial_write_u64(current_time.day);
    serial_write("\n");

    serial_write("[EagleOS] RTC time: ");

    if (current_time.hour < 10) {
        serial_write("0");
    }

    serial_write_u64(current_time.hour);
    serial_write(":");

    if (current_time.minute < 10) {
        serial_write("0");
    }

    serial_write_u64(current_time.minute);
    serial_write(":");

    if (current_time.second < 10) {
        serial_write("0");
    }

    serial_write_u64(current_time.second);
    serial_write("\n");
}

static void test_system_datetime(void) {
    log_info("Testing system date and time...");

    struct clock_datetime datetime;

    if (!clock_get_datetime(&datetime)) {
        log_error("Unable to retrieve system date and time.");
        return;
    }

    log_success("System date and time are working.");

    serial_write("[EagleOS] System date: ");

    serial_write_u64(datetime.year);
    serial_write("-");

    if (datetime.month < 10) {
        serial_write("0");
    }

    serial_write_u64(datetime.month);
    serial_write("-");

    if (datetime.day < 10) {
        serial_write("0");
    }

    serial_write_u64(datetime.day);
    serial_write("\n");

    serial_write("[EagleOS] System time: ");

    if (datetime.hour < 10) {
        serial_write("0");
    }

    serial_write_u64(datetime.hour);
    serial_write(":");

    if (datetime.minute < 10) {
        serial_write("0");
    }

    serial_write_u64(datetime.minute);
    serial_write(":");

    if (datetime.second < 10) {
        serial_write("0");
    }

    serial_write_u64(datetime.second);
    serial_write(".");

    if (datetime.millisecond < 100) {
        serial_write("0");
    }

    if (datetime.millisecond < 10) {
        serial_write("0");
    }

    serial_write_u64(datetime.millisecond);
    serial_write("\n");
}

static void test_system_clock_advancement(void) {
    log_info("Testing system clock advancement...");

    struct clock_datetime before;
    struct clock_datetime after;

    if (!clock_get_datetime(&before)) {
        log_error("Could not read initial system time.");
        halt_system();
    }

    __asm__ volatile ("sti" ::: "memory");

    if (!clock_sleep_ms(2200)) {
        __asm__ volatile ("cli" ::: "memory");
        log_error("System clock advancement test failed.");
        halt_system();
    }

    __asm__ volatile ("cli" ::: "memory");

    if (!clock_get_datetime(&after)) {
        log_error("Could not read updated system time.");
        halt_system();
    }

    uint64_t before_seconds =
        (uint64_t)before.hour * 3600 +
        (uint64_t)before.minute * 60 +
        before.second;

    uint64_t after_seconds =
        (uint64_t)after.hour * 3600 +
        (uint64_t)after.minute * 60 +
        after.second;

    uint64_t elapsed_seconds =
        (after_seconds + 86400 - before_seconds) % 86400;

    serial_write("[EagleOS] Elapsed system seconds: ");
    serial_write_u64(elapsed_seconds);
    serial_write("\n");

    serial_write("[EagleOS] System clock after test: ");

    if (after.hour < 10) serial_write("0");
    serial_write_u64(after.hour);
    serial_write(":");

    if (after.minute < 10) serial_write("0");
    serial_write_u64(after.minute);
    serial_write(":");

    if (after.second < 10) serial_write("0");
    serial_write_u64(after.second);
    serial_write(".");

    if (after.millisecond < 100) serial_write("0");
    if (after.millisecond < 10) serial_write("0");
    serial_write_u64(after.millisecond);
    serial_write("\n");

    if (elapsed_seconds < 2 || elapsed_seconds > 3) {
        log_error("System clock did not advance correctly.");
        halt_system();
    }

    log_success("System clock advancement passed.");
}

static void test_keyboard(void) {
    log_info("Starting keyboard input test...");

    if (!keyboard_is_initialized()) {
        log_error("Keyboard is not initialized.");
        halt_system();
    }

    log_success("Keyboard driver is ready.");

    serial_write("\n");
    serial_write("========================================\n");
    serial_write("         EAGLEOS KEYBOARD TEST\n");
    serial_write("========================================\n");
    serial_write("Type something in the QEMU window.\n");
    serial_write("Your keystrokes will appear below.\n");
    serial_write("========================================\n\n");

    for (;;) {
        __asm__ volatile ("cli" ::: "memory");

        char character = keyboard_read_char();

        if (character == 0) {
            __asm__ volatile ("sti; hlt" ::: "memory");
            continue;
        }

        __asm__ volatile ("sti" ::: "memory");

        if (character == '\n') {
            serial_write("\n");
        } else if (character == '\b') {
            serial_write("\b \b");
        } else if (character == '\t') {
            serial_write("\t");
        } else if (character >= 32 && character <= 126) {
            serial_write_char(character);
        }
    }
}

static void test_pci(void) {
    log_info("Testing PCI device enumeration...");

    if (!pci_is_initialized()) {
        log_error("PCI subsystem is not initialized.");
        halt_system();
    }

    size_t count = pci_get_device_count();

    serial_write("\n");
    serial_write("========================================\n");
    serial_write("          EAGLEOS PCI DEVICES\n");
    serial_write("========================================\n");

    serial_write("Discovered PCI devices: ");
    serial_write_u64(count);
    serial_write("\n\n");

    for (size_t i = 0; i < count; i++) {
        struct pci_device device;

        if (!pci_get_device(i, &device)) {
            continue;
        }

        serial_write("PCI Device ");
        serial_write_u64(i);
        serial_write("\n");

        serial_write("  Bus: ");
        serial_write_u64(device.bus);
        serial_write("\n");

        serial_write("  Device: ");
        serial_write_u64(device.device);
        serial_write("\n");

        serial_write("  Function: ");
        serial_write_u64(device.function);
        serial_write("\n");

        serial_write("  Vendor ID: ");
        serial_write_hex(device.vendor_id);
        serial_write("\n");

        serial_write("  Device ID: ");
        serial_write_hex(device.device_id);
        serial_write("\n");

        serial_write("  Class: ");
        serial_write_hex(device.class_code);
        serial_write("\n");

        serial_write("  Subclass: ");
        serial_write_hex(device.subclass);
        serial_write("\n");

        serial_write("  Programming Interface: ");
        serial_write_hex(device.programming_interface);
        serial_write("\n");

        serial_write("  Header Type: ");
        serial_write_hex(device.header_type);
        serial_write("\n");

        serial_write("  Command Register: ");
        serial_write_hex(device.command);
        serial_write("\n");

        serial_write("  Status Register: ");
        serial_write_hex(device.status);
        serial_write("\n");

        serial_write("  Interrupt Line: ");
        serial_write_u64(device.interrupt_line);
        serial_write("\n");

        serial_write("  BARs:\n");

        for (uint8_t bar = 0; bar < device.bar_count; bar++) {
            serial_write("    BAR");
            serial_write_u64(bar);
            serial_write(": ");
            serial_write_hex(device.bars[bar]);
            serial_write(" | ");
            serial_write(pci_bar_type_name(device.bar_info[bar].type));
            serial_write(" | Address: ");
            serial_write_hex(device.bar_info[bar].address);

            if (device.bar_info[bar].prefetchable) {
                serial_write(" | Prefetchable");
            }

            serial_write("\n");
        }

        serial_write("\n");
    }

    struct pci_device storage;

    serial_write("Searching for mass storage controllers...\n");

    if (pci_find_class(0x01, 0x00, 0xFF, &storage)) {
        log_success("PCI mass storage controller detected.");
    } else if (pci_find_class(0x01, 0x06, 0xFF, &storage)) {
        log_success("PCI SATA controller detected.");
    } else {
        log_info("No matching mass storage controller found.");
    }

    log_success("PCI enumeration test completed.");
}

static void test_pci_configuration(void) {
    log_info("Testing PCI device configuration...");

    struct pci_device sata;

    if (!pci_find_class(0x01, 0x06, 0x01, &sata)) {
        log_error("AHCI SATA controller not found.");
        return;
    }

    log_success("AHCI SATA controller located.");

    serial_write("[EagleOS] PCI Command Register: ");
    serial_write_hex(pci_read_command(&sata));
    serial_write("\n");

    serial_write("[EagleOS] PCI Status Register: ");
    serial_write_hex(pci_read_status(&sata));
    serial_write("\n");

    if (pci_enable_memory_space(&sata)) {
        log_success("PCI memory-space access enabled.");
    } else {
        log_error("Failed to enable PCI memory-space access.");
    }

    serial_write("[EagleOS] Updated PCI Command Register: ");
    serial_write_hex(sata.command);
    serial_write("\n");

    if (!pci_refresh_device(&sata)) {
        log_error("Could not refresh PCI device information.");
        return;
    }

    log_success("PCI device configuration test completed.");
}

static void test_ahci(void) {
    log_info("Initializing AHCI storage controller...");

    if (!ahci_init()) {
        log_error("AHCI initialization failed.");
        return;
    }

    const struct ahci_info *info = ahci_get_info();

    if (info == 0) {
        log_error("Could not retrieve AHCI controller information.");
        return;
    }

    log_success("AHCI controller initialized.");

    serial_write("[EagleOS] AHCI ABAR: ");
    serial_write_hex(info->abar_physical);
    serial_write("\n");

    serial_write("[EagleOS] AHCI Version: ");
    serial_write_hex(info->version);
    serial_write("\n");

    serial_write("[EagleOS] AHCI Capabilities: ");
    serial_write_hex(info->capabilities);
    serial_write("\n");

    serial_write("[EagleOS] AHCI Global Control: ");
    serial_write_hex(info->global_control);
    serial_write("\n");

    serial_write("[EagleOS] AHCI Implemented Ports: ");
    serial_write_hex(info->implemented_ports);
    serial_write("\n");

    serial_write("[EagleOS] Hardware Port Count: ");
    serial_write_u64(info->hardware_port_count);
    serial_write("\n");

    serial_write("[EagleOS] Implemented Port Count: ");
    serial_write_u64(info->implemented_port_count);
    serial_write("\n");

    serial_write("[EagleOS] Detected Device Count: ");
    serial_write_u64(info->detected_device_count);
    serial_write("\n");

    serial_write("[EagleOS] 64-bit DMA Support: ");
    serial_write(info->supports_64bit ? "Yes\n" : "No\n");

    for (uint8_t i = 0; i < AHCI_MAX_PORTS; i++) {
        const struct ahci_port_info *port = &info->ports[i];

        if (!port->implemented) {
            continue;
        }

        serial_write("[EagleOS] AHCI Port ");
        serial_write_u64(port->port_number);

        serial_write(" | SATA Status: ");
        serial_write_hex(port->sata_status);

        serial_write(" | Signature: ");
        serial_write_hex(port->signature);

        serial_write(" | Device: ");
        serial_write(ahci_device_type_name(port->device_type));

        serial_write("\n");
    }

    log_success("AHCI controller inspection completed.");
}

static void test_blockdev_storage(void) {
    log_info("Initializing block device manager...");

    if (!ahci_is_initialized()) {
        log_error("AHCI controller is not initialized.");
        return;
    }

    if (!blockdev_init()) {
        log_error("Block device manager initialization failed.");
        return;
    }

    log_success("Block device manager initialized.");

    uint32_t device_count = blockdev_get_device_count();

    serial_write("[EagleOS] Registered block devices: ");
    serial_write_u64(device_count);
    serial_write("\n");

    if (device_count == 0) {
        log_warning("No supported block devices were registered.");
        return;
    }

    for (uint32_t i = 0; i < device_count; i++) {
        struct block_device device;

        if (!blockdev_get_device(i, &device)) {
            log_error("Could not retrieve block device information.");
            continue;
        }

        serial_write("\n[EagleOS] Block Device ");
        serial_write_u64(device.id);
        serial_write("\n");

        serial_write("[EagleOS] Type: ");
        serial_write(blockdev_type_name(device.type));
        serial_write("\n");

        serial_write("[EagleOS] Controller port: ");
        serial_write_u64(device.controller_port);
        serial_write("\n");

        serial_write("[EagleOS] Block size: ");
        serial_write_u64(device.block_size);
        serial_write(" bytes\n");

        serial_write("[EagleOS] Block count: ");
        serial_write_u64(device.block_count);
        serial_write("\n");

        serial_write("[EagleOS] Read only: ");
        serial_write(device.read_only ? "Yes\n" : "No\n");

        log_success("Block device information retrieved.");

        if (device.block_size != 2048) {
            log_warning("Unexpected optical-disc block size.");
            continue;
        }

        uint8_t sector[2048];

        log_info("Reading ISO 9660 volume descriptor through block device manager...");

        if (!blockdev_read(device.id, 16, 1, sector)) {
            log_error("Block device read failed.");
            continue;
        }

        serial_write("[EagleOS] Sector signature: ");

        for (uint32_t j = 0; j < 7; j++) {
            serial_write_hex(sector[j]);
            serial_write(" ");
        }

        serial_write("\n");

        if (
            sector[0] == 1 &&
            sector[1] == 'C' &&
            sector[2] == 'D' &&
            sector[3] == '0' &&
            sector[4] == '0' &&
            sector[5] == '1' &&
            sector[6] == 1
        ) {
            log_success("ISO 9660 volume descriptor verified.");
            log_success("Block device manager successfully read storage data.");
        } else {
            log_warning("Sector was read, but ISO 9660 signature did not match.");
        }
    }
}

static void test_iso9660(void) {
    log_info("Testing ISO 9660 filesystem...");

    if (!blockdev_get_device_count()) {
        log_warning("No block devices available for filesystem testing.");
        return;
    }

    uint32_t mounted_device = UINT32_MAX;

    for (uint32_t i = 0; i < blockdev_get_device_count(); i++) {
        struct block_device device;

        if (!blockdev_get_device(i, &device)) {
            continue;
        }

        if (device.block_size != ISO9660_BLOCK_SIZE) {
            continue;
        }

        if (iso9660_mount(device.id)) {
            mounted_device = device.id;
            break;
        }
    }

    if (mounted_device == UINT32_MAX) {
        log_warning("No mountable ISO 9660 filesystem found.");
        return;
    }

    log_success("ISO 9660 filesystem mounted.");

    const struct iso9660_volume *volume =
        iso9660_get_volume();

    if (volume == NULL) {
        log_error("Could not retrieve filesystem information.");
        return;
    }

    serial_write("[EagleOS] Filesystem device: ");
    serial_write_u64(volume->device_id);
    serial_write("\n");

    serial_write("[EagleOS] Volume blocks: ");
    serial_write_u64(volume->volume_blocks);
    serial_write("\n");

    serial_write("[EagleOS] Logical block size: ");
    serial_write_u64(volume->logical_block_size);
    serial_write("\n");

    serial_write("[EagleOS] Root directory LBA: ");
    serial_write_u64(volume->root_directory_lba);
    serial_write("\n");

    serial_write("[EagleOS] Root directory size: ");
    serial_write_u64(volume->root_directory_size);
    serial_write(" bytes\n");

    log_info("Reading ISO 9660 root directory...");

    struct iso9660_directory_entry entries[32];
    size_t entry_count = 0;

    if (!iso9660_list_directory(
        "/",
        entries,
        32,
        &entry_count
    )) {
        log_error("Could not read ISO 9660 root directory.");
        return;
    }

    serial_write("\n");
    serial_write("========================================\n");
    serial_write("       EAGLEOS ISO 9660 DIRECTORY\n");
    serial_write("========================================\n");

    serial_write("[EagleOS] Root directory entries: ");
    serial_write_u64(entry_count);
    serial_write("\n\n");

    size_t displayed = entry_count;

    if (displayed > 32) {
        displayed = 32;
    }

    for (size_t i = 0; i < displayed; i++) {
        serial_write(entries[i].is_directory ? "[DIR]  " : "[FILE] ");
        serial_write(entries[i].name);

        serial_write(" | Size: ");
        serial_write_u64(entries[i].size);
        serial_write(" bytes\n");
    }

    if (entry_count > 32) {
        log_warning("Directory listing truncated to 32 entries.");
    }

    log_success("ISO 9660 root directory successfully read.");
}

static void test_iso9660_file_access(void) {
    log_info("Testing ISO 9660 file access...");

    if (!iso9660_is_mounted()) {
        log_error("ISO 9660 filesystem is not mounted.");
        return;
    }

    struct iso9660_directory_entry entry;

    /*
     * Test directory lookup.
     */

    log_info("Testing directory lookup...");

    if (!iso9660_find("/BOOT", &entry)) {
        log_error("Could not find /BOOT.");
        return;
    }

    if (!entry.is_directory) {
        log_error("/BOOT is not a directory.");
        return;
    }

    log_success("Directory lookup passed.");

    /*
     * Test nested directory listing.
     */

    log_info("Testing /BOOT directory traversal...");

    struct iso9660_directory_entry entries[32];
    size_t entry_count = 0;

    if (!iso9660_list_directory(
        "/BOOT",
        entries,
        32,
        &entry_count
    )) {
        log_error("Could not list /BOOT.");
        return;
    }

    serial_write("[EagleOS] /BOOT entries: ");
    serial_write_u64(entry_count);
    serial_write("\n");

    size_t displayed = entry_count;

    if (displayed > 32) {
        displayed = 32;
    }

    for (size_t i = 0; i < displayed; i++) {
        serial_write(entries[i].is_directory ? "[DIR]  " : "[FILE] ");
        serial_write(entries[i].name);
        serial_write("\n");
    }

    log_success("Nested directory listing passed.");

    /*
     * Test another directory.
     */

    log_info("Testing /EFI directory traversal...");

    entry_count = 0;

    if (iso9660_list_directory(
        "/EFI",
        entries,
        32,
        &entry_count
    )) {
        serial_write("[EagleOS] /EFI entries: ");
        serial_write_u64(entry_count);
        serial_write("\n");

        displayed = entry_count;

        if (displayed > 32) {
            displayed = 32;
        }

        for (size_t i = 0; i < displayed; i++) {
            serial_write(entries[i].is_directory ? "[DIR]  " : "[FILE] ");
            serial_write(entries[i].name);
            serial_write("\n");
        }

        log_success("EFI directory traversal passed.");
    } else {
        log_warning("Could not list /EFI.");
    }

    /*
     * Test file lookup.
     */

    log_info("Testing file lookup...");

    if (!iso9660_find("/BOOT.CAT", &entry)) {
        log_error("Could not find /BOOT.CAT.");
        return;
    }

    if (entry.is_directory) {
        log_error("/BOOT.CAT unexpectedly resolved to a directory.");
        return;
    }

    log_success("File lookup passed.");

    /*
     * Test reading file contents.
     */

    log_info("Testing ISO 9660 file reading...");

    uint8_t file_buffer[ISO9660_BLOCK_SIZE];
    size_t bytes_read = 0;

    if (!iso9660_read_file(
        "/BOOT.CAT",
        file_buffer,
        sizeof(file_buffer),
        &bytes_read
    )) {
        log_error("Could not read /BOOT.CAT.");
        return;
    }

    serial_write("[EagleOS] File size: ");
    serial_write_u64(entry.size);
    serial_write(" bytes\n");

    serial_write("[EagleOS] Bytes read: ");
    serial_write_u64(bytes_read);
    serial_write("\n");

    if (bytes_read != entry.size) {
        log_error("File read size does not match directory entry.");
        return;
    }

    serial_write("[EagleOS] First 16 bytes: ");

    size_t preview_size = bytes_read;

    if (preview_size > 16) {
        preview_size = 16;
    }

    for (size_t i = 0; i < preview_size; i++) {
        serial_write_hex(file_buffer[i]);
        serial_write(" ");
    }

    serial_write("\n");

    log_success("ISO 9660 file contents successfully read.");
    log_success("All filesystem file-access tests passed.");
}

static void test_filesystem_manager(void) {
    log_info("Testing filesystem manager...");

    if (!filesystem_init()) {
        log_error("Filesystem manager initialization failed.");
        return;
    }

    log_success("Filesystem manager initialized.");

    /*
     * Register ISO 9660.
     */

    log_info("Registering ISO 9660 filesystem driver...");

    const struct filesystem_type *iso_type =
        filesystem_get_iso9660_type();

    if (iso_type == NULL ||
        !filesystem_register(iso_type)) {
        log_error("ISO 9660 filesystem registration failed.");
        return;
    }

    log_success("ISO 9660 filesystem registered.");

    /*
     * Retrieve the device mounted by the previous ISO tests.
     */

    if (!iso9660_is_mounted()) {
        log_error("No ISO 9660 volume is currently mounted.");
        return;
    }

    const struct iso9660_volume *volume =
        iso9660_get_volume();

    if (volume == NULL) {
        log_error("Could not retrieve ISO 9660 volume information.");
        return;
    }

    uint32_t device_id = volume->device_id;

    serial_write("[EagleOS] Filesystem device: ");
    serial_write_u64(device_id);
    serial_write("\n");

    /*
     * Mount through the generic filesystem manager.
     */

    log_info("Mounting filesystem at /...");

    if (!filesystem_mount_device(device_id, "/")) {
        log_error("Filesystem manager mount failed.");
        return;
    }

    log_success("Filesystem mounted at /.");

    /*
     * Test generic directory listing.
     */

    log_info("Testing generic directory listing...");

    struct filesystem_entry entries[16];
    size_t entry_count = 0;

    if (!filesystem_list_directory(
        "/",
        entries,
        16,
        &entry_count
    )) {
        log_error("Generic directory listing failed.");
        return;
    }

    serial_write("[EagleOS] Root directory entries: ");
    serial_write_u64(entry_count);
    serial_write("\n");

    size_t displayed = entry_count;

    if (displayed > 16) {
        displayed = 16;
    }

    for (size_t i = 0; i < displayed; i++) {
        serial_write(
            entries[i].is_directory ? "[DIR]  " : "[FILE] "
        );

        serial_write(entries[i].name);
        serial_write(" | Size: ");
        serial_write_u64(entries[i].size);
        serial_write(" bytes\n");
    }

    log_success("Generic directory listing passed.");

    /*
     * Test generic directory traversal.
     */

    log_info("Testing generic directory traversal...");

    entry_count = 0;

    if (!filesystem_list_directory(
        "/BOOT",
        entries,
        16,
        &entry_count
    )) {
        log_error("Generic nested directory listing failed.");
        return;
    }

    serial_write("[EagleOS] /BOOT entries: ");
    serial_write_u64(entry_count);
    serial_write("\n");

    log_success("Generic directory traversal passed.");

    /*
     * Test generic file lookup.
     */

    log_info("Testing generic file lookup...");

    struct filesystem_entry file;

    if (!filesystem_find("/BOOT.CAT", &file)) {
        log_error("Generic file lookup failed.");
        return;
    }

    if (file.is_directory) {
        log_error("File lookup returned a directory.");
        return;
    }

    log_success("Generic file lookup passed.");

    /*
     * Test generic file reading.
     */

    log_info("Testing generic file reading...");

    uint8_t file_buffer[2048];
    size_t bytes_read = 0;

    if (!filesystem_read_file(
        "/BOOT.CAT",
        file_buffer,
        sizeof(file_buffer),
        &bytes_read
    )) {
        log_error("Generic file reading failed.");
        return;
    }

    serial_write("[EagleOS] File size: ");
    serial_write_u64(file.size);
    serial_write(" bytes\n");

    serial_write("[EagleOS] Bytes read: ");
    serial_write_u64(bytes_read);
    serial_write("\n");

    if (bytes_read != file.size) {
        log_error("Generic file size mismatch.");
        return;
    }

    log_success("Generic file reading passed.");

    /*
     * Display filesystem manager information.
     */

    serial_write("[EagleOS] Registered filesystem types: ");
    serial_write_u64(filesystem_get_type_count());
    serial_write("\n");

    serial_write("[EagleOS] Mounted filesystems: ");
    serial_write_u64(filesystem_get_mount_count());
    serial_write("\n");

    const struct filesystem_mount *mount =
        filesystem_get_mount(0);

    if (mount != NULL) {
        serial_write("[EagleOS] Mount point: ");
        serial_write(mount->mount_path);
        serial_write("\n");

        serial_write("[EagleOS] Filesystem: ");
        serial_write(mount->type->name);
        serial_write("\n");
    }

    log_success("All filesystem manager tests passed.");
}

void _start(void) {
    __asm__ volatile ("cli");

    serial_init();

    serial_write("\n");

    log_info("Initializing interrupt descriptor table...");

    if (!idt_init()) {
        log_error("Interrupt descriptor table initialization failed.");
        halt_system();
    }

    log_success("Interrupt descriptor table initialized.");

    log_info("Initializing programmable interrupt controller...");

    pic_init();

    log_success("Programmable interrupt controller initialized.");

    uint16_t pic_mask = pic_get_mask();

    serial_write("[EagleOS] Master PIC mask: ");
    serial_write_hex(pic_mask & 0xFF);
    serial_write("\n");

    serial_write("[EagleOS] Slave PIC mask: ");
    serial_write_hex((pic_mask >> 8) & 0xFF);
    serial_write("\n");

    log_info("Initializing hardware IRQ subsystem...");

    if (!irq_init()) {
        log_error("Hardware IRQ subsystem initialization failed.");
        halt_system();
    }

    log_info("Initializing PS/2 keyboard...");

    if (!keyboard_init()) {
        log_error("PS/2 keyboard initialization failed.");
        halt_system();
    }

    log_success("PS/2 keyboard initialized.");

    log_success("Hardware IRQ subsystem initialized.");

    log_info("Initializing programmable interval timer...");

    if (!pit_init(PIT_DEFAULT_FREQUENCY)) {
        log_error("Programmable interval timer initialization failed.");
        halt_system();
    }

    log_info("Initializing PCI subsystem...");

    if (!pci_init()) {
        log_error("PCI subsystem initialization failed.");
        halt_system();
    }

    log_success("PCI subsystem initialized.");

    log_success("Programmable interval timer initialized.");

    serial_write("[EagleOS] PIT frequency: ");
    serial_write_u64(pit_get_frequency());
    serial_write(" Hz\n");

    log_info("Initializing system clock...");

    if (!clock_init()) {
        log_error("System clock initialization failed.");
        halt_system();
    }

    log_success("System clock initialized.");

    log_info("Initializing real-time clock...");

    if (!rtc_init()) {
        log_error("Real-time clock initialization failed.");
        halt_system();
    }

    log_success("Real-time clock initialized.");

    log_info("Synchronizing system clock with RTC...");

    if (!clock_sync_from_rtc()) {
        log_error("System clock synchronization failed.");
        halt_system();
    }

    log_success("System clock synchronized.");

    serial_write("================================\n");
    serial_write("       EAGLEOS KERNEL\n");
    serial_write("================================\n");

    log_debug("Serial port COM1 initialized.");
    log_info("Kernel starting...");

    if (!LIMINE_BASE_REVISION_SUPPORTED(base_revision)) {
        log_error("Unsupported Limine base revision.");
        halt_system();
    }

    log_success("Limine protocol initialized.");

    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        log_error("Framebuffer unavailable.");
        halt_system();
    }

    framebuffer = framebuffer_request.response->framebuffers[0];

    if (framebuffer == NULL ||
        framebuffer->bpp != 32 ||
        framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB) {
        log_error("Unsupported framebuffer format.");
        halt_system();
    }

    log_success("Framebuffer initialized.");

    detect_memory();
    inspect_virtual_memory();

    log_info("Initializing virtual memory manager...");

    if (!vmm_init(hhdm_offset)) {
        log_error("Virtual memory manager initialization failed.");
        halt_system();
    }

    log_success("Virtual memory manager initialized.");

    test_vmm();

    log_info("Initializing physical memory manager...");

    if (!pmm_init(memory_map_request.response)) {
        log_error("Physical memory manager initialization failed.");
        halt_system();
    }

    log_success("Physical memory manager initialized.");

    serial_write("[EagleOS] Free physical pages: ");
    serial_write_u64(pmm_get_free_pages());
    serial_write("\n");

    test_pmm();
    test_vmm_mapping();

    log_info("Initializing kernel display...");
    draw_boot_screen();

    test_pit();
    test_clock();
    test_rtc();
    test_system_datetime();
    test_system_clock_advancement();
    test_pci();
    test_pci_configuration();
    test_ahci();
    test_blockdev_storage();
    test_iso9660();
    test_iso9660_file_access();
    test_filesystem_manager();
    
    log_success("EagleOS kernel initialization complete.");

    test_keyboard();
}