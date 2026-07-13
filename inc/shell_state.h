#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <unistd.h>

typedef struct {
    bool subshell;
    int tty_fd;
} sh_env;

int env_init(void);
void env_free(void);
sh_env *get_env(void);

#endif
