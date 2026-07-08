#include "vfs_default.h"

#include "FreeRTOS.h"
#include "task.h"

#define DNAME_INITIAL_CAPACITY 64

static int default_open(struct file *file, const char *path);
static int default_close(struct file *file);
static loff_t default_iterate_shared(struct file *file, char *buf, size_t btr);
static poll_t default_poll(struct file *file, poll_t events, int timeout_ms);

static struct file_operations default_fops = {
    .open = default_open,
    .read = NULL,
    .write = NULL,
    .llseek = NULL,
    .fsync = NULL,
    .poll = default_poll,
    .iterate_shared = default_iterate_shared,
    .close = default_close
};

vfs_result_t mount_default(struct file_system *fs, const char *mount_point) {
    if (!fs || !mount_point) {
        return VF_INVALID; // Invalid parameters
    }

    strncpy(fs->name, "ROMFS", sizeof(fs->name) - 1);
    fs->mount_point = mount_point;
    fs->fops = &default_fops;
    fs->nops = NULL; // No node operations supported
    return vfs_mount(fs);
}

vfs_result_t unmount_default(struct file_system *fs) {
    if (!fs) {
        return VF_INVALID; // Invalid parameters
    }

    fs->mount_point = NULL; // Clear the mount point to indicate unmounted
    return vfs_unmount(fs);
}

struct __record_hlist {
    char *d_name; // The entry name (either "." or ".." or the actual entry name)
    struct __record_hlist *next; // Pointer to the next entry
};

struct __default_file_data {
    char *path; // The opened directory path
    struct __fs_hlist **curr; // The node corresponding to the file
    struct __record_hlist *records; // The list of directory entries that have been emitted so far
    char *d_name; // The current entry name (either "." or ".." or the actual entry name)
    size_t d_name_cap; // The capacity of the d_name buffer
    loff_t d_off; // Entry index for iteration
    size_t d_reclen; // Directory record length (the remaining length of the current entry name)
};

extern struct __fs_hlist *__fs_mapping;

static int default_open(struct file *file, const char *path) {
    if (!file || !path) {
        return -1; // Invalid parameters
    }

    // Ensure the opening path starts with one of the mount points in the VFS mapping
    const struct __fs_hlist *node = __fs_mapping;
    while (node != NULL && (strncmp(path, node->fs->mount_point, strlen(path)) || node->fs->mount_point[strlen(path)] != '/')) {
        node = node->next;
    }

    if (!node) {
        return -1; // No matching mount point found
    }

    struct __default_file_data *data = (struct __default_file_data *)pvPortMalloc(sizeof(struct __default_file_data));
    if (!data) {
        return -1; // Memory allocation failed
    }

    data->d_name_cap = DNAME_INITIAL_CAPACITY;
    data->d_name = (char *)pvPortMalloc(data->d_name_cap);
    if (!data->d_name) {
        vPortFree(data);
        return -1; // Memory allocation failed
    }

    data->d_name[0] = '\0'; // Initialize the entry name buffer to an empty string
    data->path = (char *)pvPortMalloc(strlen(path) + 1);
    if (!data->path) {
        vPortFree(data->d_name);
        vPortFree(data);
        return -1; // Memory allocation failed
    }

    strcpy(data->path, path);

    data->curr = &__fs_mapping;
    data->records = NULL; // Initialize the records list to NULL
    data->d_off = 0;
    data->d_reclen = 0;
    file->private_data = data;
    return 0; // Success
}

static int default_close(struct file *file) {
    if (!file || !file->private_data) {
        return -1; // Invalid parameters
    }

    struct __default_file_data *data = (struct __default_file_data *) file->private_data;
    struct __record_hlist *record = data->records;
    while (record) {
        struct __record_hlist *next = record->next;
        vPortFree(record->d_name);
        vPortFree(record);
        record = next;
    }

    vPortFree(data->path);
    vPortFree(data->d_name);
    vPortFree(data);
    file->private_data = NULL;
    return 0; // Success
}

static poll_t default_poll(struct file *file, poll_t events, int timeout_ms) {
    if (!file) {
        return POLLNVAL; // Invalid file
    }


    while(((struct __default_file_data *) file->private_data)->curr == NULL) {
        vTaskDelay(pdMS_TO_TICKS(timeout_ms));
    }

    return events & POLLIN; // Always ready for reading when there are entries to read
}

