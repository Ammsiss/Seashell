#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <wait.h>

#include "exec_funcs.h"
#include "expander.h"
#include "parser.h"
#include "lexer.h"
#include "input.h"
#include "shell_state.h"
#include "utils.h"
#include "log.h"

#include "job_state.h"
#include "sig_funcs.h"

#define SIG_READY 1
#define STDIN_READY 2

static void print_job_event(job_event *jev, bool *prompt_upset) {
    assert(jev && prompt_upset);

    if (!*prompt_upset) {
        printf("\n");
        *prompt_upset = true;
    }

    switch (jev->type) {
    case JEXITED:
        printf("[%d] exited\n", jev->jid);
        break;
    case JSTOPPED:
        printf("[%d] stopped\n", jev->jid);
        break;
    case JCONTINUED:
        printf("[%d] continued\n", jev->jid);
        break;
    }
}

static void hup_to_children(void) {
    for (size_t i = 0; i < get_jctl()->jobs.size; ++i) {
        jc_job *job = &get_jctl()->jobs.data[i];

        if (getpgrp() == job->pgrp.pgid || job->pgrp.pgid <= 1)
            xfatal("unexpected pgid %d", job->pgrp.pgid);

        if (xkill(-job->pgrp.pgid, SIGHUP) == -1 && errno != ESRCH)
            err_exit("kill");

        if (xkill(-job->pgrp.pgid, SIGCONT) == -1 && errno != ESRCH)
            err_exit("kill");
    }

    while (get_wstat(&(wait_event){0}) != -1)
        continue;

    clear_job_events();
    clear_job_table();
}

static void process_signals(void) {
    if (sigchld_caught) {
        wait_event wev;

        while (get_wstat(&wev) != -1)
            if (update_job_proc(wev) == -1)
                xfatal("update_job_table");

        sigchld_caught = false;
    }

    if (sighup_caught)
        exit(EXIT_FAILURE);

    if (sigint_caught) {
        sigint_caught = false;
    }
}

static void line_to_ast(ps_ast *ast) {
    char *line = get_line();
    da_tok toks = {0};

    if (lx_tokenize(line, &toks) == -1)
        xfatal("lx_tokenize");

    if (ps_parse(&toks, ast) == -1)
        xfatal("lx_tokenize");

    if (ex_expand(ast) == -1)
        xfatal("lx_tokenize");

    lx_free(&toks);
}

static int shell_block(pid_t fg_jid) {
    int nfds = fg_jid == -1 ? 1 : 0;

    struct pollfd events = {
        .events = POLLIN,
        .fd = sh_env.tty_fd
    };

    int ready = xppoll(&events, nfds, 0, &sh_env.og_mask);

    if (ready == -1 && errno != EINTR)
        err_exit("sigsuspend");

    if (ready == -1 && errno == EINTR) {
        process_signals();
        return SIG_READY;

    } else if (ready == 1) {
        return STDIN_READY;
    }

    xfatal("shouldn't reach here");
}

int main(void) {
    log_init();
    env_init();
    sig_setup();

    if (xatexit(hup_to_children) == -1)
        err_exit("atexit");

    LOG_INFO("seashell PID(%d)", getpid());

    display_prompt();

    while (true) {
        bool draw_prompt = false;
        bool prompt_upset = false;

        int sh_ready = shell_block(sh_env.fg_jid);

        job_event *jev;
        while ((jev = pop_job_event())) {

            if (sh_env.fg_jid != jev->jid) {
                print_job_event(jev, &prompt_upset);

            } else if (jev->type != JCONTINUED) {
                if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
                    err_exit("tcsetpgrp");

                if (jev->type == JSTOPPED) {
                    print_job_event(jev, &prompt_upset);
                }

                sh_env.fg_jid = -1;
                draw_prompt = true;
            }
        }

        if (sh_ready == STDIN_READY) {
            ps_ast ast;
            line_to_ast(&ast);

            pline_data pld = exec_pline(&ast.andors.data[0].pline, ast.bg);

            pid_t jid = add_job(pld.pids, pld.pgid);
            if (jid == -1)
                xfatal("add_job");

            if (!ast.bg) {
                sh_env.fg_jid = jid;
            }

            if (ast.bg) {
                printf("[%d] started\n", jid);
                draw_prompt = true;
            }

            free_pline_data(&pld);
            ps_free(&ast);
        }

        if (draw_prompt || (prompt_upset && sh_env.fg_jid == -1))
            display_prompt();
    }

    return EXIT_SUCCESS;
}
