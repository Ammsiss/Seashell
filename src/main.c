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

sh_result run_cmd(const char *line) {
    da_tok toks;
    ps_job job;

    if (lx_tokenize(line, &toks) == -1) {
        fprintf(stderr, "Lexer error\n");
        exit(EXIT_FAILURE);
    }

    if (ps_parse(&toks, &job) == -1) {
        fprintf(stderr, "Parser error\n");
        exit(EXIT_FAILURE);
    }

    if (ex_expand(&job) == -1) {
        fprintf(stderr, "Expansion error\n");
        exit(EXIT_FAILURE);
    }

    sh_result result = sh_run(&job);

    lx_free(&toks);
    ps_free(&job);

    return result;
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
        sh_result result = run_cmd(opt_cmd);
        if (result.exit_code == SH_FAIL)
            fprintf(stderr, "seashell: %s\n", result.msg);
        exit(EXIT_SUCCESS);
    }

    char *line;

    while (true) {
        printf("> ");
        fflush(stdout);

        line = NULL;
        size_t len;

        int num_read = getline(&line, &len, stdin);
        if (num_read == -1) {
            free(line);
            if (feof(stdin))
                break;
            return EXIT_FAILURE;
        }

        if (line[num_read - 1] == '\n')
            line[num_read - 1] = '\0';

        if (strlen(line) != 0) {
            sh_result result = run_cmd(line);
            if (result.exit_code == SH_FAIL) {
                fprintf(stderr, "seashell: %s\n", result.msg);
            } else if (result.exit_code == SH_EXIT) {
                printf("exit\n");
                free(line);
                break;
            }
        }

        free(line);
    }

    return EXIT_SUCCESS;
}
