#include <pci.h>
#include <stdint.h>
#include <stddef.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF 0x09
#define PCI_SUBCLASS 0x0A
#define PCI_CLASS_CODE 0x0B
#define PCI_HEADER_TYPE 0x0E
#define PCI_BAR0 0x10
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN 0x3D

#define PCI_HEADER_TYPE_NORMAL 0x00
#define PCI_HEADER_TYPE_BRIDGE 0x01
#define PCI_HEADER_MULTIFUNCTION 0x80

static struct pci_device pci_devices[PCI_MAX_DEVICES];

static size_t pci_device_count = 0;
static int pci_initialized = 0;

static void pci_copy_device(
    struct pci_device *destination,
    const struct pci_device *source
) {
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;

    for (size_t i = 0; i < sizeof(struct pci_device); i++) {
        dst[i] = src[i];
    }
}

static inline void pci_outl(uint16_t port, uint32_t value) {
    __asm__ volatile (
        "outl %0, %1"
        :
        : "a"(value), "Nd"(port)
        : "memory"
    );
}

static inline uint32_t pci_inl(uint16_t port) {
    uint32_t value;

    __asm__ volatile (
        "inl %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );

    return value;
}

static inline uint16_t pci_inw(uint16_t port) {
    uint16_t value;

    __asm__ volatile (
        "inw %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );

    return value;
}

static inline uint8_t pci_inb(uint16_t port) {
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );

    return value;
}

static inline void pci_outw(uint16_t port, uint16_t value) {
    __asm__ volatile (
        "outw %0, %1"
        :
        : "a"(value), "Nd"(port)
        : "memory"
    );
}

static uint32_t pci_config_address(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
) {
    return 0x80000000 |
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC);
}

