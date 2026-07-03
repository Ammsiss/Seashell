#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <wait.h>

#include "log.h"
#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "executor.h"

void run_cmd(const char *line) {
    da_tok toks;
    ps_job job;

    if (lx_tokenize(line, &toks) == -1) {
        fprintf(stderr, "seashell: lexer error\n");
        exit(EXIT_FAILURE);
    }

    if (!(ps_parse(&toks, &job) == 0 && ex_expand(&job) == 0)) {
        fprintf(stderr, "seashell: syntax error\n");
        exit(EXIT_FAILURE);
    }

    sh_run(&job);

    lx_free(&toks);
    ps_free(&job);
}

PFFORMAT(1, 2) void usage_err(const char *fmt, ...) {
    fflush(stdout);
    fprintf(stderr, "Usage: ");

    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);

    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (log_init() == -1)
        return -1;

    LOG_INFO("seashell PID(%d)", getpid());

    bool cmd_mode = false;
    const char *opt_cmd;

    char opt_char;
    while ((opt_char = getopt(argc, argv, ":c:")) != -1) {
        switch (opt_char) {
        case 'c':
            cmd_mode = true;
            opt_cmd = optarg;
            break;
        case ':':
            usage_err("-%c requires an argument", optopt);
            exit(EXIT_FAILURE); /* getopt prints usage errs */
        case '?':
            usage_err("%s [-c]", argv[0]);
        }
    }

    if (cmd_mode) {
        run_cmd(opt_cmd);
        exit(EXIT_SUCCESS);
    }

    char *line;

    while (true) {
        char *cwd = getcwd(NULL, 0);
        char *cwd_base = basename(cwd);

        printf(CBLUE "%s" CCL " " CMAGENTA ">" CCL " ", cwd_base);
        fflush(stdout);

        line = NULL;
        size_t len;

        int num_read = getline(&line, &len, stdin);
        if (num_read == -1) {
            free(line);
            free(cwd);
            if (feof(stdin))
                break;
            return EXIT_FAILURE;
        }

        if (line[num_read - 1] == '\n')
            line[num_read - 1] = '\0';

        if (strlen(line) != 0)
            run_cmd(line);

        free(line);
        free(cwd);
    }

    return EXIT_SUCCESS;
}
