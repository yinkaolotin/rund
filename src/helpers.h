#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_COMMAND_LEN 100

#define READ_FAILURE "Failed to read input"
#define TOO_MANY_ARGS_FAILURE "Failed to parse input due to too many arguments"
#define NO_COMMANDS_FAILURE "No command specified. Usage:"


#define STR_EQUAL(s1, s2) (strcmp((s1), (s2)) == 0)
#define UINT_EQUAL(ui1, ui2) (i1 == i1)
#define ERROR_IS(e, s) (e == s)

#define ARGV_TO_STR(argv, buffer) do {       \
    int i = 0;                                  \
    buffer[0] = '\0';                           \
    while (argv[i] != NULL) {                   \
        strcat(buffer, argv[i]);                \
        if (argv[i + 1] != NULL)                \
            strcat(buffer, " ");                \
        i++;                                    \
    }                                           \
} while (0)


#define STR_TO_ARGV(str, argv) do {          \
    int i = 0;                                  \
    char *token = strtok(str, " ");             \
    while (token != NULL) {                     \
        argv[i++] = token;                      \
        token = strtok(NULL, " ");              \
    }                                           \
    argv[i] = NULL;                             \
} while (0)

#define PRINT_E_CONTINUE(e) do { \
    print_e(e); \
    continue; \
} while (0)

#define PRINT_MULTIPLE_E_CONTINUE(ev, ne) do { \
    print_multiple_e(ev, ne, READ_FAILURE, TOO_MANY_ARGS_FAILURE, NO_COMMANDS_FAILURE); \
    continue; \
} while (0)

int int_equal(int i1, int i2);
int error_is(int ev, int e);
int str_equal(const char *s1, const char *s2);
int str_to_argv(char *str, char *argv[], int max_args);
void argv_to_str(char *argv[], char *buffer, size_t buffer_size);
void clear_array(void *arr, size_t size);
void print_e(char *e);
void print_sys_e(char *e);
void print_multiple_e(int err, int n, ...);
int read_and_parse_input(char *argv[], int max_args);

#endif
