#include "sd_card.h"
#include "fatfs.h"
#include <stdio.h>

int SDCard_Mount(void)
{
    printf("Mounting SD card...\r\n");

    FRESULT res = f_mount(&USERFatFS, USERPath, 1);

    if (res == FR_OK)
    {
        printf("SD mount success\r\n");
        return 0;
    }

    printf("SD mount failed, res = %d\r\n", res);
    return -1;
}