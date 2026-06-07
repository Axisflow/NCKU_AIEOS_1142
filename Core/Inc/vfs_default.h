#ifndef VFS_DEFAULT_H
#define VFS_DEFAULT_H

#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

vfs_result_t mount_default(struct file_system *fs, const char *mount_point);
vfs_result_t unmount_default(struct file_system *fs);

#ifdef __cplusplus
}
#endif

#endif /* VFS_DEFAULT_H */
