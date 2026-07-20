#define _GNU_SOURCE

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>

#include "input.h"
#include "shell_state.h"
#include "utils.h"
#include "log.h"
#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "runner.h"

void run_cmd(const char *line) {
    da_tok toks;
    ps_ast ast;

    lx_status lexer_status = lx_tokenize(line, &toks);
    if (lexer_status != 0) {
        switch (lexer_status) {
        case LX_ERRMEM:
            fatal("seashell: lexer: malloc failure\n");
        case LX_ERRNOENDQUOTE:
            fatal("seashell: lexer: unterminated quote\n");
        case LX_ERREMPTYESC:
            fatal("seashell: lexer: empty escape\n");
        case LX_ERRINPUT:
            fatal("seashell: bad input\n");
        default:
            LOG_ERR("unknown lx_status case");
            exit(EXIT_FAILURE);
        }
    }

    if (!(ps_parse(&toks, &ast) == 0 && ex_expand(&ast) == 0)) {
        fprintf(stderr, "seashell: syntax error\n");
        exit(EXIT_FAILURE);
    }

    /* no andors for now */
    sh_run_job(&ast.andors.data[0].pline, ast.bg);

    lx_free(&toks);
    ps_free(&ast);
}

int main(void) {
    if (log_init() == -1)
        fatal("log_init");
    if (env_init() == -1)
        fatal("env_init");

    LOG_INFO("seashell PID(%d)", getpid());

    struct pollfd events = {
        .events = POLLIN,
        .fd = sh_env.tty_fd
    };

    while (true) {
        if (display_prompt() == -1)
            fatal("display_prompt");

        int ready;
        while ((ready = xppoll(&events, 1, 0, &sh_env.og_mask)) == -1) {
            if (errno != EINTR) {
                err_exit("poll");
            } else {
                if (process_signals() == -1)
                    fatal("process_signals");
            }
        }

        if (ready == 1) {
            char *line;
            switch (get_line(&line)) {
            case INPUT_ERR: fatal("failed to read from terminal");
            case INPUT_OK:  run_cmd(line); break;
            case INPUT_EOF: goto done;
            }
        }
    }

done:
    env_free();
    log_free();

    return EXIT_SUCCESS;
}
