#define _GNU_SOURCE

#include <stdlib.h>
#include <linux/limits.h>
#include <string.h> // IWYU pragma: keep
#include <unistd.h>
#include <fcntl.h>

#include "input.h"
#include "dyn_str.h"
#include "log.h"
#include "shell_state.h"

static char input_line[LINE_BUF];

void display_prompt(int mode) {
    d_str prompt;
    if (d_str_init(&prompt) == -1)
        xfatal("d_str_init");

    if (mode == PROMPT_SIMPLE) {
        d_strcat(&prompt, "> ");

    } else if (mode == PROMPT_CWD) {
        static char cwd_str[PATH_MAX];

        char *cwd = xgetcwd(cwd_str, PATH_MAX);
        if (!cwd)
            err_exit("getcwd");

        char *cwd_base = basename(cwd);

        if (d_strcat(&prompt, cwd_base) == -1)
            xfatal("d_strcat");
        if (d_strcat(&prompt, "> ") == -1)
            xfatal("d_strcat");
    } else
        xfatal("unknown prompt mode");

    if (xwrite(sh_env.tty_fd, prompt.c_str, prompt.len) != (int) prompt.len)
        err_exit("write");

    d_str_free(&prompt);
}

char *get_line(void) {
    int num_read = xread(sh_env.tty_fd, input_line, LINE_BUF - 1);
    if (num_read == -1)
        err_exit("read");

    if (num_read == 0)
        exit(EXIT_SUCCESS);

    input_line[num_read - 1] = '\0';

    return input_line;
}
