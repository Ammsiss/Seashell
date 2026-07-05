#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_state.h"
#include "shell_types.h" // IWYU pragma: keep
#include "dyn_arr.h"

static da_vars st_vars;

char *st_lookup_var(char *key) {
    for (size_t i = 0; i < st_vars.size; ++i) {
        if (strcmp(st_vars.data[i].key, key) == 0) {
            return st_vars.data[i].value;
        }
    }

    return NULL;
}

void st_add_var(var_pair *var) {
    char *old_value = st_lookup_var(var->key);
    if (old_value) {
        strcpy(old_value, var->value);
    } else {
        var_pair *new_var = da_push(&st_vars);
        if (!new_var)
            return;

        *new_var = *var;
    }
}
