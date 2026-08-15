#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>

#include "shell_state.h"
#include "log.h"
#include "xfuncs.h"

shell_env sh_env = {0};

bool shell_in_fg(void) {
    return sh_env.fg_jid == NOFG;
}

void env_init(void) {
    sh_env.subshell = false;
    sh_env.fg_jid = -1;
    sh_env.tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);
}

void env_free(void) {
    xclose(sh_env.tty_fd);
    sh_env = (shell_env){0};
}
