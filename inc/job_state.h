#ifndef JOB_STATE_H
#define JOB_STATE_H

#include "dyn_arr.h"

typedef enum {
    JOB_EXIT,
    JOB_STOP,
    JOB_RUN,
} job_stat;

typedef enum {
    PROC_EXIT,
    PROC_STOP,
    PROC_RUN,
} proc_stat;

typedef enum {
    JOB_EXITED,
    JOB_STOPPED,
    JOB_CONTINUED,
} job_event_type;

typedef struct jc_proc jc_proc;
typedef struct jc_pgrp jc_pgrp;
typedef struct jc_job jc_job;
typedef struct job_table job_table;
typedef struct job_event job_event;

struct jc_proc {
    pid_t pid;
    proc_stat stat;
    int wstat;
};

struct jc_pgrp {
    pid_t pgid;
    da_proc procs;
};

struct jc_job {
    pid_t jid;
    jc_pgrp pgrp;
    job_stat stat;
};

struct job_table {
    da_job jobs;
};

struct job_event {
    pid_t jid;
    job_event_type type;
};

void get_job_events(da_jevent *jevs);

#endif
