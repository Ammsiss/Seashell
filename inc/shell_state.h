#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <unistd.h>

#include "shell_types.h"

#define SHELL_VAR_MAX 4096

struct var_pair {
    char key[SHELL_VAR_MAX];
    char value[SHELL_VAR_MAX];
};

char *st_lookup_var(char *key);
void st_add_var(var_pair *var);

#endif
