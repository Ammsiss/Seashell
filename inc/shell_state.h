#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <bits/types/sigset_t.h>
#include <fcntl.h>
#include <unistd.h>

#define NOFG -1

typedef struct {
    bool subshell;
    pid_t fg_jid;
    int tty_fd;
    sigset_t og_mask;
} shell_env;

extern shell_env sh_env;

bool shell_in_fg(void);

void env_init(void);
void env_free(void);

#endif
