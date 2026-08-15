#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <bits/types/sigset_t.h>
#include <fcntl.h>
#include <unistd.h>

#include "args.h"

#define NOFG -1

typedef struct {
    bool subshell;
    pid_t fg_jid;
    int tty_fd;
    sigset_t og_mask;
    int log_fd;
} shell_env;

extern shell_env sh_env;

typedef enum {
    LOGFD,
    LOGDIR,
} opt_names;

extern opt_data opts[];

bool shell_in_fg(void);
void env_setup(shell_env *env);

#endif
