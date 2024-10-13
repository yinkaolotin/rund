#include <sched.h>
#include <stdarg.h>
#include "helpers.h"


void print_sys_e(char *e)
{
    fprintf(stderr, "Error %s: %s\n", e, strerror(errno));
}

void print_e(char *e)
{
    fprintf(stderr, "%s\n", e);
}

void print_multiple_e(int err, int n, ...)
{
    va_list args;
    va_start(args, n);
    int index = -(err + 1);
    if (index >= 0 && index < n) {
        for (int i = 0; i <= index; i++) {
            const char *str = va_arg(args, const char*);
            if (i == index) {
                print_e((char*)str);
            }
        }
    }
    va_end(args);
}

int str_to_argv(char *str, char *argv[], int max_args) {
    int i = 0;
    char *token = strtok(str, " ");

    while (token != NULL) {
        if (i >= max_args - 1) {
            fprintf(stderr, "Too many arguments\n");
            return -1;
        }
        argv[i++] = token;
        token = strtok(NULL, " ");
    }
    argv[i] = NULL;
    return i;
}

int str_equal(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

int int_equal(int i1, int i2) {
    return i1 == i2;
}

int error_is(int ev, int e) {
    return ev == e;
}

void argv_to_str(char *argv[], char *buffer, size_t buffer_size) {
    int i = 0;
    buffer[0] = '\0';

    while (argv[i] != NULL) {
        if (strlen(buffer) + strlen(argv[i]) + 1 >= buffer_size) {
            fprintf(stderr, "Buffer too small to concatenate arguments\n");
            return;
        }
        strcat(buffer, argv[i]);

        if (argv[i + 1] != NULL) {
            strcat(buffer, " ");
        }
        i++;
    }
}

void clear_array(void *arr, size_t size) {
    memset(arr, '\0', size);
}

int read_and_parse_input(char *argv[], int max_args) {
    char input[MAX_COMMAND_LEN];

    if (fgets(input, sizeof(input), stdin) == NULL) {
        perror("Error reading input");
        return -1;
    }

    input[strcspn(input, "\n")] = 0;

    int argc = str_to_argv(input, argv, max_args);
    if (argc < 0) {
        return -2;
    }

    if (argv[1] == NULL) {
        return -3;
    }

    return 0;
}

