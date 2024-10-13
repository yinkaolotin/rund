#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "runtime.h"
#include "helpers.h"

int container_count;


int main(int argc, char**argv)
{
    int err;
    runtime_container_info_t *store = NULL;

    // skip IPC namespace for runtime-container communication
    int flags = CLONE_NEWNET | CLONE_NEWUTS | CLONE_NEWNS |
        CLONE_NEWPID | CLONE_NEWUSER | SIGCHLD;

    for (;;) {
        char *argv[MAX_COMMAND_LEN] = {NULL};
        runtime_container_arguments_t container_args;
        runtime_container_info_t *container_ptr = malloc(sizeof(runtime_container_info_t));

        if ((err = read_and_parse_input(argv, MAX_COMMAND_LEN)) != 0)
            PRINT_MULTIPLE_E_CONTINUE(err, 3);
        
        if (str_equal(argv[1], RUNTIME_CONTAINER_CREATE_STR)) {
            if (int_equal(container_count,MAX_CONTAINERS))
                PRINT_E_CONTINUE(ERROR_MAX_CONTAINERS_STR);

                if (runtime_container_retrieve(store, argv[2], TYPE_CHAR_PTR))
                PRINT_E_CONTINUE(ERROR_CONTAINER_RECREATE_STR);

            if (runtime_container_setup(container_ptr,&container_args, flags, argv) < 0)
                PRINT_E_CONTINUE(ERROR_CONTAINER_SETUP_STR);

            runtime_container_store(&store, container_ptr);
            container_count++;
        } else {
            err = runtime_container_notify(*(store), *(container_ptr));
            if (error_is(err, ERROR_NONEXISTENT_CONTAINER))
                PRINT_E_CONTINUE(ERROR_NONEXISTENT_CONTAINER_STR);
        }

        pid_t pid;
        int status;
        while ((pid = waitpid(RUNTIME_WAIT_ANY_CONTAINER, &(status), WNOHANG)) > 0) {
            runtime_container_info_t *rc = runtime_container_retrieve(store, &(pid), TYPE_PID_T);
            if (WIFEXITED(status)) {
                printf("Container exited with status %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Container terminated by signal %d\n", WTERMSIG(status));
            }
            runtime_container_remove(&(store), pid);
            container_count--;
        }
    } 
    return EXIT_SUCCESS;
}

