#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>

#include "jobctl.h"
#include "input.h"
#include "shell_state.h"
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
            errExit(false, "seashell: lexer: malloc failure\n");
        case LX_ERRNOENDQUOTE:
            errExit(false, "seashell: lexer: unterminated quote\n");
        case LX_ERREMPTYESC:
            errExit(false, "seashell: lexer: empty escape\n");
        case LX_ERRINPUT:
            errExit(false, "seashell: bad input\n");
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

int main(void) {
    if (set_sig_action(SIGTTOU, SIG_IGN, 0, NULL) == -1)
        errExit(false, "set_sig_action");

    if (log_init() == -1)
        return EXIT_FAILURE;
    if (env_init() == -1)
        return EXIT_FAILURE;
    if (job_ctl_init() == -1)
        return EXIT_FAILURE;

    LOG_INFO("seashell PID(%d)", getpid());

    char *line;
    input_status tty_st;

    do {
        tty_st = get_line(&line);
        if (tty_st == INPUT_OK)
            run_cmd(line);

    } while (tty_st == INPUT_OK);

    if (tty_st == INPUT_ERR)
        errExit(false, "failed to read from terminal");

    log_free();
    job_ctl_free();
    env_free();

    return EXIT_SUCCESS;
}