static loff_t default_iterate_shared(struct file *file, char *path, size_t path_max_len) {
    if (!file || !file->private_data || !path) {
        return -1; // Invalid parameters
    }

    struct __default_file_data *data = (struct __default_file_data *) file->private_data;

    // If d_reclen > 0, it means there are still remaining characters in the current entry name to be emitted
    if (data->d_reclen > 0) {
        // Do not move to the next entry, just emit the remaining characters of the current entry name
    }

    // First call must emit "."
    else if (data->d_off == 0) {
        data->d_reclen = strlen(__syn_current_dir); // The length of "." is 1
        strcpy(data->d_name, __syn_current_dir); // Set the current entry name to "."
    }

    // Second call emit ".." if it is not the root directory
    else if (data->d_off == 1 && strcmp(data->path, "")) {
        data->d_reclen = strlen(__syn_parent_dir); // The length of ".." is 2
        strcpy(data->d_name, __syn_parent_dir); // Set the current entry name to ".."
    }

    // Subsequent calls emit the actual entry names of the mounted file systems
    /*
     * example: if data->path is root (""), and there are two mounted file systems with mount points "fatfs" and "dev/uart2tty" under the root,
     * then the third call will emit "fatfs" and the fourth call will emit "dev".
     */
    else {
        while (*data->curr && (strncmp(data->path, (*data->curr)->fs->mount_point, strlen(data->path)) ||
               (*data->curr)->fs->mount_point[strlen(data->path)] != '/')) {
            data->curr = &(*data->curr)->next; // Move to the next mounted file system
        }

        if (*data->curr == NULL) {
            return ++data->d_off; // No more entries to read
        }

        const char *entry_name = (*data->curr)->fs->mount_point + strlen(data->path) + 1; // Get the entry name by removing the current path prefix
        size_t entry_name_len = strcspn(entry_name, "/"); // The entry name is the substring until the next '/' or the end of string

        if (entry_name_len >= data->d_name_cap) {
            // Reallocate d_name buffer if the entry name exceeds the current capacity
            size_t new_cap = entry_name_len + 1; // +1 for null terminator
            char *new_buf = (char *)pvPortMalloc(new_cap);
            if (!new_buf) {
                return data->d_off; // Memory allocation failed
            }
            vPortFree(data->d_name);
            data->d_name = new_buf;
            data->d_name_cap = new_cap;
        }

        // Check if the entry name is emitted for the first time, if so, add it to the records list; otherwise, find the existing record and update d_reclen accordingly
        struct __record_hlist *record = data->records;
        while (record) {
            if (strncmp(record->d_name, entry_name, entry_name_len) == 0 && record->d_name[entry_name_len] == '\0') {
                break; // Found the existing record for the current entry name
            }
            record = record->next;
        }

        if (record) {
            return data->d_off; // The entry name has been emitted before, just return the current entry index
        }

        // Create a new record for the current entry name
        record = (struct __record_hlist *)pvPortMalloc(sizeof(struct __record_hlist));
        if (!record) {
            return data->d_off; // Memory allocation failed
        }
        record->d_name = (char *)pvPortMalloc(entry_name_len + 1);
        if (!record->d_name) {
            vPortFree(record);
            return data->d_off; // Memory allocation failed
        }

        // Copy the entry name to the record and add it to the records list
        strncpy(record->d_name, entry_name, entry_name_len);
        record->d_name[entry_name_len] = '\0'; // Null terminate the entry name
        record->next = data->records;
        data->records = record;

        // Copy the entry name to the d_name buffer and update d_reclen accordingly
        strncpy(data->d_name, entry_name, entry_name_len);
        data->d_name[entry_name_len] = '\0'; // Null terminate the entry name
        data->d_reclen = entry_name_len; // Set the record length to the length of the emitted entry name

        if (entry_name[entry_name_len] == '/') {
            data->d_reclen++; // If there are more subdirectories, include the '/' in the record length for the next iteration
        }

        data->curr = &(*data->curr)->next; // Move to the next mounted file system for the next iteration
    }

    // Fill the path buffer with the current entry name and update d_reclen accordingly
    if (vfs_dir_emit(data->d_name, path, path_max_len, &data->d_reclen) != VF_SUCCESS) {
        return data->d_off;
    }

    // Return the same entry index if there are still remaining characters, otherwise move to the next entry
    if (!data->d_reclen) {
        data->d_off++; // Move to the next entry index
    }

    return data->d_off;
}
