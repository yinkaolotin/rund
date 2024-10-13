#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <signal.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include "runtime.h"
#include "helpers.h"


#define CONTAINER_STACK_SIZE (1024 * 1024)
#define CONTAINER_FLAG_INIT_PROCESS_EXISTS (1 << 0)


static char container_process_stack[CONTAINER_STACK_SIZE];

static volatile sig_atomic_t container_init_process_exists = 0;

static void handle_signals(int signo)
{
    if (signo == SIGUSR1) {
        printf("Received SIGUSR1, stopping container execution.\n");
        raise(SIGSTOP);
    } else if (signo == SIGTERM) {
        printf("Received SIGTERM, terminating container execution.\n");
        raise(SIGKILL);
    } else {
        printf("Received SIGCONT, resuming container execution.\n");
    }
}

static void runtime_register_signal_handlers() {
    signal(SIGCONT, handle_signals);
    signal(SIGUSR1, handle_signals);
    signal(SIGTERM, handle_signals);
}

void runtime_container_store(runtime_container_info_t **store, runtime_container_info_t *c)
{
    c->next = *store;
    *store = c;
}

void runtime_containers_clearall(runtime_container_info_t *store)
{
    runtime_container_info_t *h = store;
    while (h != NULL) {
        runtime_container_info_t *nxt = h->next;
        free(h);
        h = nxt;
    }
}

void runtime_container_remove(runtime_container_info_t **store, pid_t c)
{
    runtime_container_info_t *cur = *(store);
    runtime_container_info_t *prev = NULL;
    while(cur != NULL) {
        if (cur->pid == c) {
            if (prev == NULL) {
                *(store) = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

runtime_container_info_t *runtime_container_retrieve(runtime_container_info_t *store, void *key, key_type_t key_t)
{
    while(store != NULL) {
        switch (key_t) {
            case TYPE_CHAR_PTR:
                if (str_equal(store->name, (char*)key)) {
                    return (store);
                }
            case TYPE_PID_T:
                if (store->pid == *((pid_t*)key)) {
                    return (store);
                }
        }
        store = store->next;
    }
    return NULL;
}

static int container_create(void *args)
{
    char **argv;
    runtime_container_arguments_t *a = args;

    argv = a->argv;
    prctl(PR_SET_NAME, argv[0], 0, 0, 0);

    if (mount("proc", "/proc", "proc", 0, "") != 0) {
        print_e("Failed to mount /proc in container.");
        return 1;
    }

    if (sethostname(argv[0], strlen(argv[0])) != 0) {
        print_e("Failed to set hostname in container.");
        return 1;
    }

    close(a->pipe[1]);

    if (execvp(a->argv[1], &a->argv[1]) == -1)
        PRINT_E_CONTINUE(ERROR_START_INIT_PROCESS);

    runtime_register_signal_handlers();
    printf("Started init process....\n");
    // Main loop for handling messages from the runtime in the container
    for (;;) {
        char **command;
        char msg[100];

        ssize_t rd = read(a->pipe[0], msg, sizeof(msg));
        if (rd <= 0)
            PRINT_E_CONTINUE(ERROR_READ_FROM_RUNTIME);

        char *argv[50] = {NULL};
        str_to_argv(msg, argv, 50);

        if (a->op_type == RUNTIME_CONTAINER_EXEC) {
            switch (fork()) {
                case -1:
                    PRINT_E_CONTINUE(ERROR_EXEC_COMMAND);
                case 0:
                    if (execvp(a->argv[1], &a->argv[1]) == -1)
                        PRINT_E_CONTINUE(ERROR_START_INIT_PROCESS);
                        break;
                default:
                    break;            
            }
        }
        int status;
        pid_t pid;
        while((pid = waitpid(RUNTIME_WAIT_ANY_CONTAINER, &(status), WNOHANG) > 0)) {
            if (WIFEXITED(status)) {
                printf("Executed command exited with status %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Executed command terminated by signal %d\n", WTERMSIG(status));
            }
        }
        
    }
    return 0;
}

static int runtime_container_setargopt(runtime_container_arguments_t *a, char *op)
{
        if (str_equal(op, RUNTIME_CONTAINER_CREATE_STR)) {
            a->op_type = RUNTIME_CONTAINER_CREATE;
        } else if (str_equal(op, RUNTIME_CONTAINER_EXEC_STR)) {
            a->op_type = RUNTIME_CONTAINER_EXEC;
        } else if (str_equal(op, RUNTIME_CONTAINER_TERMINATE_STR)) {
            a->op_type = RUNTIME_CONTAINER_TERMINATE;
        } else if (str_equal(op, RUNTIME_CONTAINER_PAUSE_STR)) {
            a->op_type = RUNTIME_CONTAINER_PAUSE;
        } else {
            print_e(ERROR_UNRECOGNIZED_OP);
            return -1;
        }
    return 0;
}

int runtime_container_setup(runtime_container_info_t *c, runtime_container_arguments_t *a, int flags, char *argv[]) {
    int runtime_to_container_pipe[2];
    if (pipe(runtime_to_container_pipe) == -1)
        return -1;

    c->name = argv[2];
    a->argv = &argv[2];
    a->pipe = runtime_to_container_pipe;
    runtime_container_setargopt(a, argv[1]);
    c->args = a;

    pid_t p = clone(container_create, container_process_stack + CONTAINER_STACK_SIZE, flags, a );
    if (p < 0) {
        return -2;
    }

    c->pid = p;
    return 0;
}

int runtime_container_notify(runtime_container_info_t store, runtime_container_info_t c)
{
    runtime_container_info_t *rc;
    rc = runtime_container_retrieve(&store, c.name, TYPE_CHAR_PTR); 
    if (rc && (c.args->op_type == RUNTIME_CONTAINER_TERMINATE)) {
        printf("Terminating container %s...", rc->name);
        kill(rc->pid, SIGTERM);
    } else {
        return ERROR_NONEXISTENT_CONTAINER;
    }

    if (rc && (c.args->op_type == RUNTIME_CONTAINER_PAUSE)) {
        printf("Pausing container %s...", rc->name);
        kill(rc->pid, SIGUSR1);
    } else {
        return ERROR_NONEXISTENT_CONTAINER;
    }

    if (rc && (c.args->op_type == RUNTIME_CONTAINER_RESUME)) {
        printf("Resuming container %s...", rc->name);
        kill(rc->pid, SIGCONT);
    } else {
        return ERROR_NONEXISTENT_CONTAINER;
    }

    return ERROR_NONE;
}
