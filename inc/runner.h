#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <assert.h>

#include "dyn_str.h"
#include "parser.h"

typedef enum {
    PEXIT,
    PSTOP,
    PRUN,
} pstat;

typedef int job_id;
typedef struct jc_proc jc_proc;
typedef struct jc_pgrp jc_pgrp;
typedef struct jc_job jc_job;

struct jc_proc {
    pid_t pid;
    pstat stat;
    int exit_stat;
    int success;
    jc_pgrp *pgrp;
    d_str cmd;
};

struct jc_pgrp {
    da_proc procs;
    pid_t pgid;
    jc_job *job;
};

struct jc_job {
    jc_pgrp pgrp;
    job_id id;
    pstat stat;
};

typedef struct {
    da_job jobs;
} jc_jst;

jc_proc *add_proc(jc_pgrp *pgrp, char **argv);
int init_jst(jc_jst *jctl);
void free_jst(jc_jst *jctl);

jc_job *lookup_job(job_id jid, size_t *index);
int sighup_shutdown(void);

int jctl_wait(job_id *jid);
char *get_cmd_string(job_id jid);
char *get_pid_string(job_id jid);

void sh_run_job(const ps_pline *pline, bool bg);
void sh_run(const ps_ast *ast);

#endif
