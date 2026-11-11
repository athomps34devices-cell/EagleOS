#ifndef EAGLEOS_PCI_H
#define EAGLEOS_PCI_H

#include <stdint.h>
#include <stddef.h>

#define PCI_MAX_DEVICES 256
#define PCI_MAX_BARS 6

#define PCI_BAR_NONE 0
#define PCI_BAR_IO 1
#define PCI_BAR_MEMORY_32 2
#define PCI_BAR_MEMORY_64 3
#define PCI_BAR_MEMORY_BELOW_1MB 4
#define PCI_BAR_RESERVED 5
#define PCI_BAR_CONTINUATION 6

#define PCI_COMMAND_IO_SPACE 0x0001
#define PCI_COMMAND_MEMORY_SPACE 0x0002
#define PCI_COMMAND_BUS_MASTER 0x0004

struct pci_bar {
    uint8_t type;
    uint8_t is_64_bit;
    uint8_t prefetchable;
    uint8_t is_upper_half;
    uint64_t address;
};

struct pci_device {
    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint16_t vendor_id;
    uint16_t device_id;

    uint16_t command;
    uint16_t status;

    uint8_t class_code;
    uint8_t subclass;
    uint8_t programming_interface;
    uint8_t revision_id;

    uint8_t header_type;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;

    uint8_t bar_count;

    uint32_t bars[PCI_MAX_BARS];
    struct pci_bar bar_info[PCI_MAX_BARS];
};

int pci_init(void);
int pci_is_initialized(void);

size_t pci_get_device_count(void);
int pci_get_device(size_t index, struct pci_device *device);

int pci_find_class(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t programming_interface,
    struct pci_device *result
);

uint16_t pci_config_read_word(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
);

uint32_t pci_config_read_dword(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
);

void pci_config_write_word(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint16_t value
);

void pci_config_write_dword(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t value
);

uint16_t pci_read_command(const struct pci_device *device);
uint16_t pci_read_status(const struct pci_device *device);

int pci_enable_io_space(struct pci_device *device);
int pci_enable_memory_space(struct pci_device *device);
int pci_enable_bus_master(struct pci_device *device);

int pci_enable_device(
    struct pci_device *device,
    int enable_io,
    int enable_memory,
    int enable_bus_master
);

int pci_refresh_device(struct pci_device *device);

const char *pci_bar_type_name(uint8_t type);

#endif