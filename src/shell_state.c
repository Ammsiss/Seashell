#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>

#include "variable.h"
#include "input.h"
#include "ast_man.h"
#include "utils.h"
#include "shell_state.h"
#include "log.h"
#include "map.h"

shell_env sh_env = {0};

static volatile sig_atomic_t sigchld_caught = false;
static volatile sig_atomic_t sighup_caught = false;
static volatile sig_atomic_t sigint_caught = false;

void sigchld_handler(int _) {
    sigchld_caught = true;
}

void sighup_handler(int _) {
    sighup_caught = true;
}

void sigint_handler(int _) {
    sigint_caught = true;
}

int process_signals(void) {
    if (sigchld_caught) {
        if (jctl_wait(NULL) == -1)
            return -1;

        sigchld_caught = false;
    }

    if (sighup_caught) {
        if (sighup_shutdown() == -1)
            xfatal("sighup_shutdown");
        exit(EXIT_SUCCESS);
    }

    if (sigint_caught) {
        printf("\n");

        if (display_prompt() == -1)
            return -1;

        sigint_caught = false;
    }

    return 0;
}

int restore_signals(void) {
    if (sigprocmask(SIG_SETMASK, &sh_env.og_mask, NULL) == -1)
        err_exit("sigprocmask");

    if (set_sig_action(SIGTTOU, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sig_action");
    if (set_sig_action(SIGTTIN, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTSTP, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGQUIT, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTERM, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");

    return 0;
}

int setup_signals(void) {
    if (xsigprocmask(0, NULL, &sh_env.og_mask) == -1)
        err_exit("sigprocmask");

    if (set_sig_action(SIGTTOU, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sig_action");
    if (set_sig_action(SIGTTIN, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTSTP, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGQUIT, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTERM, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");

    if (block_sig(SIGCHLD) == -1)
        xfatal("block_sig");
    if (set_sig_action(SIGCHLD, sigchld_handler, 0, NULL) == -1)
        xfatal("set_sig_action");

    if (block_sig(SIGHUP) == -1)
        xfatal("block_sig");
    if (set_sig_action(SIGHUP, sighup_handler, 0, NULL) == -1)
        xfatal("set_sig_action");

    if (block_sig(SIGINT) == -1)
        xfatal("block_sig");
    if (set_sig_action(SIGINT, sigint_handler, 0, NULL) == -1)
        xfatal("set_sig_action");

    return 0;
}

int env_init(void) {
    sh_env.subshell = false;

    sh_env.tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);
    if (sh_env.tty_fd == -1)
        err_exit("open");

    if (setup_signals() == -1)
        xfatal("setup_procmask");

    if (mp_init(&sh_env.vars, VAR_KEY_SIZE) == -1)
        xfatal("mp_init");

    if (init_jst(&sh_env.jctl) == -1)
        xfatal("init_jst");

    init_job_plans(&sh_env.job_plans);

    return 0;
}

void env_free(void) {
    xclose(sh_env.tty_fd);
    mp_free(&sh_env.vars);
    free_jst(&sh_env.jctl);
    free_job_plans(&sh_env.job_plans);
    sh_env = (shell_env){0};
}
