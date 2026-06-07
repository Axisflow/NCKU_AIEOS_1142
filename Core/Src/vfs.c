#include <stdint.h>

#include "FreeRTOS.h"
#include "vfs.h"

struct __fs_hlist *__fs_mapping = NULL;

vfs_result_t vfs_mount(const struct file_system *fs) {
    if (!fs || !__valid_path(fs->mount_point)) {
        return VF_INVALID; // Invalid parameters
    }

    struct __fs_hlist *new_node = (struct __fs_hlist *)pvPortMalloc(sizeof(struct __fs_hlist));
    if (!new_node) {
        return VF_ERROR; // Memory allocation failed
    }

    new_node->fs = fs;
    new_node->next = __fs_mapping;
    __fs_mapping = new_node;

    return VF_SUCCESS; // Mounted successfully
}

vfs_result_t vfs_unmount(const struct file_system *fs) {
    if (!fs) {
        return VF_INVALID; // Invalid parameters
    }

    for (struct __fs_hlist **cur = &__fs_mapping; *cur; cur = &(*cur)->next) {
        if ((*cur)->fs == fs) {
            void *to_delete = (void *) *cur;
            *cur = (*cur)->next;
            vPortFree(to_delete);
            return VF_SUCCESS; // Unmounted successfully
        }
    }

    return VF_NOT_FOUND; // File system not found
}

vfs_result_t vfs_normalize(char *target) {
    if (!__valid_path(target)) {
        return VF_INVALID; // Invalid parameters
    }

    char *dst = target; // Destination pointer for writing the normalized path
    char *src = target; // Source pointer for reading the original path
    while (*src) {
        // Skip redundant slashes
        while (*src == '/') {
            src++;
        }

        // Copy the next path component
        while (*src && *src != '/') {
            *dst++ = *src++;
        }

        // Add a single slash if there are more components to process
        if (*src) {
            *dst++ = '/';
        }
    }
    
    if (src != target && *(src - 1) == '/') {
        dst--; // Remove trailing slash
    }

    *dst = '\0'; // Null-terminate the normalized path
    return VF_SUCCESS; // Normalized successfully
}

const struct file_system *vfs_lookup(const char *path) {
    if (!__valid_path(path)) {
        return NULL; // Invalid path
    }
    
    for (struct __fs_hlist *current = __fs_mapping; current; current = current->next) {
        if (strncmp(path, current->fs->mount_point, strlen(current->fs->mount_point)) == 0 &&
            (path[strlen(current->fs->mount_point)] == '/' || path[strlen(current->fs->mount_point)] == '\0')) {
            return current->fs; // Found the mounted file system
        }
    }

    return NULL; // Not found
}

vfs_result_t vfs_dir_emit(const char *src, char *dst, size_t dst_max_len, size_t *reclen) {
    if (!src || !dst || !reclen || dst_max_len <= 0 || *reclen > strlen(src)) {
        return VF_INVALID; // Invalid parameters
    }

    const char *src_begin = src + strlen(src) - *reclen;
    size_t emit_len = *reclen < (dst_max_len - 1) ? *reclen : dst_max_len - 1;
    strncpy(dst, src_begin, emit_len);
    dst[emit_len] = '\0'; // Null-terminate the emitted entry name
    return VF_SUCCESS;
}

vf_result_t vf_open(struct file *fp, const char *path, unsigned int flags) {
    if (!fp || !path) {
        return VF_INVALID; // Invalid parameters
    }

    char *normalized_path = (char *)pvPortMalloc(strlen(path) + 1);
    if (!normalized_path) {
        return VF_ERROR; // Memory allocation failed
    }

    strcpy(normalized_path, path);
    int ret = vfs_normalize(normalized_path);
    if (ret != VF_SUCCESS) {
        vPortFree(normalized_path);
        return ret; // Normalization failed
    }

    const struct file_system *mounted = vfs_lookup(normalized_path);
    if (!mounted) {
        vPortFree(normalized_path);
        return VF_NOT_FOUND; // Not found
    }

    fp->f_flags = flags;
    fp->fs = mounted;

    if (mounted->fops && mounted->fops->open) {
        ret = mounted->fops->open(fp, normalized_path);
        vPortFree(normalized_path);
        return ret;
    }

    vPortFree(normalized_path);
    return VF_SUCCESS; // Opened successfully
}

