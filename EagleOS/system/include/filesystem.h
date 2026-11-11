#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stddef.h>

#define FILESYSTEM_SUCCESS 1
#define FILESYSTEM_FAILURE 0

#define FILESYSTEM_MAX_TYPES 8
#define FILESYSTEM_MAX_MOUNTS 8
#define FILESYSTEM_MAX_NAME_LENGTH 128
#define FILESYSTEM_MAX_PATH_LENGTH 256
#define FILESYSTEM_MAX_DIRECTORY_ENTRIES 64

#define FILESYSTEM_FLAG_DIRECTORY 0x02
#define FILESYSTEM_FLAG_HIDDEN    0x01

struct filesystem_entry {
    char name[FILESYSTEM_MAX_NAME_LENGTH];
    uint32_t size;
    uint8_t flags;
    uint8_t is_directory;
};

struct filesystem_type {
    const char *name;

    int (*probe)(uint32_t device_id);
    int (*mount)(uint32_t device_id);

    int (*list_directory)(
        const char *path,
        struct filesystem_entry *entries,
        size_t capacity,
        size_t *entry_count
    );

    int (*find)(
        const char *path,
        struct filesystem_entry *entry
    );

    int (*read_file)(
        const char *path,
        void *buffer,
        size_t capacity,
        size_t *bytes_read
    );
};

struct filesystem_mount {
    char mount_path[FILESYSTEM_MAX_PATH_LENGTH];
    uint32_t device_id;
    uint8_t initialized;
    const struct filesystem_type *type;
};

int filesystem_init(void);

int filesystem_register(
    const struct filesystem_type *type
);

int filesystem_mount_device(
    uint32_t device_id,
    const char *mount_path
);

int filesystem_is_initialized(void);

uint32_t filesystem_get_type_count(void);
uint32_t filesystem_get_mount_count(void);

const struct filesystem_mount *filesystem_get_mount(
    uint32_t index
);

const struct filesystem_type *filesystem_get_type(
    uint32_t index
);

int filesystem_list_directory(
    const char *path,
    struct filesystem_entry *entries,
    size_t capacity,
    size_t *entry_count
);

int filesystem_find(
    const char *path,
    struct filesystem_entry *entry
);

int filesystem_read_file(
    const char *path,
    void *buffer,
    size_t capacity,
    size_t *bytes_read
);

const struct filesystem_type *filesystem_get_iso9660_type(void);

#endif