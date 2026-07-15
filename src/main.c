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

static volatile sig_atomic_t sigchld_caught = false;

void sigchld_handler(int sig) {
    (void) sig;
    sigchld_caught = true;
}

void process_signals(void) {
    if (sigchld_caught) {
        /* wait for all pending zombie children */
        sigchld_caught = false;
    }
}

int main(void) {
    if (log_init() == -1)
        errExit(false, "log_init");
    if (env_init() == -1)
        errExit(false, "env_init");

    LOG_INFO("seashell PID(%d)", getpid());

    if (set_sig_action(SIGTTOU, SIG_IGN, 0, NULL) == -1)
        errExit(false, "set_sig_action");

    if (block_sig(SIGCHLD) == -1)
        errExit(false, "block_sig");
    if (set_sig_action(SIGCHLD, sigchld_handler, 0, NULL) == -1)
        errExit(false, "set_sig_action");

    struct pollfd events = {
        .events = POLLIN,
        .fd = get_env()->tty_fd
    };

    while (true) {
        if (display_prompt() == -1)
            errExit(false, "display_prompt");

        int ready;
        while ((ready = xppoll(&events, 1, 0, &get_env()->og_mask)) == -1) {
            if (errno != EINTR)
                errExit(true, "poll");
            process_signals();
        }

        char *line;
        switch (get_line(&line)) {
        case INPUT_ERR: errExit(false, "failed to read from terminal");
        case INPUT_OK:  run_cmd(line); break;
        case INPUT_EOF: goto done;
        }
    }

done:
    env_free();
    log_free();

    return EXIT_SUCCESS;
}
