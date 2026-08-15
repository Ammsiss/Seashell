#ifndef SIGNAL_FUNCS_H
#define SIGNAL_FUNCS_H

#define _GNU_SOURCE

#include <signal.h>

extern volatile sig_atomic_t sigchld_caught;
extern volatile sig_atomic_t sighup_caught;
extern volatile sig_atomic_t sigint_caught;

void reset_sig_flags(void);
void sig_restore(sigset_t *og_mask);
void sig_setup(sigset_t *og_mask);

#endif
