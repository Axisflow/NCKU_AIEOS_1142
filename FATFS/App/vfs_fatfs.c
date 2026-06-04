#include "vfs_fatfs.h"

#include <string.h>
#include <stdio.h>

static int fatfs_open(struct file *file, const char *path);
static __vf_ssize_t fatfs_read(struct file *file, char *buf, size_t count);
static __vf_ssize_t fatfs_write(struct file *file, const char *buf, size_t count);
static loff_t fatfs_llseek(struct file *file, loff_t offset, int whence);
static int fatfs_fsync(struct file *file, loff_t start, loff_t end, int datasync);
static poll_t fatfs_poll(struct file *file, poll_t events, int timeout_ms);
static loff_t fatfs_iterate_shared (struct file *file, char *path, size_t path_max_len);
static int fatfs_close(struct file *file);

static int fatfs_create(const struct file_system *fs, const char *path, const char *name, umode_t mode);
static int fatfs_unlink(const struct file_system *fs, const char *path);

static const struct node_operations fatfs_nops = {
    .create = fatfs_create,
    .unlink = fatfs_unlink,
    .rmdir = fatfs_unlink, // FATFS does not distinguish between unlink and rmdir
};

static const struct file_operations fatfs_fops = {
    .open = fatfs_open,
    .read = fatfs_read,
    .write = fatfs_write,
    .llseek = fatfs_llseek,
    .fsync = fatfs_fsync,
    .poll = fatfs_poll,
    .iterate_shared = fatfs_iterate_shared,
    .close = fatfs_close,
};

int mount_fatfs(struct fat_fs *fs, const char *mount_point)
{
    if (!fs || !mount_point) {
        return -1; // Invalid parameters
    }

    fs->mutex = xSemaphoreCreateMutex();
    if (fs->mutex == NULL) {
        return -1; // Failed to create mutex
    }

    FRESULT res = f_mount(&fs->fs, "", 1);
    if (res != FR_OK) {
        vSemaphoreDelete(fs->mutex);
        return -res; // Failed to mount file system
    }

    strncpy(fs->base.name, "FATFS", sizeof(fs->base.name) - 1);
    fs->base.mount_point = mount_point; // Store the mount point for later use
    fs->base.nops = &fatfs_nops;
    fs->base.fops = &fatfs_fops;

    xSemaphoreTake(fs->mutex, portMAX_DELAY); // Ensure no other thread is using the file system
    vf_result_t vfs_res = vfs_mount((const struct file_system *) fs);
    if (vfs_res != VF_SUCCESS) {
        f_mount(NULL, "", 0); // Unmount the file system
        vSemaphoreDelete(fs->mutex);
        return vfs_res; // Failed to mount to VFS
    }

    xSemaphoreGive(fs->mutex);
    return 0; // Success
}

int unmount_fatfs(struct fat_fs *fs)
{
    if (!fs) {
        return -1; // Invalid parameter
    }

    xSemaphoreTake(fs->mutex, portMAX_DELAY); // Ensure no other thread is using the file system
    vf_result_t vfs_res = vfs_unmount((const struct file_system *) fs);
    if (vfs_res != VF_SUCCESS) {
        xSemaphoreGive(fs->mutex);
        return vfs_res; // Failed to unmount from VFS
    }

    f_mount(NULL, "", 0); // Unmount the file system
    fs->base.mount_point = NULL; // Clear the mount point
    vSemaphoreDelete(fs->mutex);
    return 0; // Success
}

#define AM_NOT_ROOT 0x80
struct fatfs_file {
    union {
        FIL file; // For regular files
        struct {
            DIR dir;  // For directories
            BYTE fattrib;
            loff_t d_off; // Entry index for iteration
            unsigned char d_reclen; // Directory record length (the remaining length of the current entry name)
            FILINFO _info;
        };
    };
};