uint32_t pci_config_read_dword(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
) {
    pci_outl(
        PCI_CONFIG_ADDRESS,
        pci_config_address(bus, device, function, offset)
    );

    return pci_inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read_word(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
) {
    pci_outl(
        PCI_CONFIG_ADDRESS,
        pci_config_address(bus, device, function, offset)
    );

    return pci_inw(PCI_CONFIG_DATA + (offset & 2));
}

static uint8_t pci_config_read_byte(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
) {
    pci_outl(
        PCI_CONFIG_ADDRESS,
        pci_config_address(bus, device, function, offset)
    );

    return pci_inb(PCI_CONFIG_DATA + (offset & 3));
}

void pci_config_write_word(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint16_t value
) {
    pci_outl(
        PCI_CONFIG_ADDRESS,
        pci_config_address(bus, device, function, offset)
    );

    pci_outw(PCI_CONFIG_DATA + (offset & 2), value);
}

void pci_config_write_dword(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t value
) {
    pci_outl(
        PCI_CONFIG_ADDRESS,
        pci_config_address(bus, device, function, offset)
    );

    pci_outl(PCI_CONFIG_DATA, value);
}

static void pci_decode_bars(struct pci_device *device) {
    uint8_t header_type = device->header_type & 0x7F;

    if (header_type == PCI_HEADER_TYPE_NORMAL) {
        device->bar_count = 6;
    } else if (header_type == PCI_HEADER_TYPE_BRIDGE) {
        device->bar_count = 2;
    } else {
        device->bar_count = 0;
        return;
    }

    for (uint8_t i = 0; i < device->bar_count; i++) {
        uint32_t raw = pci_config_read_dword(
            device->bus,
            device->device,
            device->function,
            PCI_BAR0 + (i * 4)
        );

        device->bars[i] = raw;

        device->bar_info[i].type = PCI_BAR_NONE;
        device->bar_info[i].is_64_bit = 0;
        device->bar_info[i].prefetchable = 0;
        device->bar_info[i].is_upper_half = 0;
        device->bar_info[i].address = 0;

        if (raw == 0) {
            continue;
        }

        if (raw & 1) {
            device->bar_info[i].type = PCI_BAR_IO;
            device->bar_info[i].address = raw & ~0x3ULL;
            continue;
        }

        uint8_t memory_type = (raw >> 1) & 3;

        device->bar_info[i].prefetchable = (raw >> 3) & 1;

        if (memory_type == 0) {
            device->bar_info[i].type = PCI_BAR_MEMORY_32;
            device->bar_info[i].address = raw & ~0xFULL;
        } else if (memory_type == 1) {
            device->bar_info[i].type = PCI_BAR_MEMORY_BELOW_1MB;
            device->bar_info[i].address = raw & ~0xFULL;
        } else if (memory_type == 2) {
            uint64_t address = raw & ~0xFULL;

            device->bar_info[i].type = PCI_BAR_MEMORY_64;
            device->bar_info[i].is_64_bit = 1;

            if (i + 1 < device->bar_count) {
                uint32_t upper = pci_config_read_dword(
                    device->bus,
                    device->device,
                    device->function,
                    PCI_BAR0 + ((i + 1) * 4)
                );

                device->bars[i + 1] = upper;
                address |= (uint64_t)upper << 32;

                device->bar_info[i + 1].type = PCI_BAR_CONTINUATION;
                device->bar_info[i + 1].is_upper_half = 1;

                device->bar_info[i].address = address;
                i++;
            } else {
                device->bar_info[i].address = address;
            }
        } else {
            device->bar_info[i].type = PCI_BAR_RESERVED;
        }
    }
}

static int pci_read_device(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    struct pci_device *result
) {
    uint16_t vendor_id = pci_config_read_word(
        bus, device, function, PCI_VENDOR_ID
    );

    if (vendor_id == 0xFFFF) {
        return 0;
    }

    result->bus = bus;
    result->device = device;
    result->function = function;

    result->vendor_id = vendor_id;

    result->device_id = pci_config_read_word(
        bus, device, function, PCI_DEVICE_ID
    );

    result->command = pci_config_read_word(
        bus, device, function, PCI_COMMAND
    );

    result->status = pci_config_read_word(
        bus, device, function, PCI_STATUS
    );

    result->revision_id = pci_config_read_byte(
        bus, device, function, PCI_REVISION_ID
    );

    result->programming_interface = pci_config_read_byte(
        bus, device, function, PCI_PROG_IF
    );

    result->subclass = pci_config_read_byte(
        bus, device, function, PCI_SUBCLASS
    );

    result->class_code = pci_config_read_byte(
        bus, device, function, PCI_CLASS_CODE
    );

    result->header_type = pci_config_read_byte(
        bus, device, function, PCI_HEADER_TYPE
    );

    result->interrupt_line = pci_config_read_byte(
        bus, device, function, PCI_INTERRUPT_LINE
    );

    result->interrupt_pin = pci_config_read_byte(
        bus, device, function, PCI_INTERRUPT_PIN
    );

    pci_decode_bars(result);

    return 1;
}

static void pci_scan_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        uint16_t vendor_id = pci_config_read_word(
            bus, device, 0, PCI_VENDOR_ID
        );

        if (vendor_id == 0xFFFF) {
            continue;
        }

        uint8_t header_type = pci_config_read_byte(
            bus, device, 0, PCI_HEADER_TYPE
        );

        uint8_t function_count =
            (header_type & PCI_HEADER_MULTIFUNCTION) ? 8 : 1;

        for (uint8_t function = 0; function < function_count; function++) {
            if (pci_device_count >= PCI_MAX_DEVICES) {
                return;
            }

            struct pci_device discovered;

            if (!pci_read_device(bus, device, function, &discovered)) {
                continue;
            }

            pci_copy_device(
                &pci_devices[pci_device_count],
                &discovered
            );

            pci_device_count++;
        }
    }
}

int pci_init(void) {
    pci_device_count = 0;
    pci_initialized = 0;

    for (uint16_t bus = 0; bus < 256; bus++) {
        pci_scan_bus((uint8_t)bus);

        if (pci_device_count >= PCI_MAX_DEVICES) {
            break;
        }
    }

    pci_initialized = 1;

    return 1;
}

int pci_is_initialized(void) {
    return pci_initialized;
}

size_t pci_get_device_count(void) {
    return pci_initialized ? pci_device_count : 0;
}

int pci_get_device(size_t index, struct pci_device *device) {
    if (!pci_initialized || device == NULL ||
        index >= pci_device_count) {
        return 0;
    }

    pci_copy_device(device, &pci_devices[index]);

    return 1;
}

