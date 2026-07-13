#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <unistd.h>

typedef struct {
    bool subshell;
    int tty_fd;
} sh_env;

sh_env *get_env(void);

#endif
