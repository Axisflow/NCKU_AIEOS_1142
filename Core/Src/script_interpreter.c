#include "script_interpreter.h"
#include "vfs.h"
#include "main.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#define MAX_SCRIPT_SIZE 4096
#define MAX_SCRIPT_LINES 128
#define MAX_LINE_LEN 128
#define MAX_VARS 16

typedef struct {
    char name[16];
    int value;
    int used;
} script_var_t;

static char g_script_buffer[MAX_SCRIPT_SIZE];
static char g_lines[MAX_SCRIPT_LINES][MAX_LINE_LEN];
static int g_line_count = 0;

static script_var_t g_vars[MAX_VARS];

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

static int starts_with_word(const char *line, const char *word)
{
    size_t len = strlen(word);

    if (strncmp(line, word, len) != 0) {
        return 0;
    }

    if (line[len] == '\0' || line[len] == ' ' || line[len] == '\t') {
        return 1;
    }

    return 0;
}

static int is_number(const char *s)
{
    if (s == NULL || *s == '\0') {
        return 0;
    }

    if (*s == '-' || *s == '+') {
        s++;
    }

    if (*s == '\0') {
        return 0;
    }

    while (*s) {
        if (*s < '0' || *s > '9') {
            return 0;
        }
        s++;
    }

    return 1;
}

static int set_var(const char *name, int value)
{
    for (int i = 0; i < MAX_VARS; i++) {
        if (g_vars[i].used && strcmp(g_vars[i].name, name) == 0) {
            g_vars[i].value = value;
            return 0;
        }
    }

    for (int i = 0; i < MAX_VARS; i++) {
        if (!g_vars[i].used) {
            g_vars[i].used = 1;
            strncpy(g_vars[i].name, name, sizeof(g_vars[i].name) - 1);
            g_vars[i].name[sizeof(g_vars[i].name) - 1] = '\0';
            g_vars[i].value = value;
            return 0;
        }
    }

    printf("Variable table full\r\n");
    return -1;
}

static int get_var(const char *name, int *value)
{
    for (int i = 0; i < MAX_VARS; i++) {
        if (g_vars[i].used && strcmp(g_vars[i].name, name) == 0) {
            *value = g_vars[i].value;
            return 0;
        }
    }

    printf("Variable not found: %s\r\n", name);
    return -1;
}

static int get_value(const char *token, int *value)
{
    if (is_number(token)) {
        *value = atoi(token);
        return 0;
    }

    return get_var(token, value);
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

    struct file file = {0};

    vf_result_t open_result = vf_open(&file, path, 0);

    if (open_result != VF_SUCCESS) {
        printf("vf_open failed: %s, result=%d\r\n", path, open_result);
        return -1;
    }

    __vf_ssize_t written = vf_write(&file, value, strlen(value));

    vf_close(&file);

    if (written < 0) {
        printf("vf_write failed: %s\r\n", path);
        return -1;
    }

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

    vTaskDelay(pdMS_TO_TICKS(ms));

    return 0;
}

static int execute_cat(char *line)
{
    char path[64] = {0};
    char buffer[128] = {0};

    int matched = sscanf(line, "cat %63s", path);

    if (matched != 1) {
        printf("cat parse failed: %s\r\n", line);
        return -1;
    }

    printf("Interpreter: cat path=%s\r\n", path);

    struct file file = {0};

    vf_result_t open_result = vf_open(&file, path, 0);

    if (open_result != VF_SUCCESS) {
        printf("vf_open failed: %s, result=%d\r\n", path, open_result);
        return -1;
    }

    __vf_ssize_t n = vf_read(&file, buffer, sizeof(buffer) - 1);

    vf_close(&file);

    if (n < 0) {
        printf("vf_read failed: %s\r\n", path);
        return -1;
    }

    buffer[n] = '\0';

    printf("Sensor data: %s\r\n", buffer);

    return 0;
}

static int execute_read(char *line)
{
    char name[16] = {0};
    char path[64] = {0};
    char buffer[32] = {0};

    int matched = sscanf(line, "read %15s %63s", name, path);

    if (matched != 2) {
        printf("read parse failed: %s\r\n", line);
        return -1;
    }

    struct file file = {0};

    vf_result_t open_result = vf_open(&file, path, 0);

    if (open_result != VF_SUCCESS) {
        printf("vf_open failed: %s, result=%d\r\n", path, open_result);
        return -1;
    }

    __vf_ssize_t n = vf_read(&file, buffer, sizeof(buffer) - 1);

    vf_close(&file);

    if (n < 0) {
        printf("vf_read failed: %s\r\n", path);
        return -1;
    }

    buffer[n] = '\0';
    trim_line(buffer);

    int value = atoi(buffer);

    if (set_var(name, value) != 0) {
        return -1;
    }

    printf("Interpreter: read %s=%d from %s\r\n", name, value, path);

    return 0;
}

