#include <stdint.h>
#include <stddef.h>

#include <ahci.h>
#include <pci.h>
#include <pmm.h>
#include <vmm.h>

#define AHCI_ABAR_VIRTUAL_BASE 0xFFFFA00000000000ULL
#define AHCI_DMA_VIRTUAL_BASE  0xFFFFA00000100000ULL

#define AHCI_PAGE_SIZE 4096ULL
#define AHCI_WAIT_LIMIT 10000000ULL

#define AHCI_PORT_BASE 0x100
#define AHCI_PORT_SIZE 0x80

#define AHCI_REG_CAP 0x00
#define AHCI_REG_GHC 0x04
#define AHCI_REG_PI  0x0C
#define AHCI_REG_VS  0x10

#define AHCI_PORT_CLB  0x00
#define AHCI_PORT_CLBU 0x04
#define AHCI_PORT_FB   0x08
#define AHCI_PORT_FBU  0x0C
#define AHCI_PORT_IS   0x10
#define AHCI_PORT_IE   0x14
#define AHCI_PORT_CMD  0x18
#define AHCI_PORT_TFD  0x20
#define AHCI_PORT_SIG  0x24
#define AHCI_PORT_SSTS 0x28
#define AHCI_PORT_SERR 0x30
#define AHCI_PORT_CI   0x38
#define AHCI_PORT_SACT 0x34

#define AHCI_CMD_ST  (1U << 0)
#define AHCI_CMD_FRE (1U << 4)
#define AHCI_CMD_FR  (1U << 14)
#define AHCI_CMD_CR  (1U << 15)
#define AHCI_CMD_ATAPI (1U << 24)

#define AHCI_TFD_BSY (1U << 7)
#define AHCI_TFD_DRQ (1U << 3)
#define AHCI_TFD_ERR (1U << 0)

#define AHCI_IS_TFES (1U << 30)

#define AHCI_SIGNATURE_SATA  0x00000101
#define AHCI_SIGNATURE_ATAPI 0xEB140101
#define AHCI_SIGNATURE_SEMB  0xC33C0101
#define AHCI_SIGNATURE_PM    0x96690101

#define AHCI_BAR_INDEX 5

#define AHCI_DMA_CLB_OFFSET  0x0000
#define AHCI_DMA_FB_OFFSET   0x1000
#define AHCI_DMA_TABLE_OFFSET 0x2000
#define AHCI_DMA_DATA_OFFSET 0x3000
#define AHCI_DMA_PORT_STRIDE 0x10000

#define ATAPI_PACKET_COMMAND 0xA0
#define ATAPI_READ_CAPACITY  0x25
#define ATAPI_READ_12        0xA8

#define ATAPI_BLOCK_SIZE_MAX 4096

struct ahci_command_header {
    uint16_t flags;
    uint16_t prdt_length;
    uint32_t transferred_bytes;
    uint32_t table_base;
    uint32_t table_base_upper;
    uint32_t reserved[4];
} __attribute__((packed));

struct ahci_prdt_entry {
    uint32_t data_base;
    uint32_t data_base_upper;
    uint32_t reserved;
    uint32_t byte_count;
} __attribute__((packed));

struct ahci_command_table {
    uint8_t command_fis[64];
    uint8_t packet_command[16];
    uint8_t reserved[48];
    struct ahci_prdt_entry prdt[1];
} __attribute__((packed));

struct ahci_port_dma {
    uint64_t command_list_physical;
    uint64_t fis_physical;
    uint64_t command_table_physical;
    uint64_t data_physical;

    struct ahci_command_header *command_list;
    uint8_t *fis;
    struct ahci_command_table *command_table;
    uint8_t *data;

    uint8_t prepared;
};

static struct ahci_info controller_info;

static struct ahci_port_dma port_dma[AHCI_MAX_PORTS];

static volatile uint32_t *abar;

static uint32_t mmio_read(uint32_t offset) {
    return abar[offset / 4];
}

