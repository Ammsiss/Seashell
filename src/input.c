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

int display_prompt(void) {
    d_str prompt;
    if (d_str_init(&prompt) == -1)
        return -1;

    static char cwd_str[PATH_MAX];
    char *cwd = xgetcwd(cwd_str, PATH_MAX);
    if (!cwd)
        goto fail;
    char *cwd_base = basename(cwd);

    if (d_strcat(&prompt, cwd_base) == -1)
        goto fail;
    if (d_strcat(&prompt, "> ") == -1)
        goto fail;

    if (xwrite(sh_env.tty_fd, prompt.c_str, prompt.len) != (int) prompt.len) {
        LOG_ERR("failed/partial write");
        goto fail;
    }

    d_str_free(&prompt);
    return 0;

fail:
    d_str_free(&prompt);
    return -1;
}

input_status get_line(char **line) {
    do {
        int num_read = xread(sh_env.tty_fd, input_line, LINE_BUF - 1);
        if (num_read == -1)
            return INPUT_ERR;

        if (num_read == 0)
            return INPUT_EOF;

        input_line[num_read - 1] = '\0';
    } while (strlen(input_line) == 0);

    *line = input_line;
    return INPUT_OK;
}
