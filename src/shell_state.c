#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shell_state.h"
#include "log.h"
#include "shell_types.h" // IWYU pragma: keep
#include "dyn_arr.h"

static da_vars st_vars = {0};

static bool st_get_index_var(char *key, size_t *index) {
    for (size_t i = 0; i < st_vars.size; ++i) {
        if (strcmp(st_vars.data[i].key, key) == 0) {
            *index = i;
            return true;
        }
    }

    return false;
}

char *st_lookup_var(char *key) {
    size_t index;
    if (st_get_index_var(key, &index)) {
        return st_vars.data[index].value;
    } else
        return NULL;
}

int st_add_var(var_pair *var) {
    size_t index;
    if (st_get_index_var(var->key, &index)) {
        strcpy(st_vars.data[index].value, var->value);
    } else {
        var_pair *new_var = da_push(&st_vars);
        if (!new_var) {
            LOG_ERR("da_push");
            return - 1;
        }

        *new_var = *var;
    }

    return 0;
}

int st_delete_var(char *key) {
    size_t index;
    if (st_get_index_var(key, &index)) {
        if (da_delete(&st_vars, index) == -1) {
            LOG_ERR("da_delete");
            return -1;
        }
    }

    return 0;
}
