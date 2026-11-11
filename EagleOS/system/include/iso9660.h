#ifndef ISO9660_H
#define ISO9660_H

#include <stdint.h>
#include <stddef.h>

#define ISO9660_BLOCK_SIZE 2048
#define ISO9660_MAX_NAME_LENGTH 128

#define ISO9660_FLAG_HIDDEN       0x01
#define ISO9660_FLAG_DIRECTORY    0x02
#define ISO9660_FLAG_ASSOCIATED   0x04
#define ISO9660_FLAG_EXTENDED     0x08
#define ISO9660_FLAG_PERMISSIONS  0x10
#define ISO9660_FLAG_MULTI_EXTENT 0x80

#define ISO9660_SUCCESS 1
#define ISO9660_FAILURE 0

struct iso9660_volume {
    uint32_t device_id;
    uint32_t volume_blocks;
    uint32_t logical_block_size;
    uint32_t root_directory_lba;
    uint32_t root_directory_size;
    uint8_t mounted;
};

struct iso9660_directory_entry {
    char name[ISO9660_MAX_NAME_LENGTH];
    uint32_t extent_lba;
    uint32_t size;
    uint8_t flags;
    uint8_t is_directory;
};

int iso9660_mount(uint32_t device_id);

int iso9660_is_mounted(void);

const struct iso9660_volume *iso9660_get_volume(void);

int iso9660_list_directory(
    const char *path,
    struct iso9660_directory_entry *entries,
    size_t capacity,
    size_t *entry_count
);

int iso9660_find(
    const char *path,
    struct iso9660_directory_entry *entry
);

int iso9660_read_file(
    const char *path,
    void *buffer,
    size_t capacity,
    size_t *bytes_read
);

#endif