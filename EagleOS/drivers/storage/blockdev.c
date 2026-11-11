#include <stdint.h>
#include <stddef.h>

#include <blockdev.h>
#include <ahci.h>

#define BLOCKDEV_MAX_TRANSFER_SIZE 4096U

static struct block_device devices[BLOCKDEV_MAX_DEVICES];

static uint32_t device_count;
static uint8_t initialized;

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
    volatile uint8_t *dest = (volatile uint8_t *)destination;
    const volatile uint8_t *src = (const volatile uint8_t *)source;

    for (uint64_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}

int blockdev_init(void) {
    if (initialized) {
        return 1;
    }

    zero_memory(devices, sizeof(devices));

    device_count = 0;

    if (!ahci_is_initialized()) {
        return 0;
    }

    const struct ahci_info *controller = ahci_get_info();

    if (controller == NULL) {
        return 0;
    }

    for (uint32_t port = 0; port < AHCI_MAX_PORTS; port++) {
        const struct ahci_port_info *port_info =
            &controller->ports[port];

        if (!port_info->device_present) {
            continue;
        }

        if (port_info->device_type != AHCI_DEVICE_ATAPI) {
            continue;
        }

        if (!ahci_probe_device((uint8_t)port)) {
            continue;
        }

        if (device_count >= BLOCKDEV_MAX_DEVICES) {
            break;
        }

        struct block_device *device =
            &devices[device_count];

        device->id = (uint8_t)device_count;
        device->type = BLOCKDEV_TYPE_AHCI;
        device->initialized = 1;
        device->read_only = 1;

        device->controller_port = (uint8_t)port;

        device->block_size = ahci_get_block_size((uint8_t)port);
        device->block_count = ahci_get_block_count((uint8_t)port);

        if (device->block_size == 0 ||
            device->block_count == 0) {
            zero_memory(device, sizeof(*device));
            continue;
        }

        device_count++;
    }

    initialized = 1;

    return 1;
}

int blockdev_is_initialized(void) {
    return initialized;
}

uint32_t blockdev_get_device_count(void) {
    return device_count;
}

int blockdev_get_device(
    uint32_t id,
    struct block_device *device
) {
    if (!initialized || device == NULL) {
        return 0;
    }

    if (id >= device_count) {
        return 0;
    }

    copy_memory(
        device,
        &devices[id],
        sizeof(struct block_device)
    );

    return 1;
}

int blockdev_read(
    uint32_t device_id,
    uint64_t lba,
    uint32_t block_count,
    void *buffer
) {
    if (!initialized ||
        buffer == NULL ||
        block_count == 0 ||
        device_id >= device_count) {
        return 0;
    }

    const struct block_device *device = &devices[device_id];

    if (!device->initialized ||
        device->block_size == 0 ||
        device->block_count == 0) {
        return 0;
    }

    if (lba >= device->block_count ||
        block_count > device->block_count - lba) {
        return 0;
    }

    uint32_t maximum_blocks =
        BLOCKDEV_MAX_TRANSFER_SIZE / device->block_size;

    if (maximum_blocks == 0) {
        return 0;
    }

    uint8_t *output = (uint8_t *)buffer;

    uint32_t remaining = block_count;

    while (remaining > 0) {
        uint32_t current_count = remaining;

        if (current_count > maximum_blocks) {
            current_count = maximum_blocks;
        }

        uint64_t transfer_size =
            (uint64_t)current_count * device->block_size;

        if (!ahci_read_blocks(
            device->controller_port,
            lba,
            current_count,
            output
        )) {
            return 0;
        }

        output += transfer_size;
        lba += current_count;
        remaining -= current_count;
    }

    return 1;
}

const char *blockdev_type_name(uint8_t type) {
    switch (type) {
        case BLOCKDEV_TYPE_AHCI:
            return "AHCI";

        case BLOCKDEV_TYPE_NVME:
            return "NVMe";

        case BLOCKDEV_TYPE_RAM:
            return "RAM";

        default:
            return "Unknown";
    }
}