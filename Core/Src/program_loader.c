#include "program_loader.h"
#include "fatfs.h"
#include "script_interpreter.h"

#include <stdio.h>

int ProgramLoader_RunScript(const char *path)
{
    FIL file;
    FRESULT res;
    char line[128];

    printf("Running script: %s\r\n", path);

    res = f_open(&file, path, FA_READ);

    if (res != FR_OK)
    {
        printf("f_open script failed, res = %d\r\n", res);
        return -1;
    }

    while (f_gets(line, sizeof(line), &file) != NULL)
    {
        ScriptInterpreter_ExecuteLine(line);
    }

    f_close(&file);

    printf("Script finished: %s\r\n", path);

    return 0;
}