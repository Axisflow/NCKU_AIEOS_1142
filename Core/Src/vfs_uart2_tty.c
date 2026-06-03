#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "task.h"

#include "vfs_uart2_tty.h"

#include "stm32f4xx_hal.h"
#include "stm32f407xx.h"

#include <string.h>

struct __file {
    uint8_t is_dir;
    loff_t d_off;
};

static int __open(struct file *file, const char *path);
static __vf_ssize_t __read(struct file *file, char *buf, size_t count);
static __vf_ssize_t __write(struct file *file, const char *buf, size_t count);
static loff_t __llseek(struct file *file, loff_t offset, int whence);
static int __fsync(struct file *file, loff_t start, loff_t end, int datasync);
static poll_t __poll(struct file *file, poll_t events, int timeout_ms);
static int __close(struct file *file);

static const struct file_operations __fops = {
    .open = __open,
    .read = __read,
    .write = __write,
    .llseek = __llseek,
    .fsync = __fsync,
    .poll = __poll,
    .iterate_shared = NULL,
    .close = __close,
};

extern UART_HandleTypeDef huart2;

static struct uart2_tty_fs *__fs = NULL;
static uint8_t __uart2_rx_byte;

static void __arm_rx(void)
{
    /* Arm single-byte interrupt reception. */
    (void)HAL_UART_Receive_IT(&huart2, &__uart2_rx_byte, 1);
}

/* Override the weak HAL callback to feed RX bytes into the stream buffer. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == NULL || huart->Instance != USART2) {
        return;
    }

    if (__fs && __fs->rx_stream) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        (void)xStreamBufferSendFromISR(__fs->rx_stream, &__uart2_rx_byte, 1, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken != pdFALSE && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }

    __arm_rx();
}

int mount_uart2_tty(struct uart2_tty_fs *fs, const char *mount_point, size_t rx_stream_size)
{
    if (!fs || !mount_point || rx_stream_size == 0) {
        return VF_INVALID;
    }

    fs->tx_mutex = xSemaphoreCreateMutex();
    if (fs->tx_mutex == NULL) {
        return VF_ERROR;
    }

    fs->rx_stream = xStreamBufferCreate(rx_stream_size, 1);
    if (fs->rx_stream == NULL) {
        vSemaphoreDelete(fs->tx_mutex);
        fs->tx_mutex = NULL;
        return VF_ERROR;
    }

    strncpy(fs->base.name, "DEVFS", sizeof(fs->base.name) - 1);
    fs->base.mount_point = mount_point;
    fs->base.nops = NULL; /* No node operations for a TTY FS */
    fs->base.fops = &__fops;

    __fs = fs;
    __arm_rx();

    return vfs_mount((const struct file_system *)fs);
}

int unmount_uart2_tty(struct uart2_tty_fs *fs)
{
    if (!fs) {
        return VF_INVALID;
    }

    if (__fs == fs) {
        __fs = NULL;
    }

    int res = vfs_unmount((const struct file_system *)fs);
    if (fs->rx_stream) {
        vStreamBufferDelete(fs->rx_stream);
        fs->rx_stream = NULL;
    }
    if (fs->tx_mutex) {
        vSemaphoreDelete(fs->tx_mutex);
        fs->tx_mutex = NULL;
    }
    fs->base.mount_point = NULL;
    return res;
}

static int __open(struct file *file, const char *path)
{
    if (!file || !path || !file->fs || !file->fs->mount_point) {
        return VF_INVALID;
    }

    const size_t mp_len = strlen(file->fs->mount_point);
    if (strncmp(path, file->fs->mount_point, mp_len) != 0) {
        return VF_INVALID;
    }

    const char *rel = path + mp_len;

    struct __file *priv = (struct __file *)pvPortMalloc(sizeof(*priv));
    if (!priv) {
        return VF_ERROR;
    }
    priv->is_dir = 0;
    priv->d_off = 0;

    /* Support opening the mount root as a directory, and "tty2" as the device. */
    if (rel[0] == '\0') {
        priv->is_dir = 1;
    } else if (strcmp(rel, "tty2") == 0) {
        priv->is_dir = 0;
    } else {
        vPortFree(priv);
        return VF_NOT_FOUND;
    }

    file->private_data = priv;
    return VF_SUCCESS;
}

static __vf_ssize_t __read(struct file *file, char *buf, size_t count)
{
    if (!file || !buf || count == 0) {
        return 0;
    }

    struct __file *priv = (struct __file *)file->private_data;
    if (!priv || priv->is_dir) {
        return (__vf_ssize_t)VF_INVALID;
    }

    struct uart2_tty_fs *fs = (struct uart2_tty_fs *)file->fs;
    if (!fs || !fs->rx_stream) {
        return (__vf_ssize_t)VF_ERROR;
    }

    size_t got = xStreamBufferReceive(fs->rx_stream, buf, count, portMAX_DELAY);
    return (__vf_ssize_t)got;
}

static __vf_ssize_t __write(struct file *file, const char *buf, size_t count)
{
    if (!file || !buf) {
        return (__vf_ssize_t)VF_INVALID;
    }

    struct __file *priv = (struct __file *)file->private_data;
    if (!priv || priv->is_dir) {
        return (__vf_ssize_t)VF_INVALID;
    }

    struct uart2_tty_fs *fs = (struct uart2_tty_fs *)file->fs;
    if (!fs || !fs->tx_mutex) {
        return (__vf_ssize_t)VF_ERROR;
    }

    xSemaphoreTake(fs->tx_mutex, portMAX_DELAY);
    HAL_StatusTypeDef st = HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)count, HAL_MAX_DELAY);
    xSemaphoreGive(fs->tx_mutex);

    if (st != HAL_OK) {
        return (__vf_ssize_t)VF_ERROR;
    }
    return (__vf_ssize_t)count;
}

static loff_t __llseek(struct file *file, loff_t offset, int whence)
{
    return (loff_t)VF_INVALID;
}

static int __fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
    return VF_SUCCESS;
}

static poll_t __poll(struct file *file, poll_t events, int timeout_ms)
{
    (void)timeout_ms;
    if (!file) {
        return POLLNVAL;
    }

    struct __file *priv = (struct __file *)file->private_data;
    if (!priv || priv->is_dir) {
        return POLLNVAL;
    }

    struct uart2_tty_fs *fs = (struct uart2_tty_fs *)file->fs;
    if (!fs || !fs->rx_stream) {
        return POLLERR;
    }

    poll_t revents = 0;
    if (events & POLLIN) {
        while (xStreamBufferIsEmpty(fs->rx_stream)) {
            vTaskDelay(pdMS_TO_TICKS(25));
        }
        
        revents |= POLLIN;
    }

    if (events & POLLOUT) {
        revents |= POLLOUT;
    }
    
    return revents;
}

static int __close(struct file *file)
{
    if (!file) {
        return VF_INVALID;
    }

    if (file->private_data) {
        vPortFree(file->private_data);
        file->private_data = NULL;
    }
    return VF_SUCCESS;
}
