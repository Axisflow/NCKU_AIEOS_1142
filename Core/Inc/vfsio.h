#ifndef VFSIO_H
#define VFSIO_H

#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

vf_result_t vf_fprintf(struct file *fp, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* VFSIO_H */
