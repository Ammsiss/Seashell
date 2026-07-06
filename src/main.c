#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <wait.h>

#include "utils.h"
#include "log.h"
#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "executor.h"

void run_cmd(const char *line) {
    da_tok toks;
    ps_job job;

    lx_status lexer_status = lx_tokenize(line, &toks);
    if (lexer_status != 0) {
        switch (lexer_status) {
        case LX_ERRMEM:
            errExit(EXIT_FAILURE, false, "seashell: lexer: malloc failure\n");
        case LX_ERRNOENDQUOTE:
            errExit(EXIT_FAILURE, false, "seashell: lexer: unterminated quote\n");
        case LX_ERREMPTYESC:
            errExit(EXIT_FAILURE, false, "seashell: lexer: empty escape\n");
        case LX_ERRINPUT:
            errExit(EXIT_FAILURE, false, "seashell: bad input\n");
        default:
            LOG_ERR("unknown lx_status case");
            exit(EXIT_FAILURE);
        }
    }

    if (!(ps_parse(&toks, &job) == 0 && ex_expand(&job) == 0)) {
        fprintf(stderr, "seashell: syntax error\n");
        exit(EXIT_FAILURE);
    }

    sh_run(&job, STDIN_FILENO, STDOUT_FILENO);

    lx_free(&toks);
    ps_free(&job);
}

int main(int argc, char **argv) {
    if (log_init() == -1)
        return EXIT_FAILURE;

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

        printf(CMAGENTA "%s" CCL " " CMAGENTA ">" CCL " ", cwd_base);
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
