#include <filesystem.h>
#include <iso9660.h>
#include <blockdev.h>

static struct filesystem_type registered_types[FILESYSTEM_MAX_TYPES];
static struct filesystem_mount mounted_filesystems[FILESYSTEM_MAX_MOUNTS];

static struct iso9660_directory_entry iso_entries[
    FILESYSTEM_MAX_DIRECTORY_ENTRIES
];

static uint32_t registered_type_count = 0;
static uint32_t mounted_filesystem_count = 0;

static int initialized = 0;

static void copy_string(
    char *destination,
    const char *source,
    size_t capacity
) {
    if (destination == NULL || capacity == 0) {
        return;
    }

    size_t i = 0;

    if (source != NULL) {
        while (source[i] && i + 1 < capacity) {
            destination[i] = source[i];
            i++;
        }
    }

    destination[i] = '\0';
}

static size_t string_length(const char *string) {
    if (string == NULL) {
        return 0;
    }

    size_t length = 0;

    while (string[length]) {
        length++;
    }

    return length;
}

static int strings_equal(
    const char *first,
    const char *second
) {
    if (first == NULL || second == NULL) {
        return 0;
    }

    size_t i = 0;

    while (first[i] && second[i]) {
        if (first[i] != second[i]) {
            return 0;
        }

        i++;
    }

    return first[i] == second[i];
}

static int path_matches_mount(
    const char *path,
    const char *mount_path
) {
    if (path == NULL || mount_path == NULL) {
        return 0;
    }

    size_t mount_length = string_length(mount_path);

    if (mount_length == 0) {
        return 0;
    }

    for (size_t i = 0; i < mount_length; i++) {
        if (path[i] != mount_path[i]) {
            return 0;
        }
    }

    if (mount_length == 1 && mount_path[0] == '/') {
        return 1;
    }

    return path[mount_length] == '\0' ||
           path[mount_length] == '/';
}

static const struct filesystem_mount *resolve_mount(
    const char *path,
    const char **relative_path
) {
    if (path == NULL || relative_path == NULL) {
        return NULL;
    }

    const struct filesystem_mount *best_mount = NULL;
    size_t best_length = 0;

    for (uint32_t i = 0; i < mounted_filesystem_count; i++) {
        const struct filesystem_mount *mount =
            &mounted_filesystems[i];

        if (!mount->initialized) {
            continue;
        }

        if (!path_matches_mount(path, mount->mount_path)) {
            continue;
        }

        size_t length = string_length(mount->mount_path);

        if (best_mount == NULL || length > best_length) {
            best_mount = mount;
            best_length = length;
        }
    }

    if (best_mount == NULL) {
        return NULL;
    }

    if (best_length == 1 && best_mount->mount_path[0] == '/') {
        *relative_path = path;
    } else {
        *relative_path = path + best_length;

        if (**relative_path == '\0') {
            *relative_path = "/";
        }
    }

    return best_mount;
}

/* ISO 9660 driver adapter */

static int iso9660_probe(uint32_t device_id) {
    struct block_device device;

    if (!blockdev_get_device(device_id, &device)) {
        return 0;
    }

    if (!device.initialized ||
        device.block_size != ISO9660_BLOCK_SIZE ||
        device.block_count <= 16) {
        return 0;
    }

    uint8_t descriptor[ISO9660_BLOCK_SIZE];

    if (!blockdev_read(device_id, 16, 1, descriptor)) {
        return 0;
    }

    return descriptor[0] == 1 &&
           descriptor[1] == 'C' &&
           descriptor[2] == 'D' &&
           descriptor[3] == '0' &&
           descriptor[4] == '0' &&
           descriptor[5] == '1' &&
           descriptor[6] == 1;
}

static int iso9660_mount_device(uint32_t device_id) {
    if (iso9660_is_mounted()) {
        const struct iso9660_volume *volume =
            iso9660_get_volume();

        if (volume == NULL || volume->device_id != device_id) {
            return 0;
        }

        return 1;
    }

    return iso9660_mount(device_id);
}

static int iso9660_list(
    const char *path,
    struct filesystem_entry *entries,
    size_t capacity,
    size_t *entry_count
) {
    if (entry_count == NULL) {
        return 0;
    }

    *entry_count = 0;

    size_t requested = capacity;

    if (requested > FILESYSTEM_MAX_DIRECTORY_ENTRIES) {
        requested = FILESYSTEM_MAX_DIRECTORY_ENTRIES;
    }

    size_t total = 0;

    if (!iso9660_list_directory(
        path,
        iso_entries,
        requested,
        &total
    )) {
        return 0;
    }

    size_t converted = total;

    if (converted > requested) {
        converted = requested;
    }

    for (size_t i = 0; i < converted; i++) {
        copy_string(
            entries[i].name,
            iso_entries[i].name,
            sizeof(entries[i].name)
        );

        entries[i].size = iso_entries[i].size;
        entries[i].flags = iso_entries[i].flags;
        entries[i].is_directory =
            iso_entries[i].is_directory;
    }

    *entry_count = total;

    return 1;
}

static int iso9660_find_entry(
    const char *path,
    struct filesystem_entry *entry
) {
    if (entry == NULL) {
        return 0;
    }

    struct iso9660_directory_entry iso_entry;

    if (!iso9660_find(path, &iso_entry)) {
        return 0;
    }

    copy_string(
        entry->name,
        iso_entry.name,
        sizeof(entry->name)
    );

    entry->size = iso_entry.size;
    entry->flags = iso_entry.flags;
    entry->is_directory = iso_entry.is_directory;

    return 1;
}

