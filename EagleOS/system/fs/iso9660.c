#include <iso9660.h>
#include <blockdev.h>

void *memcpy(void *destination, const void *source, size_t count) {
    volatile uint8_t *dest = (volatile uint8_t *)destination;
    const volatile uint8_t *src = (const volatile uint8_t *)source;

    for (size_t i = 0; i < count; i++) {
        dest[i] = src[i];
    }

    return destination;
}

#define ISO9660_PVD_LBA 16
#define ISO9660_MAX_DESCRIPTOR_SCAN 64
#define ISO9660_DIRECTORY_FLAG 0x02
#define ISO9660_MULTI_EXTENT_FLAG 0x80

static struct iso9660_volume mounted_volume;

static uint8_t sector_buffer[ISO9660_BLOCK_SIZE];

static uint16_t read_le16(const uint8_t *data) {
    return (uint16_t)data[0] |
        ((uint16_t)data[1] << 8);
}

static uint32_t read_le32(const uint8_t *data) {
    return (uint32_t)data[0] |
        ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[3] << 24);
}

static int character_to_upper(int character) {
    if (character >= 'a' && character <= 'z') {
        return character - 'a' + 'A';
    }

    return character;
}

static void copy_string(char *destination, const char *source, size_t capacity) {
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

static int read_sector(uint32_t lba, void *buffer) {
    if (!mounted_volume.mounted || buffer == NULL) {
        return ISO9660_FAILURE;
    }

    if (lba >= mounted_volume.volume_blocks) {
        return ISO9660_FAILURE;
    }

    return blockdev_read(
        mounted_volume.device_id,
        lba,
        1,
        buffer
    );
}

static int extent_is_valid(uint32_t lba, uint32_t size) {
    if (!mounted_volume.mounted) {
        return 0;
    }

    if (lba >= mounted_volume.volume_blocks) {
        return 0;
    }

    uint64_t blocks =
        ((uint64_t)size + ISO9660_BLOCK_SIZE - 1) /
        ISO9660_BLOCK_SIZE;

    if ((uint64_t)lba + blocks > mounted_volume.volume_blocks) {
        return 0;
    }

    return 1;
}

static int parse_directory_record(
    const uint8_t *record,
    uint8_t record_length,
    struct iso9660_directory_entry *entry
) {
    if (record == NULL || entry == NULL || record_length < 34) {
        return ISO9660_FAILURE;
    }

    uint8_t identifier_length = record[32];

    if (identifier_length == 0 ||
        (uint16_t)33 + identifier_length > record_length) {
        return ISO9660_FAILURE;
    }

    uint32_t extent_lba = read_le32(record + 2);
    uint32_t data_length = read_le32(record + 10);
    uint8_t flags = record[25];

    if (!extent_is_valid(extent_lba, data_length)) {
        return ISO9660_FAILURE;
    }

    entry->extent_lba = extent_lba;
    entry->size = data_length;
    entry->flags = flags;
    entry->is_directory =
        (flags & ISO9660_DIRECTORY_FLAG) != 0;

    size_t output_length = 0;

    for (uint8_t i = 0; i < identifier_length; i++) {
        uint8_t character = record[33 + i];

        if (character == ';') {
            break;
        }

        if (output_length + 1 >= ISO9660_MAX_NAME_LENGTH) {
            return ISO9660_FAILURE;
        }

        entry->name[output_length++] =
            (char)character;
    }

    entry->name[output_length] = '\0';

    return ISO9660_SUCCESS;
}

static int names_equal(const char *first, const char *second) {
    if (first == NULL || second == NULL) {
        return 0;
    }

    size_t first_length = 0;
    size_t second_length = 0;

    while (first[first_length] && first[first_length] != ';') {
        first_length++;
    }

    while (second[second_length] && second[second_length] != ';') {
        second_length++;
    }

    if (first_length != second_length) {
        return 0;
    }

    for (size_t i = 0; i < first_length; i++) {
        if (character_to_upper((uint8_t)first[i]) !=
            character_to_upper((uint8_t)second[i])) {
            return 0;
        }
    }

    return 1;
}

static int scan_directory(
    uint32_t directory_lba,
    uint32_t directory_size,
    struct iso9660_directory_entry *entries,
    size_t capacity,
    size_t *entry_count,
    const char *search_name,
    struct iso9660_directory_entry *found
) {
    if (!extent_is_valid(directory_lba, directory_size)) {
        return ISO9660_FAILURE;
    }

    if (entry_count != NULL) {
        *entry_count = 0;
    }

    uint32_t sectors =
        (directory_size + ISO9660_BLOCK_SIZE - 1) /
        ISO9660_BLOCK_SIZE;

    size_t total_entries = 0;

    for (uint32_t sector = 0; sector < sectors; sector++) {
        if (!read_sector(directory_lba + sector, sector_buffer)) {
            return ISO9660_FAILURE;
        }

        uint32_t sector_bytes = ISO9660_BLOCK_SIZE;

        uint32_t remaining =
            directory_size - sector * ISO9660_BLOCK_SIZE;

        if (remaining < sector_bytes) {
            sector_bytes = remaining;
        }

        uint32_t offset = 0;

        while (offset < sector_bytes) {
            uint8_t record_length = sector_buffer[offset];

            if (record_length == 0) {
                break;
            }

            if (record_length < 34 ||
                offset + record_length > sector_bytes) {
                return ISO9660_FAILURE;
            }

            struct iso9660_directory_entry entry;

            if (!parse_directory_record(
                &sector_buffer[offset],
                record_length,
                &entry
            )) {
                return ISO9660_FAILURE;
            }

            uint8_t identifier_length =
                sector_buffer[offset + 32];

            const uint8_t *identifier =
                &sector_buffer[offset + 33];

            int special_entry =
                identifier_length == 1 &&
                (identifier[0] == 0 || identifier[0] == 1);

            if (!special_entry) {
                if (search_name != NULL &&
                    names_equal(entry.name, search_name)) {

                    if (found != NULL) {
                        *found = entry;
                    }

                    return ISO9660_SUCCESS;
                }

                if (entries != NULL && total_entries < capacity) {
                    entries[total_entries] = entry;
                }

                total_entries++;
            }

            offset += record_length;
        }
    }

    if (entry_count != NULL) {
        *entry_count = total_entries;
    }

    return ISO9660_SUCCESS;
}

int iso9660_mount(uint32_t device_id) {
    struct block_device device;

    mounted_volume.mounted = 0;

    if (!blockdev_get_device(device_id, &device)) {
        return ISO9660_FAILURE;
    }

    if (!device.initialized ||
        device.block_size != ISO9660_BLOCK_SIZE ||
        device.block_count <= ISO9660_PVD_LBA) {
        return ISO9660_FAILURE;
    }

    uint8_t descriptor[ISO9660_BLOCK_SIZE];

    int found_primary = 0;

    for (uint32_t i = 0; i < ISO9660_MAX_DESCRIPTOR_SCAN; i++) {
        uint32_t lba = ISO9660_PVD_LBA + i;

        if (lba >= device.block_count) {
            break;
        }

        if (!blockdev_read(device_id, lba, 1, descriptor)) {
            return ISO9660_FAILURE;
        }

        if (descriptor[1] != 'C' ||
            descriptor[2] != 'D' ||
            descriptor[3] != '0' ||
            descriptor[4] != '0' ||
            descriptor[5] != '1' ||
            descriptor[6] != 1) {
            return ISO9660_FAILURE;
        }

        if (descriptor[0] == 1) {
            found_primary = 1;
            break;
        }

        if (descriptor[0] == 255) {
            break;
        }
    }

    if (!found_primary) {
        return ISO9660_FAILURE;
    }

    uint32_t volume_blocks = read_le32(descriptor + 80);
    uint16_t logical_block_size = read_le16(descriptor + 128);

    if (volume_blocks == 0 ||
        logical_block_size != ISO9660_BLOCK_SIZE ||
        volume_blocks > device.block_count) {
        return ISO9660_FAILURE;
    }

    uint8_t *root_record = &descriptor[156];

    if (root_record[0] < 34 ||
        !(root_record[25] & ISO9660_DIRECTORY_FLAG)) {
        return ISO9660_FAILURE;
    }

    uint32_t root_lba = read_le32(root_record + 2);
    uint32_t root_size = read_le32(root_record + 10);

    mounted_volume.device_id = device_id;
    mounted_volume.volume_blocks = volume_blocks;
    mounted_volume.logical_block_size = logical_block_size;
    mounted_volume.root_directory_lba = root_lba;
    mounted_volume.root_directory_size = root_size;
    mounted_volume.mounted = 1;

    if (!extent_is_valid(root_lba, root_size)) {
        mounted_volume.mounted = 0;
        return ISO9660_FAILURE;
    }

    return ISO9660_SUCCESS;
}

int iso9660_is_mounted(void) {
    return mounted_volume.mounted;
}

const struct iso9660_volume *iso9660_get_volume(void) {
    if (!mounted_volume.mounted) {
        return NULL;
    }

    return &mounted_volume;
}

static int resolve_path(
    const char *path,
    struct iso9660_directory_entry *result
) {
    if (!mounted_volume.mounted || path == NULL || result == NULL) {
        return ISO9660_FAILURE;
    }

    uint32_t current_lba =
        mounted_volume.root_directory_lba;

    uint32_t current_size =
        mounted_volume.root_directory_size;

    while (*path == '/') {
        path++;
    }

    if (*path == '\0') {
        result->extent_lba = current_lba;
        result->size = current_size;
        result->flags = ISO9660_DIRECTORY_FLAG;
        result->is_directory = 1;
        copy_string(result->name, "/", sizeof(result->name));
        return ISO9660_SUCCESS;
    }

    char component[ISO9660_MAX_NAME_LENGTH];

    for (;;) {
        size_t length = 0;

        while (*path && *path != '/') {
            if (length + 1 >= sizeof(component)) {
                return ISO9660_FAILURE;
            }

            component[length++] = *path++;
        }

        component[length] = '\0';

        if (length == 0) {
            return ISO9660_FAILURE;
        }

        while (*path == '/') {
            path++;
        }

        struct iso9660_directory_entry found;

        if (!scan_directory(
            current_lba,
            current_size,
            NULL,
            0,
            NULL,
            component,
            &found
        )) {
            return ISO9660_FAILURE;
        }

        if (*path == '\0') {
            *result = found;
            return ISO9660_SUCCESS;
        }

        if (!found.is_directory) {
            return ISO9660_FAILURE;
        }

        current_lba = found.extent_lba;
        current_size = found.size;
    }
}

int iso9660_list_directory(
    const char *path,
    struct iso9660_directory_entry *entries,
    size_t capacity,
    size_t *entry_count
) {
    if (path == NULL || entry_count == NULL) {
        return ISO9660_FAILURE;
    }

    struct iso9660_directory_entry directory;

    if (!resolve_path(path, &directory)) {
        return ISO9660_FAILURE;
    }

    if (!directory.is_directory) {
        return ISO9660_FAILURE;
    }

    return scan_directory(
        directory.extent_lba,
        directory.size,
        entries,
        capacity,
        entry_count,
        NULL,
        NULL
    );
}

int iso9660_find(
    const char *path,
    struct iso9660_directory_entry *entry
) {
    return resolve_path(path, entry);
}

int iso9660_read_file(
    const char *path,
    void *buffer,
    size_t capacity,
    size_t *bytes_read
) {
    if (bytes_read != NULL) {
        *bytes_read = 0;
    }

    if (path == NULL || buffer == NULL || bytes_read == NULL) {
        return ISO9660_FAILURE;
    }

    struct iso9660_directory_entry entry;

    if (!iso9660_find(path, &entry)) {
        return ISO9660_FAILURE;
    }

    if (entry.is_directory ||
        (entry.flags & ISO9660_MULTI_EXTENT_FLAG)) {
        return ISO9660_FAILURE;
    }

    if (entry.size > capacity) {
        return ISO9660_FAILURE;
    }

    if (!extent_is_valid(entry.extent_lba, entry.size)) {
        return ISO9660_FAILURE;
    }

    uint8_t *destination = (uint8_t *)buffer;

    uint32_t remaining = entry.size;
    uint32_t current_lba = entry.extent_lba;

    while (remaining > 0) {
        if (!read_sector(current_lba, sector_buffer)) {
            return ISO9660_FAILURE;
        }

        uint32_t amount = remaining;

        if (amount > ISO9660_BLOCK_SIZE) {
            amount = ISO9660_BLOCK_SIZE;
        }

        for (uint32_t i = 0; i < amount; i++) {
            destination[i] = sector_buffer[i];
        }

        destination += amount;
        remaining -= amount;
        current_lba++;
    }

    *bytes_read = entry.size;

    return ISO9660_SUCCESS;
}