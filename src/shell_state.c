#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>

#include "input.h"
#include "utils.h"
#include "shell_state.h"
#include "log.h"

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

int setup_procmask(void) {
    /* save initial procmask */
    if (xsigprocmask(0, NULL, &sh_env.og_mask) == -1)
        err_exit("sigprocmask");

    /* SIGTTOU */
    if (set_sig_action(SIGTTOU, SIG_IGN, 0, NULL) == -1)
        fatal("set_sig_action");

    /* SIGCHLD */
    if (block_sig(SIGCHLD) == -1)
        fatal("block_sig");
    if (set_sig_action(SIGCHLD, sigchld_handler, 0, NULL) == -1)
        fatal("set_sig_action");

    /* SIGHUP */
    if (block_sig(SIGHUP) == -1)
        fatal("block_sig");
    if (set_sig_action(SIGHUP, sighup_handler, 0, NULL) == -1)
        fatal("set_sig_action");

    /* SIGINT */
    if (block_sig(SIGINT) == -1)
        fatal("block_sig");
    if (set_sig_action(SIGINT, sigint_handler, 0, NULL) == -1)
        fatal("set_sig_action");

    return 0;
}

int env_init(void) {
    sh_env.subshell = false;

    sh_env.tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);
    if (sh_env.tty_fd == -1)
        err_exit("open");

    if (setup_procmask() == -1)
        xfatal("setup_procmask");

    if (da_init(&sh_env.vars) == -1)
        xfatal("da_init");

    if (init_jst(&sh_env.jctl) == -1)
        xfatal("init_jst");

    return 0;
}

void env_free(void) {
    xclose(sh_env.tty_fd);
    da_free(&sh_env.vars);
    free_jst(&sh_env.jctl);
    sh_env = (shell_env){0};
}