int pci_find_class(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t programming_interface,
    struct pci_device *result
) {
    if (!pci_initialized || result == NULL) {
        return 0;
    }

    for (size_t i = 0; i < pci_device_count; i++) {
        const struct pci_device *device = &pci_devices[i];

        if (device->class_code != class_code ||
            device->subclass != subclass) {
            continue;
        }

        if (programming_interface != 0 &&
            programming_interface != 0xFF &&
            device->programming_interface != programming_interface) {
            continue;
        }

        pci_copy_device(result, device);

        return 1;
    }

    return 0;
}

uint16_t pci_read_command(const struct pci_device *device) {
    if (device == NULL) {
        return 0;
    }

    return pci_config_read_word(
        device->bus,
        device->device,
        device->function,
        PCI_COMMAND
    );
}

uint16_t pci_read_status(const struct pci_device *device) {
    if (device == NULL) {
        return 0;
    }

    return pci_config_read_word(
        device->bus,
        device->device,
        device->function,
        PCI_STATUS
    );
}

int pci_refresh_device(struct pci_device *device) {
    if (device == NULL) {
        return 0;
    }

    device->command = pci_read_command(device);
    device->status = pci_read_status(device);

    return 1;
}

int pci_enable_io_space(struct pci_device *device) {
    if (device == NULL) {
        return 0;
    }

    uint16_t command = pci_read_command(device);
    command |= PCI_COMMAND_IO_SPACE;

    pci_config_write_word(
        device->bus,
        device->device,
        device->function,
        PCI_COMMAND,
        command
    );

    pci_refresh_device(device);

    return (device->command & PCI_COMMAND_IO_SPACE) != 0;
}

int pci_enable_memory_space(struct pci_device *device) {
    if (device == NULL) {
        return 0;
    }

    uint16_t command = pci_read_command(device);
    command |= PCI_COMMAND_MEMORY_SPACE;

    pci_config_write_word(
        device->bus,
        device->device,
        device->function,
        PCI_COMMAND,
        command
    );

    pci_refresh_device(device);

    return (device->command & PCI_COMMAND_MEMORY_SPACE) != 0;
}

int pci_enable_bus_master(struct pci_device *device) {
    if (device == NULL) {
        return 0;
    }

    uint16_t command = pci_read_command(device);
    command |= PCI_COMMAND_BUS_MASTER;

    pci_config_write_word(
        device->bus,
        device->device,
        device->function,
        PCI_COMMAND,
        command
    );

    pci_refresh_device(device);

    return (device->command & PCI_COMMAND_BUS_MASTER) != 0;
}

int pci_enable_device(
    struct pci_device *device,
    int enable_io,
    int enable_memory,
    int enable_bus_master
) {
    if (device == NULL) {
        return 0;
    }

    uint16_t command = pci_read_command(device);

    if (enable_io) {
        command |= PCI_COMMAND_IO_SPACE;
    }

    if (enable_memory) {
        command |= PCI_COMMAND_MEMORY_SPACE;
    }

    if (enable_bus_master) {
        command |= PCI_COMMAND_BUS_MASTER;
    }

    pci_config_write_word(
        device->bus,
        device->device,
        device->function,
        PCI_COMMAND,
        command
    );

    pci_refresh_device(device);

    if (enable_io &&
        !(device->command & PCI_COMMAND_IO_SPACE)) {
        return 0;
    }

    if (enable_memory &&
        !(device->command & PCI_COMMAND_MEMORY_SPACE)) {
        return 0;
    }

    if (enable_bus_master &&
        !(device->command & PCI_COMMAND_BUS_MASTER)) {
        return 0;
    }

    return 1;
}

const char *pci_bar_type_name(uint8_t type) {
    switch (type) {
        case PCI_BAR_NONE:
            return "Unused";

        case PCI_BAR_IO:
            return "I/O";

        case PCI_BAR_MEMORY_32:
            return "32-bit Memory";

        case PCI_BAR_MEMORY_64:
            return "64-bit Memory";

        case PCI_BAR_MEMORY_BELOW_1MB:
            return "Below 1 MB Memory";

        case PCI_BAR_RESERVED:
            return "Reserved";

        case PCI_BAR_CONTINUATION:
            return "64-bit Upper Half";

        default:
            return "Unknown";
    }
}