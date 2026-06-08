#include "vfsio.h"
#include "FreeRTOS.h"

#include <stdarg.h>
#include <stdio.h>

vf_result_t vf_fprintf(struct file *fp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args); // dry run
    va_end(args);

    if (len < 0) return VF_INVALID; // Encoding error

    char *buffer = (char *)pvPortMalloc(len + 1);
    if (!buffer) return VF_ERROR; // Memory allocation failed

    va_start(args, fmt);
    vsnprintf(buffer, len + 1, fmt, args);
    va_end(args);

    int rst = vf_write(fp, buffer, (size_t)len);
    vPortFree(buffer);
    return rst;
}