vf_result_t vf_close(struct file *fp) {
    if (!fp) {
        return VF_ERROR; // Invalid file
    }

    int result = VF_SUCCESS;
    if (fp->fs->fops && fp->fs->fops->close) {
        result = fp->fs->fops->close(fp);
    }

    fp->fs = NULL; // Clear the file system reference
    return result; // Closed successfully
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

poll_t vf_poll(struct file *file, poll_t events, int timeout_ms) {
    if (!file) {
        return POLLNVAL; // Invalid file
    }

    if (!file->fs->fops || !file->fs->fops->poll) {
        return POLLERR; // No poll operation defined
    }

    if ((events & POLLHUP) && file->fs->mount_point == NULL) {
        return POLLHUP | POLLERR; // Hang up and error if the file system is unmounted
    }

    return file->fs->fops->poll(file, events, timeout_ms);
}

loff_t vf_readdir (struct file *file, char *path, size_t path_max_len) {
    if (!file) {
        return (loff_t) VF_ERROR; // Invalid file
    }

    if (!file->fs->fops || !file->fs->fops->iterate_shared) {
        return (loff_t) VF_INVALID; // No readdir operation defined
    }

    return file->fs->fops->iterate_shared(file, path, path_max_len);
}

vf_result_t vf_mkdir(const char *path, const char *name, umode_t mode) {
    if (!path) {
        return VF_INVALID; // Invalid parameters
    }

    char *normalized_path = (char *)pvPortMalloc(strlen(path) + 1);
    if (!normalized_path) {
        return VF_ERROR; // Memory allocation failed
    }

    strcpy(normalized_path, path);
    int ret = vfs_normalize(normalized_path);
    if (ret != VF_SUCCESS) {
        vPortFree(normalized_path);
        return ret; // Normalization failed
    }

    const struct file_system *mounted = vfs_lookup(normalized_path);
    if (!mounted) {
        vPortFree(normalized_path);
        return VF_NOT_FOUND; // Not found
    }

    if (!mounted->nops || !mounted->nops->create) {
        vPortFree(normalized_path);
        return VF_INVALID; // No mkdir operation defined
    }

    ret = mounted->nops->create(mounted, normalized_path, name, mode);
    vPortFree(normalized_path);
    return ret;
}

vf_result_t vf_unlink(const char *path) {
    if (!path) {
        return VF_INVALID; // Invalid parameters
    }
    
    char *normalized_path = (char *)pvPortMalloc(strlen(path) + 1);
    if (!normalized_path) {
        return VF_ERROR; // Memory allocation failed
    }

    strcpy(normalized_path, path);
    int ret = vfs_normalize(normalized_path);
    if (ret != VF_SUCCESS) {
        vPortFree(normalized_path);
        return ret; // Normalization failed
    }

    const struct file_system *mounted = vfs_lookup(normalized_path);
    if (!mounted) {
        vPortFree(normalized_path);
        return VF_NOT_FOUND; // Not found
    }

    if (!mounted->nops || !mounted->nops->unlink) {
        vPortFree(normalized_path);
        return VF_INVALID; // No unlink operation defined
    }

    ret = mounted->nops->unlink(mounted, normalized_path);
    vPortFree(normalized_path);
    return ret;
}

vf_result_t vf_rmdir(const char *path) {
    if (!path) {
        return VF_INVALID; // Invalid parameters
    }

    char *normalized_path = (char *)pvPortMalloc(strlen(path) + 1);
    if (!normalized_path) {
        return VF_ERROR; // Memory allocation failed
    }

    strcpy(normalized_path, path);
    int ret = vfs_normalize(normalized_path);
    if (ret != VF_SUCCESS) {
        vPortFree(normalized_path);
        return ret; // Normalization failed
    }

    const struct file_system *mounted = vfs_lookup(normalized_path);
    if (!mounted) {
        vPortFree(normalized_path);
        return VF_NOT_FOUND; // Not found
    }

    ret = mounted->nops->rmdir(mounted, normalized_path);
    vPortFree(normalized_path);
    return ret;
}
