#define _GNU_SOURCE

#include <stdio.h> // IWYU pragma: keep

#include "shell_state.h"
#include "log.h"
#include "ast_man.h"
#include "dyn_arr.h"
#include "parser.h"
#include "expander.h"
#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "runner.h"

void free_job_plans(da_plan *job_plans) {
    for (size_t i = 0; i < job_plans->size; ++i)
        ps_free(&job_plans->data[i].ast);

    da_free(job_plans);

    *job_plans = (da_plan){0};
}

int init_job_plans(da_plan *job_plans) {
    *job_plans = (da_plan){0};

    if (da_init(job_plans) == -1)
        xfatal("da_init");

    return 0;
}

static job_plan *register_plan(const char *line) {
    if (line && *line == '\0')
        return NULL;

    job_plan *plan = da_push(&sh_env.job_plans);
    if (!plan)
        xfatal("da_push");

    da_tok toks;
    lx_status lexer_status = lx_tokenize(line, &toks);
    if (lexer_status != LX_OK) {
        err_msg("lexer: %s\n", lx_errstr(lexer_status));
        goto fail;
    }

    if (ps_parse(&toks, &plan->ast) == -1) {
        err_msg("parse error\n");
        lx_free(&toks);
        goto fail;
    }

    if (ex_expand(&plan->ast) == -1) {
        err_msg("expand error\n");
        lx_free(&toks);
        ps_free(&plan->ast);
        goto fail;
    }

    plan->index = 0;
    plan->job_n = plan->ast.andors.size;

    lx_free(&toks);

    return plan;

fail:
    if (da_delete(&sh_env.job_plans, sh_env.job_plans.size - 1) == -1)
        xfatal("da_delete");

    return NULL;
}

static int remove_plan(int plan_id) {
    for (size_t i = 0; i < sh_env.job_plans.size; ++i) {
        if (plan_id == sh_env.job_plans.data[i].jid) {
            ps_free(&sh_env.job_plans.data[i].ast);
            if (da_delete(&sh_env.job_plans, i) == -1)
                xfatal("da_delete");
            return 0;
        }
    }

    return -1;
}

static void run_next(job_plan *plan, bool success) {
    if (plan->index < plan->job_n) {
        if (plan->index == 0) {
            LOG_INFO("[plan new] running at index=%ld", plan->index);
        } else
            LOG_INFO("[plan %d] running at index=%ld", plan->jid, plan->index);

        ps_andor *andor = &plan->ast.andors.data[plan->index];
        ++plan->index;

        if (andor->op == PS_AND_IF && !success)
            return;
        if (andor->op == PS_OR_IF && success)
            return;

        sh_run_job(&andor->pline, plan->ast.bg, &plan->jid);

    } else {
        remove_plan(plan->jid);
        LOG_INFO("[plan %d] removed with index=%ld", plan->jid, plan->index);
    }
}

job_plan *lookup_plan_by_jid(job_id jid) {
    for (size_t i = 0; i < sh_env.job_plans.size; ++i)
        if (jid == sh_env.job_plans.data[i].jid)
            return &sh_env.job_plans.data[i];

    return NULL;
}

void run_next_if_more(job_id jid, bool success) {
    LOG_INFO("looking up [plan %d]", jid);

    job_plan *plan = lookup_plan_by_jid(jid);
    if (!plan)
        return;

    run_next(plan, success);
}

void add_ast(const char *line) {
    job_plan *plan = register_plan(line);
    if (!plan)
        return;

    run_next(plan, true);

    return;
}
