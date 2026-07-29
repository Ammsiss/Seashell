#define _GNU_SOURCE

#include <signal.h>

#include "log.h"
#include "shell_state.h"
#include "sig_funcs.h"

volatile sig_atomic_t sigchld_caught = false;
volatile sig_atomic_t sighup_caught = false;
volatile sig_atomic_t sigint_caught = false;

void sigchld_handler(int _) {
    sigchld_caught = true;
}

void sighup_handler(int _) {
    sighup_caught = true;
}

void sigint_handler(int _) {
    sigint_caught = true;
}

int set_sig_action(int sig, sighandler_t handler, int flags, sigset_t *mask) {
    struct sigaction sa;
    sa.sa_flags = flags;
    sa.sa_handler = handler;

    if (mask) {
        sa.sa_mask = *mask;
    } else {
        if (xsigemptyset(&sa.sa_mask) == -1)
            return -1;
    }

    if (xsigaction(sig, &sa, NULL) == -1)
        return -1;

    return 0;
}

int procmask_add(int sig, int how) {
    sigset_t set;

    if (xsigemptyset(&set) == -1)
        return -1;
    if (xsigaddset(&set, sig) == -1)
        return -1;

    if (xsigprocmask(how, &set, NULL) == -1)
        return -1;

    return 0;
}

int block_sig(int sig) {
    if (procmask_add(sig, SIG_BLOCK) == -1)
        return -1;

    return 0;
}

void sig_restore(void) {
    if (xsigprocmask(SIG_SETMASK, &sh_env.og_mask, NULL) == -1)
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
}

void sig_setup(void) {
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
}