static int execute_set(char *line)
{
    char name[16] = {0};
    char value_token[16] = {0};

    int matched = sscanf(line, "set %15s %15s", name, value_token);

    if (matched != 2) {
        printf("set parse failed: %s\r\n", line);
        return -1;
    }

    int value = 0;

    if (get_value(value_token, &value) != 0) {
        return -1;
    }

    if (set_var(name, value) != 0) {
        return -1;
    }

    printf("Interpreter: set %s=%d\r\n", name, value);

    return 0;
}

static int execute_tryread(char *line)
{
    char name[16] = {0};
    char path[64] = {0};
    int default_value = 0;
    char buffer[32] = {0};

    int matched = sscanf(line, "tryread %15s %63s %d", name, path, &default_value);

    if (matched != 3) {
        printf("tryread parse failed: %s\r\n", line);
        return -1;
    }

    struct file file = {0};

    vf_result_t open_result = vf_open(&file, path, 0);

    if (open_result != VF_SUCCESS) {
        printf("tryread: vf_open failed: %s, use default=%d\r\n",
               path,
               default_value);

        set_var(name, default_value);
        return 0;
    }

    __vf_ssize_t n = vf_read(&file, buffer, sizeof(buffer) - 1);

    vf_close(&file);

    if (n < 0) {
        printf("tryread: vf_read failed: %s, use default=%d\r\n",
               path,
               default_value);

        set_var(name, default_value);
        return 0;
    }

    buffer[n] = '\0';
    trim_line(buffer);

    int value = atoi(buffer);

    set_var(name, value);

    printf("Interpreter: tryread %s=%d from %s\r\n", name, value, path);

    return 0;
}

static int execute_print(char *line)
{
    char name[16] = {0};

    int matched = sscanf(line, "print %15s", name);

    if (matched != 1) {
        printf("print parse failed: %s\r\n", line);
        return -1;
    }

    int value = 0;

    if (get_var(name, &value) != 0) {
        return -1;
    }

    printf("%s = %d\r\n", name, value);

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

    if (starts_with_word(line, "echo")) {
        return execute_echo(line);
    }

    if (starts_with_word(line, "sleep")) {
        return execute_sleep(line);
    }

    if (starts_with_word(line, "cat")) {
        return execute_cat(line);
    }

    if (starts_with_word(line, "set")) {
        return execute_set(line);
    }

    if (starts_with_word(line, "tryread")) {
        return execute_tryread(line);
    }

    if (starts_with_word(line, "read")) {
        return execute_read(line);
    }

    if (starts_with_word(line, "print")) {
        return execute_print(line);
    }

    printf("Unknown command: %s\r\n", line);

    return -1;
}

static int eval_condition(char *line, int *result)
{
    char left[16] = {0};
    char op[3] = {0};
    char right[16] = {0};

    int matched = sscanf(line, "if %15s %2s %15s", left, op, right);

    if (matched != 3) {
        printf("if parse failed: %s\r\n", line);
        return -1;
    }

    int left_value = 0;
    int right_value = 0;

    if (get_value(left, &left_value) != 0) {
        return -1;
    }

    if (get_value(right, &right_value) != 0) {
        return -1;
    }

    if (strcmp(op, ">") == 0) {
        *result = left_value > right_value;
    } else if (strcmp(op, ">=") == 0) {
        *result = left_value >= right_value;
    } else if (strcmp(op, "<") == 0) {
        *result = left_value < right_value;
    } else if (strcmp(op, "<=") == 0) {
        *result = left_value <= right_value;
    } else if (strcmp(op, "==") == 0) {
        *result = left_value == right_value;
    } else if (strcmp(op, "!=") == 0) {
        *result = left_value != right_value;
    } else {
        printf("Unknown operator: %s\r\n", op);
        return -1;
    }

    printf("Interpreter: %s %s %s => %s\r\n", left, op, right, *result ? "true" : "false");

    return 0;
}

static int find_if_parts(int if_line, int end, int *else_line, int *endif_line)
{
    int depth = 0;

    *else_line = -1;
    *endif_line = -1;

    for (int i = if_line + 1; i < end; i++) {
        char *line = g_lines[i];

        if (starts_with_word(line, "if")) {
            depth++;
        } else if (strcmp(line, "endif") == 0) {
            if (depth == 0) {
                *endif_line = i;
                return 0;
            } else {
                depth--;
            }
        } else if (strcmp(line, "else") == 0 && depth == 0) {
            *else_line = i;
        }
    }

    printf("endif not found for line %d\r\n", if_line + 1);
    return -1;
}

