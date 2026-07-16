#define _GNU_SOURCE

#include <signal.h> // IWYU pragma: keep
#include <assert.h>
#include <fcntl.h>

#include "shell_state.h"
#include "log.h"

shell_env sh_env = {0};

int env_init(void) {
    sh_env.subshell = false;
    sh_env.tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);
    if (sh_env.tty_fd == -1)
        return -1;
    if (xsigprocmask(0, NULL, &sh_env.og_mask) == -1)
        return -1;
    if (da_init(&sh_env.vars) == -1)
        return -1;

    return 0;
}

void env_free(void) {
    xclose(sh_env.tty_fd);
    da_free(&sh_env.vars);
    sh_env = (shell_env){0};
}
