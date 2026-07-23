#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variable.h"
#include "builtins.h"
#include "log.h"
#include "parser.h"
#include "utils.h"
#include "runner.h"

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

static jc_job *arg_to_job(char *name, char *job_arg) {
    if (job_arg[0] != '%' || job_arg[1] == '\0')
        goto fail;

    char *endptr;
    job_id jid = (int) strtol(&job_arg[1], &endptr, 10);

    if (*endptr != '\0')
        goto fail;

    jc_job *job = lookup_job(jid, NULL);
    if (!job)
        goto fail;

    return job;

fail:
    fprintf(stderr, "%s: job not found: %s\n", name, job_arg);
    return NULL;
}

static int run_fg_builtin(char **argv, shell_env *sh_env) {
    if (sh_env->subshell) {
        fprintf(stderr, "%s: no job control in this shell\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    jc_job *job = arg_to_job(argv[0], argv[1]);
    if (!job)
        return EXIT_FAILURE;

    if (xtcsetpgrp(sh_env->tty_fd, job->pgrp.pgid) == -1) {
        perror(argv[0]);
        return EXIT_FAILURE;
    }

    if (job->stat == PSTOPPED) {
        if (xkill(-job->pgrp.pgid, SIGCONT) == -1 && errno != ESRCH) {
            perror(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (getpgrp() != job->pgrp.pgid)
        if (jctl_wait(&job->id) == -1)
            xfatal("wait_for_pids");

    return EXIT_SUCCESS;
}

static int run_bg_builtin(char **argv, shell_env *sh_env) {
    if (sh_env->subshell) {
        fprintf(stderr, "%s: no job control in this shell\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    jc_job *job = arg_to_job(argv[0], argv[1]);
    if (!job)
        return EXIT_FAILURE;

    if (job->stat == PSTOPPED) {
        if (xkill(-job->pgrp.pgid, SIGCONT) == -1 && errno != ESRCH) {
            perror(argv[0]);
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "%s: job %d already in background\n", argv[0],
                job->id);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int arg_to_sig(char *sig_arg) {
    /* - just to be consistent */
    if (strcmp("-HUP", sig_arg) == 0)
        return SIGHUP;
    if (strcmp("-INT", sig_arg) == 0)
        return SIGINT;
    if (strcmp("-QUIT", sig_arg) == 0)
        return SIGQUIT;
    if (strcmp("-KILL", sig_arg) == 0)
        return SIGKILL;
    if (strcmp("-USR1", sig_arg) == 0)
        return SIGUSR1;
    if (strcmp("-SEGV", sig_arg) == 0)
        return SIGSEGV;
    if (strcmp("-USR2", sig_arg) == 0)
        return SIGUSR2;
    if (strcmp("-PIPE", sig_arg) == 0)
        return SIGPIPE;
    if (strcmp("-ALRM", sig_arg) == 0)
        return SIGALRM;
    if (strcmp("-TERM", sig_arg) == 0)
        return SIGTERM;
    if (strcmp("-CHLD", sig_arg) == 0)
        return SIGCHLD;
    if (strcmp("-CONT", sig_arg) == 0)
        return SIGCONT;
    if (strcmp("-STOP", sig_arg) == 0)
        return SIGSTOP;
    if (strcmp("-TSTP", sig_arg) == 0)
        return SIGTSTP;
    if (strcmp("-TTIN", sig_arg) == 0)
        return SIGTTIN;
    if (strcmp("-TTOU", sig_arg) == 0)
        return SIGTTOU;
    if (strcmp("-WINCH", sig_arg) == 0)
        return SIGWINCH;

    return -1;
}

/* default sig is TERM */
static int run_kill_builtin(char **argv, shell_env *sh_env) {
    if (sh_env->subshell) {
        fprintf(stderr, "%s: no job control in this shell\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!validate_argc(argv, 1, 2))
        return EXIT_FAILURE;

    int sig = SIGTERM;
    jc_job *job = NULL;

    if (!argv[2]) {
        if (!(job = arg_to_job(argv[0], argv[1])))
            return EXIT_FAILURE;
    } else {
        sig = arg_to_sig(argv[1]);
        if (sig == -1) {
            fprintf(stderr, "%s: unknown signal: %s\n", argv[0], argv[1]);
            return EXIT_FAILURE;
        }

        if (!(job = arg_to_job(argv[0], argv[2])))
            return EXIT_FAILURE;
    }

    if (xkill(-job->pgrp.pgid, sig) == -1 && errno != ESRCH) {
        perror(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_jobs_builtin(char **argv, shell_env *sh_env) {
    if (!validate_argc(argv, 0, 0))
        return EXIT_FAILURE;

    for (size_t i = 0; i < sh_env->jctl.jobs.size; ++i) {

        jc_job *job = &sh_env->jctl.jobs.data[i];

        printf("[%d] ", job->id);

        /* TODO: Make option -p for this */
        // char *pid_str = get_pid_string(job->id);
        // if (!pid_str) {
        //     fprintf(stderr, "get_pid_string\n");
        //     return EXIT_FAILURE;
        // }

        switch (job->stat) {
        case PRUNNING:
            printf("running");
            break;
        case PSTOPPED:
            printf("stopped");
            break;
        case PEXITED:
            printf("???");
            break;
        }

        char *cmd_str = get_cmd_string(job->id);
        if (!cmd_str) {
            fprintf(stderr, "get_cmd_string\n");
            return EXIT_FAILURE;
        }

        printf("   %s\n", cmd_str);
        free(cmd_str);
    }

    return EXIT_SUCCESS;
}

static int run_exit_builtin(char **argv, shell_env *sh_env) {
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

static int run_set_builtin(char **argv, shell_env *sh_env) {
    if (!validate_argc(argv, 2, 2))
        return EXIT_FAILURE;

    var_err err = add_var(&sh_env->vars, argv[1], argv[2]);

    if (err != 0) {
        fprintf(stderr, "%s: %s\n", argv[0], var_errstr(err));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int run_unset_builtin(char **argv, shell_env *sh_env) {
    if (!validate_argc(argv, 1, 1))
        return EXIT_FAILURE;

    var_err err = delete_var(&sh_env->vars, argv[1]);

    if (err != VAR_OK) {
        fprintf(stderr, "%s: %s\n", argv[0], var_errstr(err));
        return EXIT_FAILURE;
    }

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
        int out = builtin->func(argv, &sh_env);

        if (status)
            *status = out;

        return true;
    }

    return false;
}
