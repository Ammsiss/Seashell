#include <string.h>

#include "variable.h"
#include "dyn_arr.h"
#include "log.h"

static bool get_index_var(da_vars *vars, char *key, size_t *index) {
    for (size_t i = 0; i < vars->size; ++i) {
        if (strcmp(vars->data[i].key, key) == 0) {
            *index = i;
            return true;
        }
    }

    return false;
}

char *lookup_var(da_vars *vars, char *key) {
    size_t index;
    if (get_index_var(vars, key, &index)) {
        return vars->data[index].value;
    } else
        return NULL;
}

int add_var(da_vars *vars, var_pair *var) {
    size_t index;
    if (get_index_var(vars, var->key, &index)) {
        strcpy(vars->data[index].value, var->value);
    } else {
        var_pair *new_var = da_push(vars);
        if (!new_var) {
            LOG_ERR("da_push");
            return - 1;
        }

        *new_var = *var;
    }

    return 0;
}

int delete_var(da_vars *vars, char *key) {
    size_t index;
    if (get_index_var(vars, key, &index)) {
        if (da_delete(vars, index) == -1) {
            LOG_ERR("da_delete");
            return -1;
        }
    }

    return 0;
}
