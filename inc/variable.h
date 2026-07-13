#ifndef VARIABLE_H
#define VARIABLE_H

#define SHELL_VAR_MAX 4096

#include "shell_types.h"

struct var_pair {
    char key[SHELL_VAR_MAX];
    char value[SHELL_VAR_MAX];
};

int st_add_var(var_pair *var);
int st_delete_var(char *key);
char *st_lookup_var(char *key);

#endif