static int fatfs_open(struct file *file, const char *path)
{
    struct fatfs_file *data = pvPortMalloc(sizeof(struct fatfs_file));
    if (data == NULL) {
        return -1; // Memory allocation failed
    }

    const char *_p = path + strlen(((struct fat_fs *) file->fs)->base.mount_point);

    // check if file is directory or not, if directory, use f_opendir & DIR*
    SemaphoreHandle_t mutex = ((struct fat_fs *) file->fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    FRESULT res = f_opendir(&data->dir, _p);
    xSemaphoreGive(mutex);
    if (res == FR_NO_PATH) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        res = f_open(&data->file, _p, (BYTE) file->f_flags);
        xSemaphoreGive(mutex);
        if (res != FR_OK) {
            vPortFree(data);
            return -res; // Failed to open file
        }
    } else if (res != FR_OK) {
        vPortFree(data);
        return -res; // Failed to open directory
    } else {
        data->fattrib = AM_DIR; // Mark as directory
        if (strcmp(_p, "")) data->fattrib |= AM_NOT_ROOT; // Mark as not root
    }

    file->private_data = data;
    return 0; // Success
}

static int fatfs_close(struct file *file)
{
    struct fatfs_file *data = (struct fatfs_file *) file->private_data;
    if (data == NULL) {
        return -1; // Invalid file pointer
    }

    SemaphoreHandle_t mutex = ((struct fat_fs *) file->fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (data->fattrib & AM_DIR) {
        f_closedir(&data->dir);
    } else {
        f_close(&data->file);
    }

    xSemaphoreGive(mutex);
    vPortFree(data);
    file->private_data = NULL;
    return 0; // Success
}

static __vf_ssize_t fatfs_read(struct file *file, char *buf, size_t count)
{
    struct fatfs_file *data = (struct fatfs_file *) file->private_data;
    if (data == NULL) {
        return -1; // Invalid file pointer
    }

    UINT bytesRead;
    SemaphoreHandle_t mutex = ((struct fat_fs *) file->fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    FRESULT res = f_read(&data->file, buf, count, &bytesRead);
    xSemaphoreGive(mutex);
    if (res != FR_OK) {
        return -1; // Failed to read from file
    }

    return bytesRead; // Return number of bytes read
}

static __vf_ssize_t fatfs_write(struct file *file, const char *buf, size_t count)
{
    struct fatfs_file *data = (struct fatfs_file *) file->private_data;
    if (data == NULL) {
        return -1; // Invalid file pointer
    }

    UINT bytesWritten;
    SemaphoreHandle_t mutex = ((struct fat_fs *) file->fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    FRESULT res = f_write(&data->file, buf, count, &bytesWritten);
    xSemaphoreGive(mutex);
    if (res != FR_OK) {
        return -1; // Failed to write to file
    }

    return bytesWritten; // Return number of bytes written
}

static loff_t fatfs_llseek(struct file *file, loff_t offset, int whence)
{
    struct fatfs_file *data = (struct fatfs_file *) file->private_data;
    if (data == NULL) {
        return -1; // Invalid file pointer
    }

    // decorate the f_lseek & check the size loff_t not exceed the FSIZE_t
    if (offset < 0 || offset > (loff_t) UINT_MAX) {
        return -1; // Invalid offset
    }

    FSIZE_t newPos;
    switch (whence) {
        case SEEK_SET:
            newPos = (FSIZE_t) offset;
            break;
        case SEEK_CUR:
            newPos = data->file.fptr + (FSIZE_t) offset;
            break;
        case SEEK_END:
            newPos = f_size(&data->file) + (FSIZE_t) offset;
            break;
        default:
            return -1; // Invalid whence
    }

    if (newPos > f_size(&data->file)) {
        return -1; // Seeking beyond file size
    }

    SemaphoreHandle_t mutex = ((struct fat_fs *) file->fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    FRESULT res = f_lseek(&data->file, newPos);
    xSemaphoreGive(mutex);
    if (res != FR_OK) {
        return -1; // Failed to seek
    }

    return newPos; // Return new file position
}

static int fatfs_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
    struct fatfs_file *data = (struct fatfs_file *) file->private_data;
    if (data == NULL) {
        return -1; // Invalid file pointer
    }

    SemaphoreHandle_t mutex = ((struct fat_fs *) file->fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    FRESULT res = f_sync(&data->file);
    xSemaphoreGive(mutex);

    return -res;
}   

static poll_t fatfs_poll(struct file *file, poll_t events, int timeout_ms)
{
    struct fatfs_file *data = (struct fatfs_file *) file->private_data;
    if (data == NULL) {
        return POLLNVAL; // Invalid file pointer
    }

    poll_t revents = 0;

    if ((events & POLLIN) && (data->fattrib & AM_DIR || data->file.fptr < f_size(&data->file))) {
        revents |= POLLIN;
    }

    if ((events & POLLOUT)) {
        revents |= POLLOUT;
    }

    return revents;
}

static int fatfs_dir_emit(struct fatfs_file *data, char *path, size_t path_max_len)
{
    // Emit the current directory entry name to the path buffer
    size_t name_len = strlen(data->_info.fname);
    size_t emit_len = (data->d_reclen < path_max_len) ? data->d_reclen : path_max_len;
    memcpy(path, data->_info.fname + (name_len - data->d_reclen), emit_len);
    data->d_reclen -= emit_len;
    return 0; // Success
}

static loff_t fatfs_iterate_shared(struct file *file, char *path, size_t path_max_len)
{
    struct fatfs_file *data = (struct fatfs_file *) file->private_data;
    if (data == NULL) {
        return -1; // Invalid directory pointer
    }

    // If d_reclen > 0, it means there are still remaining characters in the current entry name to be emitted
    if (data->d_reclen > 0) {
        // Do not move to the next entry, just emit the remaining characters of the current entry name
    }

    // First call must emit "."
    else if (data->d_off == 0) {
        data->d_reclen = 1; // The length of "." is 1
        data->_info.fname[0] = '.'; // Set the current entry name to "."
    }

    // Second call emit ".." if it is not the root directory
    else if (data->d_off == 1 && (data->fattrib & AM_NOT_ROOT)) {
        data->d_reclen = 2; // The length of ".." is 2
        data->_info.fname[0] = '.'; // Set the current entry name to ".."
        data->_info.fname[1] = '.';
    }

    // Then emit the entries in the directory one by one
    else {
        SemaphoreHandle_t mutex = ((struct fat_fs *) file->fs)->mutex;
        xSemaphoreTake(mutex, portMAX_DELAY);
        FRESULT res = f_readdir(&data->dir, &data->_info);
        xSemaphoreGive(mutex);
        if (res != FR_OK) {
            return -res; // Failed to read directory
        }
        
        data->d_reclen = strlen(data->_info.fname); // Set the record length to the length of the entry name
    }

    // Fill the path buffer with the current entry name and update d_reclen accordingly
    fatfs_dir_emit(data, path, path_max_len);

    // Return the same entry index if there are still remaining characters, otherwise move to the next entry
    if (!data->d_reclen) {
        data->d_off++; // Move to the next entry index
    }

    return data->d_off;
}

static int fatfs_create(const struct file_system *fs, const char *path, const char *name, umode_t mode)
{
    const char *_p = path + strlen(((struct fat_fs *) fs)->base.mount_point);

    // Create a new directory entry with the specified name and mode under the given path
    char *full_path = pvPortMalloc(strlen(_p) + strlen(name) + 2); // Allocate memory for full path
    if (!full_path) {
        return -1; // Failed to allocate memory
    }

    snprintf(full_path, strlen(_p) + strlen(name) + 2, "%s/%s", _p, name);
    SemaphoreHandle_t mutex = ((struct fat_fs *) fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    int result = f_mkdir(full_path);
    xSemaphoreGive(mutex);
    vPortFree(full_path); // Free the allocated memory
    return result;
}

static int fatfs_unlink(const struct file_system *fs, const char *path)
{
    SemaphoreHandle_t mutex = ((struct fat_fs *) fs)->mutex;
    xSemaphoreTake(mutex, portMAX_DELAY);
    int result = f_unlink(path + strlen(((struct fat_fs *) fs)->base.mount_point));
    xSemaphoreGive(mutex);
    return result;
}
