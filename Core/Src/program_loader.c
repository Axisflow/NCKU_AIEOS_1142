#include "program_loader.h"
#include "vfs_fatfs.h"
#include "script_interpreter.h"

#include <stdio.h>

#define PROGRAM_SCRIPT_BUFFER_SIZE 4096

int ProgramLoader_RunScript(const char *path)
{
    struct file file;
    vf_result_t res;
    __vf_ssize_t bytes_read = 0;
    static char script_buffer[PROGRAM_SCRIPT_BUFFER_SIZE];

    printf("Loading script: %s\r\n", path);

    res = vf_open(&file, path, FA_READ);

    if (res != FR_OK) {
        printf("vf_open script failed, res = %d\r\n", res);
        return -1;
    }

    bytes_read = vf_read(&file, script_buffer, sizeof(script_buffer) - 1);

    vf_close(&file);

    if (bytes_read < 0) {
        printf("vf_read script failed, res = %d\r\n", res);
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