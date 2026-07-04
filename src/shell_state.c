#include <assert.h>

#include "shell_state.h"
#include "shell_types.h" // IWYU pragma: keep
#include "dyn_arr.h"

static da_vars st_vars;

void st_add_var(var_pair var) {
    var_pair *v = da_push(&st_vars);
    if (!v)
        return;

    *v = var;
}
