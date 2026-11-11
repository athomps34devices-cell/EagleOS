#ifndef EAGLEOS_BLOCKDEV_H
#define EAGLEOS_BLOCKDEV_H

#include <stdint.h>

#define BLOCKDEV_MAX_DEVICES 32

#define BLOCKDEV_TYPE_NONE  0
#define BLOCKDEV_TYPE_AHCI  1
#define BLOCKDEV_TYPE_NVME  2
#define BLOCKDEV_TYPE_RAM   3

struct block_device {
    uint8_t id;
    uint8_t type;
    uint8_t initialized;
    uint8_t read_only;

    uint8_t controller_port;
    uint8_t reserved[3];

    uint32_t block_size;
    uint64_t block_count;
};

int blockdev_init(void);
int blockdev_is_initialized(void);

uint32_t blockdev_get_device_count(void);

int blockdev_get_device(
    uint32_t id,
    struct block_device *device
);

int blockdev_read(
    uint32_t device_id,
    uint64_t lba,
    uint32_t block_count,
    void *buffer
);

const char *blockdev_type_name(uint8_t type);

#endif