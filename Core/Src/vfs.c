#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "projdefs.h"
#include "portable.h"
#include "vfs.h"

struct __vfs_hlist {
    struct file_system *fs;
    struct __vfs_hlist *next;
};

static struct __vfs_hlist *__mapping = NULL;

vf_result_t __valid_path(const char *path) {
    if (!path || path[0] == '\0' || path[0] != '/') { // More condictions?
        return VF_INVALID; // Invalid path
    }
    return VF_SUCCESS; // Valid path
}

vfs_result_t vfs_mount(struct file_system *fs) {
    if (!fs || !fs->mount_point || __valid_path(fs->mount_point) != VF_SUCCESS) {
        return VF_INVALID; // Invalid parameters
    }

    if (__valid_path(fs->mount_point) != VF_SUCCESS) {
        return VF_INVALID; // Mount point is not valid
    }

    struct __vfs_hlist *new_node = (struct __vfs_hlist *)pvPortMalloc(sizeof(struct __vfs_hlist));
    if (!new_node) {
        return VF_ERROR; // Memory allocation failed
    }

    new_node->fs = fs;
    new_node->next = __mapping;
    __mapping = new_node;

    return VF_SUCCESS; // Mounted successfully
}

vfs_result_t vfs_unmount(struct file_system *fs) {
    if (!fs) {
        return VF_INVALID; // Invalid parameters
    }

    for (struct __vfs_hlist **cur = &__mapping; *cur; cur = &(*cur)->next) {
        if ((*cur)->fs == fs) {
            void *to_delete = (void *) *cur;
            *cur = (*cur)->next;
            vPortFree(to_delete);
            return VF_SUCCESS; // Unmounted successfully
        }
    }

    return VF_NOT_FOUND; // File system not found
}

struct file_system *vfs_lookup(const char *path) {
    if (__valid_path(path) != VF_SUCCESS) {
        return NULL; // Invalid path
    }
    
    for (struct __vfs_hlist *current = __mapping; current; current = current->next) {
        if (strcmp(path, current->fs->mount_point) == 0) {
            return current->fs; // Found the mounted file system
        }
    }

    return NULL; // Not found
}

vf_result_t vf_open(struct file *fp, const char *path, unsigned int flags) {
    struct file_system *mounted = vfs_lookup(path);
    if (!mounted) {
        return VF_NOT_FOUND; // Not found
    }

    fp->f_flags = flags;
    fp->fs = mounted;

    if (mounted->fops && mounted->fops->open) {
        return mounted->fops->open(fp, path);
    }

    return VF_SUCCESS; // Opened successfully
}

vf_result_t vf_close(struct file *fp) {
    if (!fp) {
        return VF_ERROR; // Invalid file
    }

    if (fp->fs->fops && fp->fs->fops->close) {
        return fp->fs->fops->close(fp);
    }

    return VF_SUCCESS; // Closed successfully
}


__vf_ssize_t vf_read(struct file *fp, char *buf, size_t btr) {
    if (!fp) {
        return VF_ERROR; // Invalid file
    }

    if (!fp->fs->fops || !fp->fs->fops->read) {
        return VF_INVALID; // No read operation defined
    }

    return fp->fs->fops->read(fp, buf, btr);
}

__vf_ssize_t vf_write(struct file *fp, const char *buf, size_t btw) {
    if (!fp) {
        return VF_ERROR; // Invalid file
    }

    if (!fp->fs->fops || !fp->fs->fops->write) {
        return VF_INVALID; // No write operation defined
    }

    return fp->fs->fops->write(fp, buf, btw);
}

loff_t vf_llseek(struct file *file, loff_t offset, int whence) {
    if (!file) {
        return VF_ERROR; // Invalid file
    }

    if (!file->fs->fops || !file->fs->fops->llseek) {
        return VF_INVALID; // No llseek operation defined
    }

    return file->fs->fops->llseek(file, offset, whence);
}

vf_result_t vf_fsync(struct file *file, int datasync) {
    if (!file) {
        return VF_ERROR; // Invalid file
    }

    if (!file->fs->fops || !file->fs->fops->fsync) {
        return VF_INVALID; // No fsync operation defined
    }

    return file->fs->fops->fsync(file, 0, OFFSET_MAX, datasync);
}

poll_t vf_poll(struct file *file, struct poll_table_struct *pt) {
    if (!file) {
        return POLLNVAL; // Invalid file
    }

    if (!file->fs->fops || !file->fs->fops->poll) {
        return POLLERR; // No poll operation defined
    }

    return file->fs->fops->poll(file, pt);
}

loff_t vf_readdir (struct file *file, size_t *count, char *path, const size_t path_max_len) {
    if (!file) {
        return (loff_t) VF_ERROR; // Invalid file
    }

    if (!file->fs->fops || !file->fs->fops->iterate_shared) {
        return (loff_t) VF_INVALID; // No readdir operation defined
    }

    return file->fs->fops->iterate_shared(file, count, path, path_max_len);
}

vf_result_t vf_mkdir(const char *path, const char *name, umode_t mode) {
    struct file_system *mounted = vfs_lookup(path);
    if (!mounted) {
        return VF_NOT_FOUND; // Not found
    }

    if (!mounted->nops || !mounted->nops->create) {
        return VF_INVALID; // No mkdir operation defined
    }

    return mounted->nops->create(path, name, mode);
}

vf_result_t vf_unlink(const char *path) {
    struct file_system *mounted = vfs_lookup(path);
    if (!mounted) {
        return VF_NOT_FOUND; // Not found
    }

    if (!mounted->nops || !mounted->nops->unlink) {
        return VF_INVALID; // No unlink operation defined
    }

    return mounted->nops->unlink(path);
}

vf_result_t vf_rmdir(const char *path) {
    struct file_system *mounted = vfs_lookup(path);
    if (!mounted) {
        return VF_NOT_FOUND; // Not found
    }

    if (!mounted->nops || !mounted->nops->rmdir) {
        return VF_INVALID; // No rmdir operation defined
    }

    return mounted->nops->rmdir(path);
}
