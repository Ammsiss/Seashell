#ifndef VARIABLE_H
#define VARIABLE_H

#define SHELL_VAR_MAX 4096

#include "dyn_arr.h"

struct var_pair {
    char key[SHELL_VAR_MAX];
    char value[SHELL_VAR_MAX];
};

typedef struct var_pair var_pair;

char *lookup_var(da_vars *vars, char *key);
int add_var(da_vars *vars, var_pair *var);
int delete_var(da_vars *vars, char *key);

#endif
