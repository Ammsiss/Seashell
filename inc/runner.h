#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <assert.h>

#include "parser.h"

typedef enum {
    PEXITED,
    PSTOPPED,
    PRUNNING,
} pstat;

typedef int job_id;
typedef struct jc_proc jc_proc;
typedef struct jc_pgrp jc_pgrp;
typedef struct jc_job jc_job;

struct jc_proc {
    pid_t pid;
    pstat stat;
    int wstat;
    jc_pgrp *pgrp;
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

int init_jst(jc_jst *jctl);
void free_jst(jc_jst *jctl);

int sighup_shutdown(void);

int jctl_wait(job_id *jid);
char *get_pid_string(job_id jid);

void sh_run_job(const ps_pline *pline, bool bg);
void sh_run(const ps_ast *ast);

#endif
