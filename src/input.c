#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h> // IWYU pragma: keep
#include <unistd.h>
#include <fcntl.h>

#include "input.h"
#include "dyn_str.h"
#include "log.h"
#include "utils.h"
#include "xfuncs.h"

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
        char *cwd_base = basename(cwd);

        if (d_strcat(&prompt, cwd_base) == -1)
            xfatal("d_strcat");
        if (d_strcat(&prompt, "> ") == -1)
            xfatal("d_strcat");
    } else
        xfatal("unknown prompt mode");

    printf("%s", prompt.c_str);
    fflush(stdout);

    d_str_free(&prompt);
}

char *get_line(void) {
    if (!fgets(input_line, LINE_BUF, stdin))
        xfatal("fgets");

    if (feof(stdout)) {
        LOG_INFO("eof, exiting");
        exit(EXIT_SUCCESS);
    }

    if (input_line[strlen(input_line) - 1] == '\n')
        input_line[strlen(input_line) - 1 ] = '\0';

    return input_line;
}
