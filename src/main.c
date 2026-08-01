#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <wait.h>

#include "builtins.h"
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

#define NOFG -1

static void print_job_event(job_event *jev, bool *prompt_upset) {
    assert(jev && prompt_upset);

    if (!*prompt_upset) {
        printf("\n");
        *prompt_upset = true;
    }

    printf("%s", get_jev_str(*jev));
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

    LOG_INFO("seashell shutting down");
}

static void process_signals(void) {
    if (sigchld_caught) {
        wait_event wev;

        while (get_wstat(&wev) != -1)
            if (update_job_proc(wev) == -1)
                xfatal("update_job_table");

        sigchld_caught = false;
    }

    if (sighup_caught) {
        LOG_INFO("sighup caught");
        exit(EXIT_FAILURE);
    }

    if (sigint_caught) {
        LOG_INFO("sigint caught");
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

void reclaim_terminal(void) {
    if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
        err_exit("tcsetpgrp");

    sh_env.fg_jid = NOFG;
}

bool fg_event(job_event *jev) {
    return sh_env.fg_jid  == jev->jid;
}

int main(void) {
    log_init("/home/juta/Projects/Seashell/logs");
    env_init();
    sig_setup();

    if (xatexit(hup_to_children) == -1)
        err_exit("atexit");

    LOG_INFO("seashell PID(%d)", getpid());

    display_prompt(PROMPT_SIMPLE);

    while (true) {
        int sh_ready = shell_block(sh_env.fg_jid);

        if (sh_ready == SIG_READY) {

            bool need_prompt = false;
            bool prompt_upset = false;

            job_event *jev;
            while ((jev = pop_job_event())) {

                if (!fg_event(jev)) {
                    print_job_event(jev, &prompt_upset);

                } else if (jev->type == JSTOPPED || jev->type == JEXITED) {
                    reclaim_terminal();
                    need_prompt = true;

                    if (jev->type == JSTOPPED)
                        print_job_event(jev, &prompt_upset);
                }
            }

            if (need_prompt || (sh_env.fg_jid == NOFG && prompt_upset))
                display_prompt(PROMPT_SIMPLE);

        } else if (sh_ready == STDIN_READY) {
            ps_ast ast;
            line_to_ast(&ast);

            ps_pline *pline = &ast.andors.data[0].pline;
            bool handled = false;

            if (pline->cmds.size == 1 && !ast.bg)
                handled = try_run_builtin(pline->cmds.data[0].argv, NULL);

            if (handled) {
                display_prompt(PROMPT_SIMPLE); /* never lost term fg status */

            } else {
                pline_data pld = exec_pline(&ast.andors.data[0].pline, ast.bg);

                pid_t jid = add_job(pld.pids, pld.pgid);
                if (jid == -1)
                    xfatal("add_job");

                free_pline_data(&pld);

                if (ast.bg) {
                    printf("[%d] started\n", jid);
                    display_prompt(PROMPT_SIMPLE);

                } else {
                    sh_env.fg_jid = jid;
                }
            }

            ps_free(&ast);
        }
    }

    return EXIT_SUCCESS;
}
