#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "log.h"
#include "parser.h"

static int run_exit_builtin(char **argv, sh_env *shell_env) {
    (void) argv; /* no args for now */

    if (shell_env->subshell)
        _exit(EXIT_SUCCESS);
    else {
        printf("exit\n");
        exit(EXIT_SUCCESS);
    }

    return EXIT_FAILURE;
}

static int run_cd_builtin(char **argv, sh_env *shell_env) {
    (void) shell_env;

    if (!argv || !argv[0]) {
        LOG_ERR("builtin cd received invalid argv structure");
        fprintf(stderr, "cd: internal error check logs\n");
        return EXIT_FAILURE;
    }

    if (!argv[1]) {
        fprintf(stderr, "cd: path required\n");
        return EXIT_FAILURE;
    }

    if (argv[2]) {
        fprintf(stderr, "cd: too many arguments\n");
        return EXIT_FAILURE;
    }

    if (chdir(argv[1]) == -1) {
        fprintf(stderr, "cd: chdir: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool verify_var_key(const char *key) {
    for (const char *c = key; *c != '\0'; ++c) {
        if (!((*c >= 'a' && *c <= 'z') ||
            (*c >= 'A' && *c <= 'Z') ||
            // (*c >= '0' && *c <= '9') ||
            *c == '-' || *c == '_'))
            return false;
    }

    return true;
}

static int run_set_builtin(char **argv, sh_env *shell_env) {
    (void) shell_env;

    if (!argv || !argv[0]) {
        LOG_ERR("builtin set received invalid argv structure");
        fprintf(stderr, "set: internal error check logs\n");
        return EXIT_FAILURE;
    }

    if (!argv[1] || !argv[2]) {
        fprintf(stderr, "set: not enough arguments\n");
        return EXIT_FAILURE;
    }

    if (argv[3]) {
        fprintf(stderr, "set: too many arguments\n");
        return EXIT_FAILURE;
    }

    var_pair var = {0};

    if (strlen(argv[1]) >= SHELL_VAR_MAX) {
        fprintf(stderr, "set: variable name too long: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (strlen(argv[2]) >= SHELL_VAR_MAX) {
        fprintf(stderr, "set: variable value too long: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    if (!verify_var_key(argv[1])) {
        fprintf(stderr, "set: invald variable name: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    strcpy(var.key, argv[1]);
    strcpy(var.value, argv[2]);

    if (st_add_var(&var) == -1) {
        fprintf(stderr, "set: failed to add variable\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_unset_builtin(char **argv, sh_env *shell_env) {
    (void) shell_env;

    if (!argv || !argv[0]) {
        LOG_ERR("builtin set received invalid argv structure");
        fprintf(stderr, "unset: internal error check logs\n");
        return EXIT_FAILURE;
    }

    if (!argv[1]) {
        fprintf(stderr, "unset: not enough arguments\n");
        return EXIT_FAILURE;
    }

    if (argv[2]) {
        fprintf(stderr, "unset: too many arguments\n");
        return EXIT_FAILURE;
    }

    if (strlen(argv[1]) >= SHELL_VAR_MAX) {
        fprintf(stderr, "set: variable name too long: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    st_delete_var(argv[1]);

    return EXIT_SUCCESS;
}

static sh_builtin builtins[BUILTIN_COUNT] = {
    { .name = "exit", .func = run_exit_builtin },
    { .name = "cd", .func = run_cd_builtin },
    { .name = "set", .func = run_set_builtin },
    { .name = "unset", .func = run_unset_builtin }
};

static sh_builtin *get_builtin(const char *arg) {
    const char *name = arg;

    for (size_t i = 0; i < BUILTIN_COUNT; ++i)
        if (strcmp(builtins[i].name, name) == 0)
            return &builtins[i];

    return NULL;
}

bool try_run_builtin(char **argv, int *status) {
    sh_builtin *builtin = get_builtin(argv[0]);
    if (builtin) {
        *status = builtin->func(argv, &shell_env);
        return true;
    }

    return false;
}
