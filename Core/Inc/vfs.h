#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <limits.h>

struct file {
    unsigned int f_flags;
    const struct file_system *fs;
    size_t offset;
    void *private_data;
};

#ifndef ssize_t
typedef long long ssize_t;
#endif

#ifndef loff_t
typedef long long loff_t;
#endif

#define OFFSET_MAX LLONG_MAX

typedef unsigned int poll_t;

#define POLLIN   (1L << 0)
#define POLLOUT  (1L << 1)
#define POLLERR  (1L << 2)
#define POLLHUP  (1L << 3)
#define POLLNVAL (1L << 4)

struct poll_table_struct {
    unsigned int events;
    unsigned int revents;
    int timeout_ms;
};

typedef enum {
    VF_SUCCESS = 0,
    VF_ERROR = -1,
    VF_NOT_FOUND = -2,
    VF_INVALID = -22,
} vf_result_t;

vf_result_t vf_open(struct file *file, const char *path, unsigned int flags);
ssize_t vf_read(struct file *file, char *buf, size_t btr);
ssize_t vf_write(struct file *file, const char *buf, size_t btw);
loff_t vf_llseek(struct file *file, loff_t offset, int whence);
vf_result_t vf_fsync(struct file *file, int datasync);
poll_t vf_poll(struct file *file, struct poll_table_struct *pt);
loff_t vf_readdir (struct file *file, size_t *count, char *path, const size_t path_max_len);
vf_result_t vf_close(struct file *file);

typedef unsigned int umode_t;

vf_result_t vf_mkdir(const char *path, const char *name, umode_t mode);
vf_result_t vf_unlink(const char *path);
vf_result_t vf_rmdir(const char *path);

struct file_operations {
  int (*open)(struct file *file, const char *path);
  ssize_t (*read)(struct file *file, void *buf, size_t count);
  ssize_t (*write)(struct file *file, const void *buf, size_t count);
  loff_t (*llseek)(struct file *file, loff_t offset, int whence);
  int (*fsync) (struct file *file, loff_t start, loff_t end, int datasync);
  poll_t (*poll)(struct file *file, struct poll_table_struct *pt);
  loff_t (*iterate_shared) (struct file *file, size_t *count, char *path, const size_t path_max_len);
  int (*close)(struct file *file);
};

struct node_operations {
  int (*create)(const char *path, const char *name, umode_t mode);
  int (*unlink)(const char *path);
  int (*rmdir)(const char *path);
};

struct file_system {
    const char name[16];
    const char *mount_point;
    const struct node_operations *nops;
    const struct file_operations *fops;
    void *private_data;
};

typedef vf_result_t vfs_result_t;

vfs_result_t vfs_mount(struct file_system *fs);
struct file_system *vfs_lookup(const char *path);
vfs_result_t vfs_unmount(struct file_system *fs);

#endif /* VFS_H */
