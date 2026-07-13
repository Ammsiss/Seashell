#include <assert.h>

#include "shell_state.h"
#include "shell_types.h" // IWYU pragma: keep

static sh_env shell_env = {0};

sh_env *get_env(void) {
    return &shell_env;
}
