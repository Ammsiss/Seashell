#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <unistd.h>

#include "shell_types.h"

#define SHELL_VAR_MAX 4096

struct var_pair {
    char key[SHELL_VAR_MAX];
    char value[SHELL_VAR_MAX];
};

int st_add_var(var_pair *var);
int st_delete_var(var_pair *var);
char *st_lookup_var(char *key);

#endif
