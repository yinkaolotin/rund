#ifndef RUNTIME_H
#define RUNTIME_H

#include <sys/msg.h>
#include <sys/wait.h>

#define MAX_CONTAINERS 3

#define RUNTIME_CONTAINER_CREATE_STR "create"
#define RUNTIME_CONTAINER_EXEC_STR "exec"
#define RUNTIME_CONTAINER_TERMINATE_STR "terminate"
#define RUNTIME_CONTAINER_PAUSE_STR "pause"
#define RUNTIME_CONTAINER_RESUME_STR "resume"

#define RUNTIME_WAIT_ANY_CONTAINER -1

#define ERROR_NONE 0 

#define ERROR_MAX_CONTAINERS_STR "Limit of creatable containers reached."
#define ERROR_CONTAINER_SETUP_STR  "Failed to setup container environment."
#define ERROR_CONTAINER_RECREATE_STR "The specified container already exists."
#define ERROR_NONEXISTENT_CONTAINER_STR "The specified container does not exist. Create it first."

#define ERROR_READ_FROM_RUNTIME "Failed to read message from runtime."
#define ERROR_START_INIT_PROCESS "Failed to start init process."
#define ERROR_EXEC_COMMAND "Failed to execute container command."

#define ERROR_UNRECOGNIZED_OP "Unrecognized operation."

#define ERROR_NONEXISTENT_CONTAINER -1

typedef enum {
    RUNTIME_CONTAINER_CREATE,
    RUNTIME_CONTAINER_EXEC,
    RUNTIME_CONTAINER_TERMINATE,
    RUNTIME_CONTAINER_PAUSE,
    RUNTIME_CONTAINER_RESUME
} runtime_container_operation_t;

typedef struct {
    char **argv;
    int *pipe;
    key_t *queue;
    runtime_container_operation_t op_type;
} runtime_container_arguments_t;

typedef struct runtime_container_info runtime_container_info_t;
struct runtime_container_info {
    char *name;
    pid_t pid;
    int status;
    runtime_container_arguments_t *args;
    runtime_container_info_t *next;
};

typedef enum {
    TYPE_PID_T,
    TYPE_CHAR_PTR
} key_type_t;

int runtime_container_setup(runtime_container_info_t *c, runtime_container_arguments_t *a, int flags, char *argv[]);
int runtime_container_notify(runtime_container_info_t store, runtime_container_info_t c);
void runtime_container_store(runtime_container_info_t **store, runtime_container_info_t *c);
void runtime_containers_clearall(runtime_container_info_t *store);
runtime_container_info_t *runtime_container_retrieve(runtime_container_info_t *store, void *key, key_type_t key_t);
void runtime_container_remove(runtime_container_info_t **store, pid_t c);

#endif
