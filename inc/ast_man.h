#ifndef AST_MAN_H
#define AST_MAN_H

#include "dyn_arr.h"
#include "parser.h"
#include "runner.h"

struct job_plan {
    job_id jid;
    int exit_stat;
    size_t index;
    size_t job_n;
    ps_ast ast;
};

typedef struct job_plan job_plan;

void free_job_plans(da_plan *job_plans);
int init_job_plans(da_plan *job_plans);
void run_next_if_more(job_id jid, bool success);
void add_ast(const char *line);

#endif