static int find_endloop(int loop_line, int end)
{
    int depth = 0;

    for (int i = loop_line + 1; i < end; i++) {
        char *line = g_lines[i];

        if (starts_with_word(line, "loop")) {
            depth++;
        } else if (strcmp(line, "endloop") == 0) {
            if (depth == 0) {
                return i;
            } else {
                depth--;
            }
        }
    }

    printf("endloop not found for line %d\r\n", loop_line + 1);
    return -1;
}

static int execute_range(int start, int end)
{
    int i = start;

    while (i < end) {
        char *line = g_lines[i];

        if (line[0] == '\0' || line[0] == '#') {
            i++;
            continue;
        }

        if (starts_with_word(line, "if")) {
            int else_line = -1;
            int endif_line = -1;
            int condition_result = 0;

            if (find_if_parts(i, end, &else_line, &endif_line) != 0) {
                return -1;
            }

            if (eval_condition(line, &condition_result) != 0) {
                return -1;
            }

            if (condition_result) {
                int true_end = (else_line >= 0) ? else_line : endif_line;

                if (execute_range(i + 1, true_end) != 0) {
                    return -1;
                }
            } else {
                if (else_line >= 0) {
                    if (execute_range(else_line + 1, endif_line) != 0) {
                        return -1;
                    }
                }
            }

            i = endif_line + 1;
            continue;
        }

        if (starts_with_word(line, "loop")) {
            int loop_count = 0;

            if (sscanf(line, "loop %d", &loop_count) != 1) {
                printf("loop parse failed: %s\r\n", line);
                return -1;
            }

            int endloop_line = find_endloop(i, end);

            if (endloop_line < 0) {
                return -1;
            }

            if (loop_count == 0) {
                while (1) {
                    if (execute_range(i + 1, endloop_line) != 0) {
                        return -1;
                    }
                }
            } else {
                for (int count = 0; count < loop_count; count++) {
                    if (execute_range(i + 1, endloop_line) != 0) {
                        return -1;
                    }
                }
            }

            i = endloop_line + 1;
            continue;
        }

        if (strcmp(line, "else") == 0 || strcmp(line, "endif") == 0 || strcmp(line, "endloop") == 0) {
            printf("Unexpected block keyword at line %d: %s\r\n", i + 1, line);
            return -1;
        }

        if (ScriptInterpreter_ExecuteLine(line) != 0) {
            printf("Script error at line %d\r\n", i + 1);
            return -1;
        }

        i++;
    }

    return 0;
}

static int parse_and_execute_script(const char *script_name)
{
    memset(g_vars, 0, sizeof(g_vars));
    g_line_count = 0;

    char *p = g_script_buffer;

    while (*p != '\0' && g_line_count < MAX_SCRIPT_LINES) {
        char *start = p;

        while (*p != '\0' && *p != '\n') {
            p++;
        }

        size_t len = p - start;

        if (len > 0 && start[len - 1] == '\r') {
            len--;
        }

        if (len >= MAX_LINE_LEN) {
            len = MAX_LINE_LEN - 1;
        }

        memcpy(g_lines[g_line_count], start, len);
        g_lines[g_line_count][len] = '\0';

        trim_line(g_lines[g_line_count]);

        g_line_count++;

        if (*p == '\n') {
            p++;
        }
    }

    printf("Running script: %s\r\n", script_name);
    printf("Script lines: %d\r\n", g_line_count);

    return execute_range(0, g_line_count);
}

int ScriptInterpreter_RunBuffer(const char *script_name, const char *script, int script_size)
{
    if (script_name == NULL) {
        script_name = "<buffer>";
    }

    if (script == NULL || script_size < 0) {
        printf("ScriptInterpreter_RunBuffer invalid argument\r\n");
        return -1;
    }

    if (script_size >= MAX_SCRIPT_SIZE) {
        printf("Script too large: %d bytes, max=%d\r\n",
               script_size,
               MAX_SCRIPT_SIZE - 1);
        return -1;
    }

    memcpy(g_script_buffer, script, script_size);
    g_script_buffer[script_size] = '\0';

    return parse_and_execute_script(script_name);
}

int ScriptInterpreter_RunScript(const char *path)
{
    struct file file = {0};

    vf_result_t open_result = vf_open(&file, path, 0);

    if (open_result != VF_SUCCESS) {
        printf("vf_open failed: %s, result=%d\r\n", path, open_result);
        return -1;
    }

    __vf_ssize_t n = vf_read(&file, g_script_buffer, sizeof(g_script_buffer) - 1);

    vf_close(&file);

    if (n < 0) {
        printf("vf_read failed: %s\r\n", path);
        return -1;
    }

    g_script_buffer[n] = '\0';

    return parse_and_execute_script(path);
}