#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

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

    sh_run(&job, STDIN_FILENO, STDOUT_FILENO);

    lx_free(&toks);
    ps_free(&job);
}

void process_sighup(const sigset_t *block_set) {
    if (sigtimedwait(block_set, NULL, &(struct timespec){0}) == -1) {
        if (errno != EAGAIN)
            errExit(true, "sigtimedwait");
    } else {
        LOG_INFO("caught sighup");
        /* send SIGHUP to all fg and bg pgroups */
        exit(128 + SIGHUP);
    }
}

int main(void) {
    struct sigaction sa;
    sa.sa_flags = 0;
    xsigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;

    if (xsigaction(SIGTTOU, &sa, NULL) == -1)
        errExit(true, "sigaction");

    sigset_t block_set;
    if (xsigemptyset(&block_set) == -1)
        errExit(true, "sigemptyset");
    if (xsigaddset(&block_set, SIGHUP) == -1)
        errExit(true, "sigaddset");
    if (xsigprocmask(SIG_SETMASK, &block_set, NULL) == -1)
        errExit(true, "sigprocmask");

    if (log_init() == -1)
        return EXIT_FAILURE;

    LOG_INFO("seashell PID(%d)", getpid());

    /* INITIALIZE SHELL STRUCTURES *************/
    shell_env.subshell = false;
    shell_env.tty_fd = xopen("/dev/tty", O_RDWR | O_CLOEXEC);
    if (shell_env.tty_fd == -1)
        errExit(true, "open");

    job_ctl_init();
    /*******************************************/

    /* INTERACTIVE INPUT ***********************/
    char *line;

    while (true) {
        input_status input_st = get_line(&line);

        if (input_st == INPUT_ERR) {
            errExit(false, "failed to get line");
        } else if (input_st == INPUT_EOF) {
            break;
        } else if (input_st == INPUT_SIG) {
            process_sighup(&block_set);
            /* handle sighup/sigchild */
        }

        run_cmd(line);
        process_sighup(&block_set);
    }
    /*******************************************/

    job_ctl_free();
    return EXIT_SUCCESS;
}
