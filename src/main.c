#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <wait.h>

#include "shell_types.h"
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

static da_plan plans = {0};

static ps_andor *plan_next(job_plan *plan) {
    return &plan->ast->andors.data[plan->index];
}

static ps_ast *line_to_ast(void) {
    ps_ast *ast = xmalloc(sizeof(ps_ast));
    if (!ast)
        err_exit("malloc");

    char *line = get_line();
    da_tok toks = {0};

    if (lx_tokenize(line, &toks) == -1)
        xfatal("lx_tokenize");

    if (ps_parse(&toks, ast) == -1)
        xfatal("lx_tokenize");

    if (ex_expand(ast) == -1)
        xfatal("lx_tokenize");

    lx_free(&toks);

    return ast;
}

static void add_plan(pid_t jid, ps_ast *ast) {
    LOG_INFO("added plan with initial jid=%d", jid);

    job_plan *plan = da_push(&plans);
    if (!plan)
        xfatal("da_push");

    plan->jid = jid;
    plan->index = 1;
    plan->ast = ast;
}

static void remove_plan(size_t index) {
    LOG_INFO("Removed plan with jid=%d", plans.data[index].jid);

    ps_free(plans.data[index].ast);
    free(plans.data[index].ast);

    if (da_delete(&plans, index) == -1)
        xfatal("da_delete");
}

static void run_next_job_in_plan(job_event *jev) {
    for (size_t i = 0; i < plans.size;) {
        if (jev->jid == plans.data[i].jid) {
            job_plan *plan = &plans.data[i];

            plan->jid = create_job_id();

            pline_data pld = exec_pline(&plan_next(plan)->pline, plan->ast->bg);
            add_job(plan->jid, pld.pids, pld.pgid);
            free_pline_data(&pld);

            if (++plan->index >= plan->ast->andors.size) {
                remove_plan(i);
                continue;
            }
        }

        ++i;
    }
}

static void print_job_event(job_event *jev, bool *prompt_upset) {
    assert(jev && prompt_upset);

    if (!*prompt_upset) {
        printf("\n");
        *prompt_upset = true;
    }

    printf("%s\n", get_jev_str(*jev));
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

void reclaim_terminal(void) {
    if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
        err_exit("tcsetpgrp");

    sh_env.fg_jid = NOFG;
}

bool fg_event(job_event *jev) {
    return sh_env.fg_jid  == jev->jid;
}

static void process_signals(void) {
    bool need_prompt = false;
    bool prompt_upset = !shell_in_fg();

    if (sigchld_caught) {
        LOG_INFO("sigchild caught");

        wait_event wev;
        while (get_wstat(&wev) != -1)
            if (update_job_proc(wev) == -1)
                xfatal("update_job_table");

        job_event *jev;
        while ((jev = pop_job_event())) {

            if (jev->type == JEXITED)
                run_next_job_in_plan(jev);

            if (!fg_event(jev)) {
                print_job_event(jev, &prompt_upset);

            } else if (jev->type == JSTOPPED || jev->type == JEXITED) {
                reclaim_terminal();
                need_prompt = true;

                if (jev->type == JSTOPPED) {
                    printf("\n");
                    print_job_event(jev, &prompt_upset);
                }
            }
        }
    }

    if (sigint_caught) {
        LOG_INFO("sigint caught");

        if (!prompt_upset) {
            printf("\n");
            prompt_upset = true;
        }
    }

    if (sighup_caught) {
        LOG_INFO("sighup caught");
        exit(EXIT_FAILURE);
    }

    if (need_prompt || (shell_in_fg() && prompt_upset))
        display_prompt(PROMPT_SIMPLE);
}

int main(void) {
    log_init("/home/juta/Projects/Seashell/logs");
    env_init();
    sig_setup();

    if (xatexit(hup_to_children) == -1)
        err_exit("atexit");

    LOG_INFO("seashell PID(%d)", getpid());

    struct pollfd events = { .events = POLLIN, .fd = sh_env.tty_fd };

    display_prompt(PROMPT_SIMPLE);

    while (true) {
        int nfds = shell_in_fg() ? 1 : 0;
        int ready = xppoll(&events, nfds, NULL, &sh_env.og_mask);

        if (ready == -1 && errno != EINTR)
            err_exit("sigsuspend");

        if (ready == -1 && errno == EINTR) {
            process_signals();
            reset_sig_flags();
            continue;
        }

        ps_ast *ast = line_to_ast();
        if (ast->andors.size == 0)
            xfatal("parsed empty ast");

        ps_pline *first_pline = &ast->andors.data[0].pline;

        if (first_pline->cmds.size == 1 && !ast->bg) {
            if (try_run_builtin(first_pline->cmds.data[0].argv, NULL)) {
                ps_free(ast);
                free(ast);
                continue;
            }
        }

        int jid = create_job_id();

        if (ast->bg) {
            printf("[%d] started\n", jid);
            display_prompt(PROMPT_SIMPLE);
        } else
            sh_env.fg_jid = jid;

        pline_data pld = exec_pline(first_pline, ast->bg);
        add_job(jid, pld.pids, pld.pgid);
        free_pline_data(&pld);

        if (ast->andors.size > 1) {
            add_plan(jid, ast);

        } else {
            ps_free(ast);
            free(ast);
        }
    }

    return EXIT_SUCCESS;
}
