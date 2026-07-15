#define _GNU_SOURCE

#include <signal.h> // IWYU pragma: keep
#include <assert.h>
#include <fcntl.h>

#include "shell_state.h"
#include "log.h"
#include "shell_types.h" // IWYU pragma: keep

static sh_env shell_env = {0};

int env_init(void) {
    shell_env.subshell = false;
    shell_env.tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);
    if (shell_env.tty_fd == -1)
        return -1;
    if (xsigprocmask(0, NULL, &shell_env.og_mask) == -1)
        return -1;

    return 0;
}

void env_free(void) {
    xclose(shell_env.tty_fd);
    shell_env = (sh_env){0};
}

sh_env *get_env(void) {
    return &shell_env;
}
