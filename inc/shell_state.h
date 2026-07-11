#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <unistd.h>

#include "shell_types.h"

#define SHELL_VAR_MAX 4096

struct var_pair {
    char key[SHELL_VAR_MAX];
    char value[SHELL_VAR_MAX];
};

typedef struct {
    bool subshell;
    int tty_fd;
} sh_env;

static sh_env shell_env = { .subshell = false };

int st_add_var(var_pair *var);
int st_delete_var(char *key);
char *st_lookup_var(char *key);

#endif