static void mmio_write(uint32_t offset, uint32_t value) {
    abar[offset / 4] = value;
}

static uint32_t port_read(uint8_t port, uint32_t offset) {
    uint32_t address =
        AHCI_PORT_BASE +
        ((uint32_t)port * AHCI_PORT_SIZE) +
        offset;

    return mmio_read(address);
}

static void port_write(uint8_t port, uint32_t offset, uint32_t value) {
    uint32_t address =
        AHCI_PORT_BASE +
        ((uint32_t)port * AHCI_PORT_SIZE) +
        offset;

    mmio_write(address, value);
}

static void zero_memory(void *memory, uint64_t size) {
    volatile uint8_t *bytes = (volatile uint8_t *)memory;

    for (uint64_t i = 0; i < size; i++) {
        bytes[i] = 0;
    }
}

static void copy_memory(
    void *destination,
    const void *source,
    uint64_t size
) {
    uint8_t *dest = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;

    for (uint64_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}

static int wait_register_clear(
    uint8_t port,
    uint32_t offset,
    uint32_t mask
) {
    for (uint64_t i = 0; i < AHCI_WAIT_LIMIT; i++) {
        if (!(port_read(port, offset) & mask)) {
            return 1;
        }

        __asm__ volatile ("pause");
    }

    return 0;
}

static int stop_port(uint8_t port) {
    uint32_t command = port_read(port, AHCI_PORT_CMD);

    command &= ~AHCI_CMD_ST;

    port_write(port, AHCI_PORT_CMD, command);

    if (!wait_register_clear(port, AHCI_PORT_CMD, AHCI_CMD_CR)) {
        return 0;
    }

    command = port_read(port, AHCI_PORT_CMD);
    command &= ~AHCI_CMD_FRE;

    port_write(port, AHCI_PORT_CMD, command);

    if (!wait_register_clear(port, AHCI_PORT_CMD, AHCI_CMD_FR)) {
        return 0;
    }

    return 1;
}

static int start_port(uint8_t port, uint8_t atapi) {
    uint32_t command = port_read(port, AHCI_PORT_CMD);

    command |= AHCI_CMD_FRE;

    if (atapi) {
        command |= AHCI_CMD_ATAPI;
    } else {
        command &= ~AHCI_CMD_ATAPI;
    }

    port_write(port, AHCI_PORT_CMD, command);

    command |= AHCI_CMD_ST;

    port_write(port, AHCI_PORT_CMD, command);

    return 1;
}

static void release_port_dma(uint8_t port) {
    struct ahci_port_dma *dma = &port_dma[port];

    uint64_t virtual_base =
        AHCI_DMA_VIRTUAL_BASE +
        ((uint64_t)port * AHCI_DMA_PORT_STRIDE);

    if (dma->command_list_physical != PMM_FAILURE &&
        dma->command_list_physical != 0) {
        vmm_unmap_page(virtual_base + AHCI_DMA_CLB_OFFSET);
        pmm_free_page(dma->command_list_physical);
    }

    if (dma->fis_physical != PMM_FAILURE &&
        dma->fis_physical != 0) {
        vmm_unmap_page(virtual_base + AHCI_DMA_FB_OFFSET);
        pmm_free_page(dma->fis_physical);
    }

    if (dma->command_table_physical != PMM_FAILURE &&
        dma->command_table_physical != 0) {
        vmm_unmap_page(virtual_base + AHCI_DMA_TABLE_OFFSET);
        pmm_free_page(dma->command_table_physical);
    }

    if (dma->data_physical != PMM_FAILURE &&
        dma->data_physical != 0) {
        vmm_unmap_page(virtual_base + AHCI_DMA_DATA_OFFSET);
        pmm_free_page(dma->data_physical);
    }

    zero_memory(dma, sizeof(*dma));

    dma->command_list_physical = PMM_FAILURE;
    dma->fis_physical = PMM_FAILURE;
    dma->command_table_physical = PMM_FAILURE;
    dma->data_physical = PMM_FAILURE;
}

static int allocate_port_dma(uint8_t port) {
    struct ahci_port_dma *dma = &port_dma[port];

    uint64_t virtual_base =
        AHCI_DMA_VIRTUAL_BASE +
        ((uint64_t)port * AHCI_DMA_PORT_STRIDE);

    dma->command_list_physical = PMM_FAILURE;
    dma->fis_physical = PMM_FAILURE;
    dma->command_table_physical = PMM_FAILURE;
    dma->data_physical = PMM_FAILURE;

    dma->command_list_physical = pmm_alloc_page();

    if (dma->command_list_physical == PMM_FAILURE) {
        goto failure;
    }

    dma->fis_physical = pmm_alloc_page();

    if (dma->fis_physical == PMM_FAILURE) {
        goto failure;
    }

    dma->command_table_physical = pmm_alloc_page();

    if (dma->command_table_physical == PMM_FAILURE) {
        goto failure;
    }

    dma->data_physical = pmm_alloc_page();

    if (dma->data_physical == PMM_FAILURE) {
        goto failure;
    }

    if (!vmm_map_page(
        virtual_base + AHCI_DMA_CLB_OFFSET,
        dma->command_list_physical,
        VMM_PAGE_KERNEL_RW
    )) {
        goto failure;
    }

    if (!vmm_map_page(
        virtual_base + AHCI_DMA_FB_OFFSET,
        dma->fis_physical,
        VMM_PAGE_KERNEL_RW
    )) {
        vmm_unmap_page(virtual_base + AHCI_DMA_CLB_OFFSET);
        goto failure;
    }

    if (!vmm_map_page(
        virtual_base + AHCI_DMA_TABLE_OFFSET,
        dma->command_table_physical,
        VMM_PAGE_KERNEL_RW
    )) {
        vmm_unmap_page(virtual_base + AHCI_DMA_CLB_OFFSET);
        vmm_unmap_page(virtual_base + AHCI_DMA_FB_OFFSET);
        goto failure;
    }

    if (!vmm_map_page(
        virtual_base + AHCI_DMA_DATA_OFFSET,
        dma->data_physical,
        VMM_PAGE_KERNEL_RW
    )) {
        vmm_unmap_page(virtual_base + AHCI_DMA_CLB_OFFSET);
        vmm_unmap_page(virtual_base + AHCI_DMA_FB_OFFSET);
        vmm_unmap_page(virtual_base + AHCI_DMA_TABLE_OFFSET);
        goto failure;
    }

    dma->command_list =
        (struct ahci_command_header *)(virtual_base + AHCI_DMA_CLB_OFFSET);

    dma->fis =
        (uint8_t *)(virtual_base + AHCI_DMA_FB_OFFSET);

    dma->command_table =
        (struct ahci_command_table *)(virtual_base + AHCI_DMA_TABLE_OFFSET);

    dma->data =
        (uint8_t *)(virtual_base + AHCI_DMA_DATA_OFFSET);

    zero_memory(dma->command_list, AHCI_PAGE_SIZE);
    zero_memory(dma->fis, AHCI_PAGE_SIZE);
    zero_memory(dma->command_table, AHCI_PAGE_SIZE);
    zero_memory(dma->data, AHCI_PAGE_SIZE);

    dma->prepared = 1;

    return 1;

failure:
    release_port_dma(port);
    return 0;
}

static int prepare_port(uint8_t port) {
    struct ahci_port_info *info = &controller_info.ports[port];

    if (!stop_port(port)) {
        return 0;
    }

    if (!allocate_port_dma(port)) {
        return 0;
    }

    struct ahci_port_dma *dma = &port_dma[port];

    port_write(
        port,
        AHCI_PORT_CLB,
        (uint32_t)dma->command_list_physical
    );

    port_write(
        port,
        AHCI_PORT_CLBU,
        (uint32_t)(dma->command_list_physical >> 32)
    );

    port_write(
        port,
        AHCI_PORT_FB,
        (uint32_t)dma->fis_physical
    );

    port_write(
        port,
        AHCI_PORT_FBU,
        (uint32_t)(dma->fis_physical >> 32)
    );

    port_write(port, AHCI_PORT_IS, 0xFFFFFFFF);
    port_write(port, AHCI_PORT_SERR, 0xFFFFFFFF);
    port_write(port, AHCI_PORT_IE, 0);

    if (!start_port(port, info->device_type == AHCI_DEVICE_ATAPI)) {
        release_port_dma(port);
        return 0;
    }

    info->command_engine_ready = 1;

    return 1;
}

static int wait_command_ready(uint8_t port) {
    for (uint64_t i = 0; i < AHCI_WAIT_LIMIT; i++) {
        uint32_t task_file = port_read(port, AHCI_PORT_TFD);

        if (!(task_file & (AHCI_TFD_BSY | AHCI_TFD_DRQ))) {
            return 1;
        }

        __asm__ volatile ("pause");
    }

    return 0;
}

static int issue_packet(
    uint8_t port,
    const uint8_t *packet,
    uint32_t transfer_size
) {
    if (port >= AHCI_MAX_PORTS) {
        return 0;
    }

    struct ahci_port_dma *dma = &port_dma[port];

    if (!dma->prepared || transfer_size == 0 ||
        transfer_size > AHCI_PAGE_SIZE) {
        return 0;
    }

    if (!wait_command_ready(port)) {
        return 0;
    }

    uint32_t active_commands = port_read(port, AHCI_PORT_CI);
    uint32_t active_queued = port_read(port, AHCI_PORT_SACT);

    if ((active_commands | active_queued) & 1U) {
        return 0;
    }

    struct ahci_command_header *header = &dma->command_list[0];
    struct ahci_command_table *table = dma->command_table;

    zero_memory(header, sizeof(*header));
    zero_memory(table, sizeof(*table));
    zero_memory(dma->data, AHCI_PAGE_SIZE);

    header->flags = 5 | (1U << 5);
    header->prdt_length = 1;

    header->table_base =
        (uint32_t)dma->command_table_physical;

    header->table_base_upper =
        (uint32_t)(dma->command_table_physical >> 32);

    table->command_fis[0] = 0x27;
    table->command_fis[1] = 0x80;
    table->command_fis[2] = ATAPI_PACKET_COMMAND;
    table->command_fis[3] = 1;

    for (uint32_t i = 0; i < 12; i++) {
        table->packet_command[i] = packet[i];
    }

    table->prdt[0].data_base =
        (uint32_t)dma->data_physical;

    table->prdt[0].data_base_upper =
        (uint32_t)(dma->data_physical >> 32);

    table->prdt[0].reserved = 0;

    table->prdt[0].byte_count =
        (transfer_size - 1) | (1U << 31);

    port_write(port, AHCI_PORT_IS, 0xFFFFFFFF);
    port_write(port, AHCI_PORT_CI, 1);

    for (uint64_t i = 0; i < AHCI_WAIT_LIMIT; i++) {
        uint32_t interrupt_status =
            port_read(port, AHCI_PORT_IS);

        if (interrupt_status & AHCI_IS_TFES) {
            port_write(port, AHCI_PORT_IS, 0xFFFFFFFF);
            return 0;
        }

        if (!(port_read(port, AHCI_PORT_CI) & 1U)) {
            uint32_t task_file =
                port_read(port, AHCI_PORT_TFD);

            port_write(port, AHCI_PORT_IS, 0xFFFFFFFF);

            if (task_file & AHCI_TFD_ERR) {
                return 0;
            }

            return 1;
        }

        __asm__ volatile ("pause");
    }

    return 0;
}

static uint32_t read_big_endian_u32(const uint8_t *data) {
    return
        ((uint32_t)data[0] << 24) |
        ((uint32_t)data[1] << 16) |
        ((uint32_t)data[2] << 8) |
        (uint32_t)data[3];
}

int ahci_init(void) {
    if (controller_info.initialized) {
        return 1;
    }

    zero_memory(&controller_info, sizeof(controller_info));

    for (uint32_t i = 0; i < AHCI_MAX_PORTS; i++) {
        zero_memory(&port_dma[i], sizeof(port_dma[i]));

        port_dma[i].command_list_physical = PMM_FAILURE;
        port_dma[i].fis_physical = PMM_FAILURE;
        port_dma[i].command_table_physical = PMM_FAILURE;
        port_dma[i].data_physical = PMM_FAILURE;
    }

    struct pci_device controller;

    if (!pci_find_class(0x01, 0x06, 0x01, &controller)) {
        return 0;
    }

    struct pci_bar *bar =
        &controller.bar_info[AHCI_BAR_INDEX];

    if (
        (bar->type != PCI_BAR_MEMORY_32 &&
         bar->type != PCI_BAR_MEMORY_64) ||
        bar->is_upper_half ||
        bar->address == 0
    ) {
        return 0;
    }

    uint64_t abar_physical = bar->address;

    if (abar_physical & (AHCI_PAGE_SIZE - 1)) {
        return 0;
    }

    if (!pci_enable_memory_space(&controller)) {
        return 0;
    }

    if (!vmm_map_range(
        AHCI_ABAR_VIRTUAL_BASE,
        abar_physical,
        2,
        VMM_PAGE_MMIO
    )) {
        return 0;
    }

    abar = (volatile uint32_t *)AHCI_ABAR_VIRTUAL_BASE;

    uint32_t capabilities = mmio_read(AHCI_REG_CAP);
    uint32_t implemented_ports = mmio_read(AHCI_REG_PI);

    controller_info.abar_physical = abar_physical;
    controller_info.capabilities = capabilities;
    controller_info.global_control = mmio_read(AHCI_REG_GHC);
    controller_info.implemented_ports = implemented_ports;
    controller_info.version = mmio_read(AHCI_REG_VS);

    controller_info.hardware_port_count =
        (uint8_t)((capabilities & 0x1F) + 1);

    controller_info.supports_64bit =
        (capabilities & (1U << 31)) != 0;

    controller_info.pci_bus = controller.bus;
    controller_info.pci_device = controller.device;
    controller_info.pci_function = controller.function;

    for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
        struct ahci_port_info *info =
            &controller_info.ports[port];

        info->port_number = port;

        if (!(implemented_ports & (1U << port))) {
            continue;
        }

        info->implemented = 1;
        controller_info.implemented_port_count++;

        info->sata_status =
            port_read(port, AHCI_PORT_SSTS);

        info->signature =
            port_read(port, AHCI_PORT_SIG);

        uint32_t det = info->sata_status & 0x0F;
        uint32_t ipm = (info->sata_status >> 8) & 0x0F;

        if (det != 3 || ipm != 1) {
            info->device_type = AHCI_DEVICE_NONE;
            continue;
        }

        info->device_present = 1;
        controller_info.detected_device_count++;

        switch (info->signature) {
            case AHCI_SIGNATURE_SATA:
                info->device_type = AHCI_DEVICE_SATA;
                break;

            case AHCI_SIGNATURE_ATAPI:
                info->device_type = AHCI_DEVICE_ATAPI;
                break;

            case AHCI_SIGNATURE_SEMB:
                info->device_type = AHCI_DEVICE_SEMB;
                break;

            case AHCI_SIGNATURE_PM:
                info->device_type = AHCI_DEVICE_PM;
                break;

            default:
                info->device_type = AHCI_DEVICE_UNKNOWN;
                break;
        }

        if (info->device_type == AHCI_DEVICE_ATAPI) {
            if (!prepare_port(port)) {
                info->command_engine_ready = 0;
            }
        }
    }

    int prepared_atapi_device = 0;

    for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
        if (port_dma[port].prepared) {
            prepared_atapi_device = 1;
            break;
        }
    }

    if (prepared_atapi_device) {
        if (!pci_enable_bus_master(&controller)) {
            return 0;
        }
    }

    controller_info.initialized = 1;

    return 1;
}

