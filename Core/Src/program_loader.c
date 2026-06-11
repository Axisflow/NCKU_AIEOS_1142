#include "program_loader.h"
#include "fatfs.h"
#include "script_interpreter.h"

#include <stdio.h>

#define PROGRAM_SCRIPT_BUFFER_SIZE 4096

int ProgramLoader_RunScript(const char *path)
{
    FIL file;
    FRESULT res;
    UINT bytes_read = 0;
    static char script_buffer[PROGRAM_SCRIPT_BUFFER_SIZE];

    printf("Loading script: %s\r\n", path);

    res = f_open(&file, path, FA_READ);

    if (res != FR_OK) {
        printf("f_open script failed, res = %d\r\n", res);
        return -1;
    }

    res = f_read(&file, script_buffer, sizeof(script_buffer) - 1, &bytes_read);

    f_close(&file);

    if (res != FR_OK) {
        printf("f_read script failed, res = %d\r\n", res);
        return -1;
    }

    script_buffer[bytes_read] = '\0';

    printf("Script loaded: %lu bytes\r\n", (unsigned long)bytes_read);

    int ret = ScriptInterpreter_RunBuffer(path, script_buffer, (int)bytes_read);

    if (ret == 0) {
        printf("Script finished: %s\r\n", path);
    } else {
        printf("Script failed: %s\r\n", path);
    }

    return ret;
}