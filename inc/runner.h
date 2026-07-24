#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <assert.h>

#include "dyn_str.h"
#include "parser.h"

#define JRESUMED 1
#define JSTOPPED 2
#define JSTARTED 4

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
    bool exit_stat;
    bool success;
    jc_pgrp *pgrp;
    d_str cmd;
};

struct jc_pgrp {
    pid_t pgid;
    da_proc procs;
    jc_job *job;
};

struct jc_job {
    job_id jid;
    jc_pgrp pgrp;
    pstat stat;
    unsigned ev;
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

pstat sh_run_job(const ps_pline *pline, bool bg, job_id *jid);

#endif
