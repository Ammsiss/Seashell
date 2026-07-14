#ifndef JOBCTL_H
#define JOBCTL_H

#include <sys/types.h>

#include "dyn_arr.h"

typedef enum {
    PSTOPPED,
    PRUNNING,
    PEXITED,
} proc_stat;

struct proc {
    proc_stat status;
    pid_t pid;
};

struct jc_pgrp {
    bool fg;
    bool stopped;
    pid_t pgid;
    da_proc procs;
};

struct jc_job {
    job_id job_id;
    da_pgrp pgrps;
};

typedef struct {
    da_job jobs;
} job_ctl_st;

int jc_init(void);
void jc_free(void);

job_id jc_create(void);

int jc_wait_for_job(job_id id);
int jc_wait_for_all();

#endif
