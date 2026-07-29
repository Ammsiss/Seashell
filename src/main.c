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
#include "wait_stat.h"
#include "sig_funcs.h"

#define SIG_READY 1
#define STDIN_READY 2

static void print_job_event(job_event *jev) {
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

    da_wevent wevs;
    get_wstats(&wevs);
    da_free(&wevs);
}

static void process_signals(void) {
    if (sigchld_caught) {
        da_wevent wevs;
        get_wstats(&wevs);

        if (update_job_table(&wevs) == -1)
            xfatal("update_job_table");

        da_free(&wevs);

        sigchld_caught = false;
    }

    if (sighup_caught)
        exit(EXIT_FAILURE);

    if (sigint_caught) {
        sigint_caught = false;
    }
}

static void line_to_ast(const char *line, ps_ast *ast) {
    da_tok toks = {0};

    if (lx_tokenize(line, &toks) == -1)
        xfatal("lx_tokenize");

    if (ps_parse(&toks, ast) == -1)
        xfatal("lx_tokenize");

    if (ex_expand(ast) == -1)
        xfatal("lx_tokenize");

    lx_free(&toks);
}

static int shell_block(bool for_stdin) {
    int nfds = for_stdin ? 1 : 0;

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
        int sh_ready = shell_block(true);

        if (sh_ready == SIG_READY) {
            job_event *jev;
            while ((jev = pop_job_event())) {
                printf("\n");
                print_job_event(jev);
                display_prompt();
            }

        } else if (sh_ready == STDIN_READY) {
            char *line = get_line();

            ps_ast ast;
            line_to_ast(line, &ast);

            pline_data pld = exec_pline(&ast.andors.data[0].pline, ast.bg);

            pid_t jid = add_job(pld.pids, pld.pgid);
            if (jid == -1)
                xfatal("add_job");

            if (!ast.bg) {
                while (true) {
                    shell_block(false);

                    bool job_done;
                    job_event *jev;

                    while ((jev = pop_job_event())) {
                        if (jid != jev->jid) {
                            print_job_event(jev);

                        } else {
                            if (jev->type == JEXITED) {
                                job_done = true;

                            } else if (jev->type == JSTOPPED) {
                                printf("\n");
                                print_job_event(jev);
                                job_done = true;
                            }
                        }
                    }

                    if (job_done)
                        break;
                };

                if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
                    err_exit("tcsetpgrp");
            }

            if (ast.bg) {
                printf("[%d] started\n", jid);
            }

            free_pline_data(&pld);
            ps_free(&ast);

            display_prompt();
        }
    }

    return EXIT_SUCCESS;
}