static int iso9660_read(
    const char *path,
    void *buffer,
    size_t capacity,
    size_t *bytes_read
) {
    return iso9660_read_file(
        path,
        buffer,
        capacity,
        bytes_read
    );
}

static const struct filesystem_type iso9660_type = {
    .name = "ISO 9660",
    .probe = iso9660_probe,
    .mount = iso9660_mount_device,
    .list_directory = iso9660_list,
    .find = iso9660_find_entry,
    .read_file = iso9660_read
};

const struct filesystem_type *filesystem_get_iso9660_type(void) {
    return &iso9660_type;
}

int filesystem_init(void) {
    registered_type_count = 0;
    mounted_filesystem_count = 0;

    for (uint32_t i = 0; i < FILESYSTEM_MAX_TYPES; i++) {
        registered_types[i].name = NULL;
        registered_types[i].probe = NULL;
        registered_types[i].mount = NULL;
        registered_types[i].list_directory = NULL;
        registered_types[i].find = NULL;
        registered_types[i].read_file = NULL;
    }

    for (uint32_t i = 0; i < FILESYSTEM_MAX_MOUNTS; i++) {
        mounted_filesystems[i].mount_path[0] = '\0';
        mounted_filesystems[i].device_id = 0;
        mounted_filesystems[i].initialized = 0;
        mounted_filesystems[i].type = NULL;
    }

    initialized = 1;

    return 1;
}

int filesystem_register(
    const struct filesystem_type *type
) {
    if (!initialized || type == NULL ||
        type->name == NULL ||
        type->probe == NULL ||
        type->mount == NULL ||
        type->list_directory == NULL ||
        type->find == NULL ||
        type->read_file == NULL) {
        return 0;
    }

    if (registered_type_count >= FILESYSTEM_MAX_TYPES) {
        return 0;
    }

    for (uint32_t i = 0; i < registered_type_count; i++) {
        if (strings_equal(
            registered_types[i].name,
            type->name
        )) {
            return 0;
        }
    }

    registered_types[registered_type_count] = *type;
    registered_type_count++;

    return 1;
}

int filesystem_mount_device(
    uint32_t device_id,
    const char *mount_path
) {
    if (!initialized || mount_path == NULL ||
        mount_path[0] != '/') {
        return 0;
    }

    if (mounted_filesystem_count >= FILESYSTEM_MAX_MOUNTS) {
        return 0;
    }

    size_t path_length = string_length(mount_path);

    if (path_length == 0 ||
        path_length >= FILESYSTEM_MAX_PATH_LENGTH) {
        return 0;
    }

    for (uint32_t i = 0; i < mounted_filesystem_count; i++) {
        if (strings_equal(
            mounted_filesystems[i].mount_path,
            mount_path
        )) {
            return 0;
        }
    }

    for (uint32_t i = 0; i < registered_type_count; i++) {
        const struct filesystem_type *type =
            &registered_types[i];

        if (!type->probe(device_id)) {
            continue;
        }

        if (!type->mount(device_id)) {
            continue;
        }

        struct filesystem_mount *mount =
            &mounted_filesystems[mounted_filesystem_count];

        copy_string(
            mount->mount_path,
            mount_path,
            sizeof(mount->mount_path)
        );

        mount->device_id = device_id;
        mount->type = type;
        mount->initialized = 1;

        mounted_filesystem_count++;

        return 1;
    }

    return 0;
}

int filesystem_is_initialized(void) {
    return initialized;
}

uint32_t filesystem_get_type_count(void) {
    return registered_type_count;
}

uint32_t filesystem_get_mount_count(void) {
    return mounted_filesystem_count;
}

const struct filesystem_mount *filesystem_get_mount(
    uint32_t index
) {
    if (index >= mounted_filesystem_count) {
        return NULL;
    }

    return &mounted_filesystems[index];
}

const struct filesystem_type *filesystem_get_type(
    uint32_t index
) {
    if (index >= registered_type_count) {
        return NULL;
    }

    return &registered_types[index];
}

int filesystem_list_directory(
    const char *path,
    struct filesystem_entry *entries,
    size_t capacity,
    size_t *entry_count
) {
    if (path == NULL || entry_count == NULL ||
        (capacity > 0 && entries == NULL)) {
        return 0;
    }

    const char *relative_path = NULL;

    const struct filesystem_mount *mount =
        resolve_mount(path, &relative_path);

    if (mount == NULL || mount->type == NULL) {
        return 0;
    }

    return mount->type->list_directory(
        relative_path,
        entries,
        capacity,
        entry_count
    );
}

int filesystem_find(
    const char *path,
    struct filesystem_entry *entry
) {
    if (path == NULL || entry == NULL) {
        return 0;
    }

    const char *relative_path = NULL;

    const struct filesystem_mount *mount =
        resolve_mount(path, &relative_path);

    if (mount == NULL || mount->type == NULL) {
        return 0;
    }

    return mount->type->find(
        relative_path,
        entry
    );
}

int filesystem_read_file(
    const char *path,
    void *buffer,
    size_t capacity,
    size_t *bytes_read
) {
    if (path == NULL || buffer == NULL ||
        bytes_read == NULL) {
        return 0;
    }

    const char *relative_path = NULL;

    const struct filesystem_mount *mount =
        resolve_mount(path, &relative_path);

    if (mount == NULL || mount->type == NULL) {
        return 0;
    }

    return mount->type->read_file(
        relative_path,
        buffer,
        capacity,
        bytes_read
    );
}