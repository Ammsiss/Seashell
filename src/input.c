#define _GNU_SOURCE

#include <linux/limits.h>
#include <string.h> // IWYU pragma: keep
#include <unistd.h>
#include <fcntl.h>

#include "input.h"
#include "dyn_str.h"
#include "log.h"
#include "shell_state.h"

static char input_line[LINE_BUF];

void display_prompt(void) {
    d_str prompt;

    if (d_str_init(&prompt) == -1)
        xfatal("d_str_init");

    static char cwd_str[PATH_MAX];

    char *cwd = xgetcwd(cwd_str, PATH_MAX);
    if (!cwd)
        err_exit("getcwd");

    char *cwd_base = basename(cwd);

    if (d_strcat(&prompt, cwd_base) == -1)
        xfatal("d_strcat");
    if (d_strcat(&prompt, "> ") == -1)
        xfatal("d_strcat");

    if (xwrite(sh_env.tty_fd, prompt.c_str, prompt.len) != (int) prompt.len)
        err_exit("write");

    d_str_free(&prompt);
}

input_stat get_line(char **line) {
    int num_read = xread(sh_env.tty_fd, input_line, LINE_BUF - 1);
    if (num_read == -1)
        return INPUT_ERR;

    if (num_read == 0)
        return INPUT_EOF;

    input_line[num_read - 1] = '\0';

    *line = input_line;
    return INPUT_OK;
}
