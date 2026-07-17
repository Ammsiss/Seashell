#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variable.h"
#include "builtins.h"
#include "log.h"
#include "parser.h"
#include "utils.h"

/*
Job control builtins:
    bg -> cotinues a suspended bg job (without bringing it to the fg)
    fg -> brings a bg job to the fg and then continues it
    kill -> signal a job
*/

static bool validate_argc(char **argv, size_t min_argc, size_t max_argc) {
    if (!argv || !argv[0]) {
        LOG_ERR("validate_args: bad argv array");
        fatal("internal error; check logs");
    }

    size_t arg_n = 0;
    for (char **i = argv + 1; *i != NULL; ++i)
        ++arg_n;

    if (arg_n < min_argc) {
        fprintf(stderr, "%s: not enough arguments\n", argv[0]);
        return false;
    }

    if (arg_n > max_argc) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        return false;
    }

    return true;
}

static int run_exit_builtin(char **argv, shell_env *sh_env) {
    if (!validate_argc(argv, 0, 1))
        return EXIT_FAILURE;

    int exit_status = EXIT_FAILURE;

    if (argv[1]) {
        char *endptr;
        exit_status = strtol(argv[1], &endptr, 10);

        if (strcmp(argv[1], "") == 0 || *endptr != '\0') {
            fprintf(stderr, "exit: invalid argument: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    if (sh_env->subshell) {
        _exit(exit_status);
    } else {
        printf("exit\n");
        exit(exit_status);
    }
}

static int run_cd_builtin(char **argv, shell_env *_) {
    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    if (chdir(argv[1]) == -1) {
        fprintf(stderr, "cd: chdir: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static bool verify_vp_key(const var_pair *vp) {
    for (const char *c = vp->key; *c != '\0'; ++c) {
        bool valid_key =
                ((*c >= 'a' && *c <= 'z') ||
                (*c >= 'A' && *c <= 'Z') ||
                (*c >= '0' && *c <= '9') ||
                *c == '-' || *c == '_');

        if (!valid_key) {
            fprintf(stderr, "set: invalid key: %s\n", vp->key);
            return false;
        }
    }

    return true;
}

static int run_set_builtin(char **argv, shell_env *sh_env) {
    if (!validate_argc(argv, 2, 2))
        return EXIT_FAILURE;

    var_pair vp = {0};
    strncpy(vp.key, argv[1], SHELL_VAR_MAX - 1);
    strncpy(vp.value, argv[2], SHELL_VAR_MAX - 1);

    if (!verify_vp_key(&vp))
        return EXIT_FAILURE;

    if (add_var(&sh_env->vars, &vp) == -1) {
        fprintf(stderr, "set: failed to add variable\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_unset_builtin(char **argv, shell_env *sh_env) {
    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    if (strlen(argv[1]) >= SHELL_VAR_MAX - 1) {
        fprintf(stderr, "set: variable name too long: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    delete_var(&sh_env->vars, argv[1]);

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
    if (!argv || !argv[0]) {
        LOG_ERR("try_run_builtin received bad argv structure");
        fatal("internal error check logs\n");
    }

    sh_builtin *builtin = get_builtin(argv[0]);
    if (builtin) {
        *status = builtin->func(argv, &sh_env);
        return true;
    }

    return false;
}
