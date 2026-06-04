#ifndef VFS_UART_TTY_H
#define VFS_UART_TTY_H

#include "vfs.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// A minimal VFS-backed TTY for USART2
struct uart2_tty_fs {
    struct file_system base;

    /* RX bytes pushed from ISR (HAL_UART_RxCpltCallback). */
    StreamBufferHandle_t rx_stream;

    /* Serializes TX (HAL_UART_Transmit). */
    SemaphoreHandle_t tx_mutex;
};

int mount_uart2_tty(struct uart2_tty_fs *fs, const char *mount_point, size_t rx_stream_size);
int unmount_uart2_tty(struct uart2_tty_fs *fs);

#ifdef __cplusplus
}
#endif

#endif /* VFS_UART_TTY_H */
