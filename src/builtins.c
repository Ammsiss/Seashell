#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell_state.h"
#include "input.h"
#include "job_state.h"
#include "builtins.h"
#include "log.h"
#include "utils.h"

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

static int run_fg_builtin(char **argv, shell_env *sh_env) {
    if (sh_env->subshell) {
        fprintf(stderr, "%s: no job control in this shell\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int run_bg_builtin(char **argv, shell_env *sh_env) {
    if (sh_env->subshell) {
        fprintf(stderr, "%s: no job control in this shell\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/* default sig is TERM */
static int run_kill_builtin(char **argv, shell_env *sh_env) {
    if (sh_env->subshell) {
        fprintf(stderr, "%s: no job control in this shell\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!validate_argc(argv, 1, 2))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int run_jobs_builtin(char **argv, shell_env *sh_env) {
    if (sh_env->subshell) {
        fprintf(stderr, "%s: no job control in this shell\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!validate_argc(argv, 0, 0))
        return EXIT_FAILURE;

    job_table *jctl = get_jctl();

    for (size_t i = 0; i < jctl->jobs.size; ++i) {

        jc_job *job = &jctl->jobs.data[i];

        printf("[%d] ", job->jid);

        switch (job->stat) {
        case JOB_RUN:
            printf("running");
            break;
        case JOB_STOP:
            printf("stopped");
            break;
        case JOB_EXIT:
            printf("done (?)");
            break;
        }

        printf("\n");
    }

    return EXIT_SUCCESS;
}

static int run_exit_builtin(char **argv, shell_env *_) {
    if (!validate_argc(argv, 0, 1))
        return EXIT_FAILURE;

    int exit_status = EXIT_SUCCESS;

    if (argv[1]) {
        char *endptr;
        exit_status = strtol(argv[1], &endptr, 10);

        if (strcmp(argv[1], "") == 0 || *endptr != '\0') {
            fprintf(stderr, "exit: invalid argument: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    if (sh_env.subshell) {
        _exit(exit_status);
    } else {
        printf("exit\n");
        exit(exit_status);
    }

    return EXIT_SUCCESS;
}

static int run_cd_builtin(char **argv, shell_env *_) {
    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    if (chdir(argv[1]) == -1) {
        fprintf(stderr, "%s: chdir: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_set_builtin(char **argv, shell_env *_) {
    if (!validate_argc(argv, 2, 2))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static int run_unset_builtin(char **argv, shell_env *_) {
    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static sh_builtin builtins[BUILTIN_COUNT] = {
    { .name = "exit", .func = run_exit_builtin },
    { .name = "cd", .func = run_cd_builtin },
    { .name = "set", .func = run_set_builtin },
    { .name = "unset", .func = run_unset_builtin },
    { .name = "jobs", .func = run_jobs_builtin },
    { .name = "kill", .func = run_kill_builtin },
    { .name = "fg", .func = run_fg_builtin },
    { .name = "bg", .func = run_bg_builtin }
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
        int builtin_status = builtin->func(argv, &sh_env);

        if (status)
            *status = builtin_status;

        if (!sh_env.subshell && shell_in_fg())
            display_prompt(PROMPT_SIMPLE);

        return true;
    }

    return false;
}
