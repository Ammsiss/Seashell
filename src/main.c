#define _GNU_SOURCE

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>

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

    sh_run(&job);

    lx_free(&toks);
    ps_free(&job);
}

static volatile sig_atomic_t sigint_caught = false;

void sigint_handler(int sig) {
    (void)sig;
    sigint_caught = true;
}

int main(void) {
    if (log_init() == -1)
        errExit(false, "log_init");
    if (env_init() == -1)
        errExit(false, "env_init");

    LOG_INFO("seashell PID(%d)", getpid());

    if (set_sig_action(SIGTTOU, SIG_IGN, 0, NULL) == -1)
        errExit(false, "set_sig_action");

    if (block_sig(SIGINT) == -1)
        errExit(false, "block_sig");
    if (set_sig_action(SIGINT, sigint_handler, 0, NULL) == -1)
        errExit(false, "set_sig_action");

    sigset_t block_set;
    if (sigprocmask(0, NULL, &block_set) == -1)
        errExit(true, "sigprocmask");
    if (sigdelset(&block_set, SIGINT) == -1)
        errExit(true, "sigdelset");

    struct pollfd events;
    events.events = POLLIN;
    events.fd = get_env()->tty_fd;

    char *line;

    while (true) {
        if (display_prompt() == -1)
            errExit(false, "display_prompt");

        int pollfd = xppoll(&events, 1, 0, &block_set);
        if (pollfd == -1) {
            if (errno == EINTR) {
                if (sigint_caught) {
                    printf("caught SIGINT; continuing...\n");
                    sigint_caught = false;
                    continue;
                }
            } else {
                errExit(true, "poll");
            }
        }

        input_status tty_st = get_line(&line);

        if (tty_st == INPUT_OK) {
            run_cmd(line);
        } else if (tty_st == INPUT_EOF) {
            break;
        } else if (tty_st == INPUT_ERR)
            errExit(false, "failed to read from terminal");
    }

    log_free();
    env_free();

    return EXIT_SUCCESS;
}
