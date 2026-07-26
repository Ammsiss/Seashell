#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>

#include "utils.h"
#include "shell_state.h"
#include "log.h"

shell_env sh_env = {0};

void env_init(void) {
    sh_env.subshell = false;

    sh_env.tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);
    if (sh_env.tty_fd == -1)
        err_exit("open");
}

void env_free(void) {
    xclose(sh_env.tty_fd);
    sh_env = (shell_env){0};
}
