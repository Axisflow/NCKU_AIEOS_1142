#ifndef VFS_FATFS_H
#define VFS_FATFS_H

#include "vfs.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inherit from 'struct file_system' and add private data for FATFS
struct fat_fs {
    struct file_system base; // Base file system structure

    FATFS fs;
    SemaphoreHandle_t mutex;
};

int mount_fatfs(struct fat_fs *fs, const char *mount_point);
int unmount_fatfs(struct fat_fs *fs);

#ifdef __cplusplus
}
#endif

#endif // VFS_FATFS_H