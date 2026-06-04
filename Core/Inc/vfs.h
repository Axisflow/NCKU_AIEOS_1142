#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

struct file {
    // The flags used to open the file (e.g., O_RDONLY, O_WRONLY, O_RDWR, etc.)
    unsigned int f_flags;

    // The file system that this file belongs to (e.g., FATFS, DEVFS, etc.)
    const struct file_system *fs;

    // The current file position (offset) in bytes from the beginning of the file
    size_t offset;

    // Private data for the file implementation (e.g., a pointer to a file descriptor, a directory iterator, etc.)
    void *private_data;
};

typedef long long __vf_ssize_t;

#ifndef loff_t
typedef long long loff_t;
#endif

#define OFFSET_MAX LLONG_MAX

typedef unsigned int poll_t;

#define POLLIN   (1L << 0) // Data other than high-priority data may be read without blocking (e.g., the file is not empty, or the device has data to read)
#define POLLOUT  (1L << 1) // Writing is now possible (e.g., the file is not full, or the device is ready for writing)
#define POLLERR  (1L << 2) // An error occurred
#define POLLHUP  (1L << 3) // The file descriptor is closed
#define POLLNVAL (1L << 4) // The file descriptor is invalid

typedef enum {
    VF_SUCCESS = 0,
    VF_ERROR = -1,
    VF_NOT_FOUND = -2,
    VF_INVALID = -22,
} vf_result_t;

vf_result_t vf_open(struct file *file, const char *path, unsigned int flags);
__vf_ssize_t vf_read(struct file *file, char *buf, size_t btr);
__vf_ssize_t vf_write(struct file *file, const char *buf, size_t btw);
loff_t vf_llseek(struct file *file, loff_t offset, int whence);
vf_result_t vf_fsync(struct file *file, int datasync);
poll_t vf_poll(struct file *file, poll_t events, int timeout_ms);
loff_t vf_readdir (struct file *file, char *path, size_t path_max_len);
vf_result_t vf_close(struct file *file);

typedef unsigned int umode_t;

vf_result_t vf_mkdir(const char *path, const char *name, umode_t mode);
vf_result_t vf_unlink(const char *path);
vf_result_t vf_rmdir(const char *path);

struct file_operations {
    // Open a file ([m]allocated and fill the file structure). Returns 0 on success, or a negative error code.
    int (*open)(struct file *file, const char *path);

    // Read from a file. Returns the number of bytes read, or a negative error code.
    __vf_ssize_t (*read)(struct file *file, char *buf, size_t count);

    // Write to a file. Returns the number of bytes written, or a negative error code.
    __vf_ssize_t (*write)(struct file *file, const char *buf, size_t count);

    // Change the file position. Returns the new file position, or a negative error code.
    loff_t (*llseek)(struct file *file, loff_t offset, int whence);

    // Synchronize the file's in-memory state with the storage device. Returns 0 on success, or a negative error code.
    int (*fsync) (struct file *file, loff_t start, loff_t end, int datasync);

    // Poll for events on the file. Returns a bitmask of events that occurred, or a negative error code.
    poll_t (*poll)(struct file *file, poll_t events, int timeout_ms);

    // Iterate over directory entries. Returns the next entry position, or a negative error code.
    loff_t (*iterate_shared) (struct file *file, char *buf, size_t btr);

    // Close a file (and release any associated or [m]allocated resources). Returns 0 on success, or a negative error code.
    int (*close)(struct file *file);
};

struct node_operations {
    // Create a directory. Returns 0 on success, or a negative error code.
    int (*create)(const struct file_system *fs, const char *path, const char *name, umode_t mode);

    // Remove a file. Returns 0 on success, or a negative error code.
    int (*unlink)(const struct file_system *fs, const char *path);

    // Remove a directory. Returns 0 on success, or a negative error code.
    int (*rmdir)(const struct file_system *fs, const char *path);
};

struct file_system {
    // the name of the file system, e.g., "FATFS", "DEVFS", etc.
    char name[16];

    // the mount point for this file system, e.g., "/fatfs", "/dev", etc.
    const char *mount_point;

    // the operations supported by this file system
    const struct node_operations *nops;

    // the file operations supported by this file system
    const struct file_operations *fops;

    // private data for the file system implementation
};

typedef vf_result_t vfs_result_t;

// Mount a file system to the VFS
vfs_result_t vfs_mount(const struct file_system *fs);

// Look up a file system by its mount point or its subdirectories. Returns NULL if not found.
const struct file_system *vfs_lookup(const char *path);

// Unmount a file system from the VFS
vfs_result_t vfs_unmount(const struct file_system *fs);

#ifdef __cplusplus
}
#endif

#endif /* VFS_H */
