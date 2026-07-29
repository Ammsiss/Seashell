#ifndef SIGNAL_FUNCS_H
#define SIGNAL_FUNCS_H

#define _GNU_SOURCE

#include <signal.h>

#include "log.h"

extern volatile sig_atomic_t sigchld_caught;
extern volatile sig_atomic_t sighup_caught;
extern volatile sig_atomic_t sigint_caught;

int set_sig_action(int sig, sighandler_t handler, int flags, sigset_t *mask);
int procmask_add(int sig, int how);
int block_sig(int sig);
void sig_restore(void);
void sig_setup(void);

#endif
