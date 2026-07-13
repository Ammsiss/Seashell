#ifndef JOBCTL_H
#define JOBCTL_H

#include <sys/types.h>

#include "dyn_arr.h"

typedef enum {
    PSTOPPED,
    PRUNNING,
    PEXITED,
} proc_stat;

struct process {
    proc_stat status;
    pid_t pid;
};

struct pgroup {
    bool fg;
    bool stopped;
    pid_t pgid;
    da_process procs;
};

typedef struct {
    da_pgroup pgroups;
} job_ctl_st;

int job_ctl_init(void);
int job_ctl_free(void);
int job_ctl_add(da_pid *pids);

#endif
