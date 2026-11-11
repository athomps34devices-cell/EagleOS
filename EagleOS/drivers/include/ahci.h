#ifndef EAGLEOS_AHCI_H
#define EAGLEOS_AHCI_H

#include <stdint.h>

#define AHCI_MAX_PORTS 32

#define AHCI_DEVICE_NONE 0
#define AHCI_DEVICE_SATA 1
#define AHCI_DEVICE_ATAPI 2
#define AHCI_DEVICE_SEMB 3
#define AHCI_DEVICE_PM 4
#define AHCI_DEVICE_UNKNOWN 5

struct ahci_port_info {
    uint8_t port_number;
    uint8_t implemented;
    uint8_t device_present;
    uint8_t device_type;

    uint32_t sata_status;
    uint32_t signature;

    uint8_t command_engine_ready;
    uint32_t block_size;
    uint64_t block_count;
};

struct ahci_info {
    uint8_t initialized;
    uint8_t supports_64bit;
    uint8_t hardware_port_count;
    uint8_t implemented_port_count;
    uint8_t detected_device_count;

    uint8_t pci_bus;
    uint8_t pci_device;
    uint8_t pci_function;

    uint64_t abar_physical;

    uint32_t capabilities;
    uint32_t global_control;
    uint32_t implemented_ports;
    uint32_t version;

    struct ahci_port_info ports[AHCI_MAX_PORTS];
};

int ahci_init(void);
int ahci_is_initialized(void);

int ahci_probe_device(uint8_t port);

int ahci_read_blocks(
    uint8_t port,
    uint64_t lba,
    uint32_t block_count,
    void *buffer
);

uint64_t ahci_get_block_count(uint8_t port);
uint32_t ahci_get_block_size(uint8_t port);

const struct ahci_info *ahci_get_info(void);

const char *ahci_device_type_name(uint8_t type);

#endif