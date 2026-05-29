#include "script_interpreter.h"
#include "vfs.h"
#include "main.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void trim_line(char *line)
{
    if (line == NULL) {
        return;
    }

    size_t len = strlen(line);

    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n' || line[len - 1] == ' '  || line[len - 1] == '\t')) {
        line[len - 1] = '\0';
        len--;
    }

    char *start = line;

    while (*start == ' ' || *start == '\t') {
        start++;
    }

    if (start != line) {
        memmove(line, start, strlen(start) + 1);
    }
}

static int execute_echo(char *line)
{
    char value[32] = {0};
    char path[64] = {0};

    int matched = sscanf(line, "echo %31s > %63s", value, path);

    if (matched != 2) {
        printf("echo parse failed: %s\r\n", line);
        return -1;
    }

    printf("Interpreter: echo value=%s path=%s\r\n", value, path);

    // struct file file = {0};

    // vf_result_t open_result = vf_open(&file, path, 0);

    // if (open_result != VF_SUCCESS) {
    //     printf("vf_open failed: %s, result=%d\r\n", path, open_result);
    //     return -1;
    // }

    // ssize_t written = vf_write(&file, value, strlen(value));

    // vf_close(&file);

    // if (written < 0) {
    //     printf("vf_write failed: %s\r\n", path);
    //     return -1;
    // }

    return 0;
}

static int execute_sleep(char *line)
{
    int ms = 0;

    int matched = sscanf(line, "sleep %d", &ms);

    if (matched != 1) {
        printf("sleep parse failed: %s\r\n", line);
        return -1;
    }

    printf("Interpreter: sleep %d ms\r\n", ms);

    // HAL_Delay(ms);

    return 0;
}

int ScriptInterpreter_ExecuteLine(char *line)
{
    if (line == NULL) {
        return -1;
    }

    trim_line(line);

    if (line[0] == '\0') {
        return 0;
    }

    if (line[0] == '#') {
        return 0;
    }

    printf("Execute line: %s\r\n", line);

    if (strncmp(line, "echo ", 5) == 0) {
        return execute_echo(line);
    }

    if (strncmp(line, "sleep ", 6) == 0) {
        return execute_sleep(line);
    }

    printf("Unknown command: %s\r\n", line);

    return -1;
}