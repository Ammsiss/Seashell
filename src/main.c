#define _GNU_SOURCE

#include <poll.h>
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
    if (*line == '\0')
        return;

    da_tok toks;

    lx_status lexer_status = lx_tokenize(line, &toks);
    if (lexer_status != 0)
        xfatal("%s", lx_errstr(lexer_status));

    ps_ast ast;

    if (ps_parse(&toks, &ast) != 0)
        xfatal("parse error");

    if (ex_expand(&ast) != 0)
        xfatal("expand error");

    sh_run(&ast);

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