int ahci_probe_device(uint8_t port) {
    if (!controller_info.initialized ||
        port >= AHCI_MAX_PORTS) {
        return 0;
    }

    struct ahci_port_info *info =
        &controller_info.ports[port];

    if (!info->device_present ||
        info->device_type != AHCI_DEVICE_ATAPI ||
        !info->command_engine_ready) {
        return 0;
    }

    uint8_t packet[12] = {0};

    packet[0] = ATAPI_READ_CAPACITY;

    if (!issue_packet(port, packet, 8)) {
        return 0;
    }

    struct ahci_port_dma *dma = &port_dma[port];

    uint32_t last_lba =
        read_big_endian_u32(dma->data);

    uint32_t block_size =
        read_big_endian_u32(dma->data + 4);

    if (block_size == 0 ||
        block_size > ATAPI_BLOCK_SIZE_MAX) {
        return 0;
    }

    info->block_size = block_size;
    info->block_count = (uint64_t)last_lba + 1;

    return 1;
}

int ahci_read_blocks(
    uint8_t port,
    uint64_t lba,
    uint32_t block_count,
    void *buffer
) {
    if (!controller_info.initialized ||
        port >= AHCI_MAX_PORTS ||
        buffer == NULL ||
        block_count == 0) {
        return 0;
    }

    struct ahci_port_info *info =
        &controller_info.ports[port];

    if (
        info->device_type != AHCI_DEVICE_ATAPI ||
        !info->command_engine_ready ||
        info->block_size == 0 ||
        info->block_count == 0
    ) {
        return 0;
    }

    if (lba >= info->block_count ||
        block_count > info->block_count - lba) {
        return 0;
    }

    uint64_t transfer_size =
        (uint64_t)info->block_size * block_count;

    if (
        transfer_size == 0 ||
        transfer_size > AHCI_PAGE_SIZE ||
        lba > 0xFFFFFFFFULL ||
        block_count > 0xFFFFFFFFU
    ) {
        return 0;
    }

    uint8_t packet[12] = {0};

    packet[0] = ATAPI_READ_12;

    packet[2] = (uint8_t)(lba >> 24);
    packet[3] = (uint8_t)(lba >> 16);
    packet[4] = (uint8_t)(lba >> 8);
    packet[5] = (uint8_t)lba;

    packet[6] = (uint8_t)(block_count >> 24);
    packet[7] = (uint8_t)(block_count >> 16);
    packet[8] = (uint8_t)(block_count >> 8);
    packet[9] = (uint8_t)block_count;

    if (!issue_packet(
        port,
        packet,
        (uint32_t)transfer_size
    )) {
        return 0;
    }

    copy_memory(
        buffer,
        port_dma[port].data,
        transfer_size
    );

    return 1;
}

uint64_t ahci_get_block_count(uint8_t port) {
    if (port >= AHCI_MAX_PORTS) {
        return 0;
    }

    return controller_info.ports[port].block_count;
}

uint32_t ahci_get_block_size(uint8_t port) {
    if (port >= AHCI_MAX_PORTS) {
        return 0;
    }

    return controller_info.ports[port].block_size;
}

int ahci_is_initialized(void) {
    return controller_info.initialized;
}

const struct ahci_info *ahci_get_info(void) {
    return &controller_info;
}

const char *ahci_device_type_name(uint8_t type) {
    switch (type) {
        case AHCI_DEVICE_NONE:
            return "None";

        case AHCI_DEVICE_SATA:
            return "SATA";

        case AHCI_DEVICE_ATAPI:
            return "ATAPI";

        case AHCI_DEVICE_SEMB:
            return "SEMB";

        case AHCI_DEVICE_PM:
            return "Port Multiplier";

        default:
            return "Unknown";
    }
}