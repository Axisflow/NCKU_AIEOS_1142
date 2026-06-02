#ifndef VFS_FATFS_H
#define VFS_FATFS_H

#include "vfs.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "ff.h"

// Inherit from 'struct file_system' and add private data for FATFS
struct fat_fs {
    struct file_system base; // Base file system structure

    FATFS fs;
    SemaphoreHandle_t mutex;
};

int mount_fatfs(struct fat_fs *fs, const char *mount_point);
int unmount_fatfs(struct fat_fs *fs);

int fatfs_open(struct file *file, const char *path);
__vf_ssize_t fatfs_read(struct file *file, char *buf, size_t count);
__vf_ssize_t fatfs_write(struct file *file, const char *buf, size_t count);
loff_t fatfs_llseek(struct file *file, loff_t offset, int whence);
int fatfs_fsync(struct file *file, loff_t start, loff_t end, int datasync);
poll_t fatfs_poll(struct file *file, poll_t events, int timeout_ms);
loff_t fatfs_iterate_shared (struct file *file, char *path, size_t path_max_len);
int fatfs_close(struct file *file);

int fatfs_create(const struct file_system *fs, const char *path, const char *name, umode_t mode);
int fatfs_unlink(const struct file_system *fs, const char *path);

#endif // VFS_FATFS_H