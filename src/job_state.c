#define _GNU_SOURCE

#include <stdio.h>

#include "log.h"
#include "job_state.h"
#include "wait_stat.h"

static job_table jctl = {0};
static da_jevent jevs = {0};

static void init_job(jc_job *job) {
    *job = (jc_job){0};

    if (da_init(&job->pgrp.procs) == -1)
        xfatal("da_init");
}

static void free_job(jc_job *job) {
    da_free(&job->pgrp.procs);
    *job = (jc_job){0};
}

static pid_t create_job_id(void) {
    pid_t jid = 1;

    if (jctl.jobs.size == 0)
        return jid;

    for (size_t i = 0; i < jctl.jobs.size; ++i) {
        if (jid == jctl.jobs.data[i].jid) {
            ++jid;
            continue;
        }
    }

    return jid;
}

static job_stat calc_job_stat(jc_job *job) {
    bool stopped = false;

    for (size_t i = 0; i < job->pgrp.procs.size; ++i) {

        jc_proc *proc = &job->pgrp.procs.data[i];

        if (proc->stat == PROC_RUN)
            return JOB_RUN;

        if (proc->stat == PROC_STOP)
            stopped = true;
    }

    return stopped ? JOB_STOP : JOB_EXIT;
}

static bool identify_proc(pid_t pid, jc_job **job, jc_proc** proc) {
    assert(job && proc);

    for (size_t i = 0; i < jctl.jobs.size; ++i) {
        jc_job *in_job = &jctl.jobs.data[i];

        for (size_t y = 0; y < in_job->pgrp.procs.size; ++y) {
            jc_proc *in_proc = &in_job->pgrp.procs.data[y];

            if (pid == in_proc->pid) {
                *proc = in_proc;
                *job = in_job;

                return true;
            }
        }
    }

    return false;
}

job_table *get_jctl(void) {
    return &jctl;
}

size_t get_job_index(pid_t jid) {
    for (size_t i = 0; i < jctl.jobs.size; ++i)
        if (jid == jctl.jobs.data[i].jid)
            return i;

    xfatal("tried to get job index of non-existant job\n");
}

void push_job_exit_event(pid_t jid) {
    job_event *jev = da_push(&jevs);
    if (!jev)
        xfatal("da_push");

    jev->jid = jid;
    jev->type = JEXITED;
}

void push_job_start_event(pid_t jid) {
    job_event *jev = da_push(&jevs);
    if (!jev)
        xfatal("da_push");

    jev->jid = jid;
    jev->type = JSTARTED;
}

void push_job_stop_event(pid_t jid) {
    job_event *jev = da_push(&jevs);
    if (!jev)
        xfatal("da_push");

    jev->jid = jid;
    jev->type = JSTOPPED;
}

void push_job_cont_event(pid_t jid) {
    job_event *jev = da_push(&jevs);
    if (!jev)
        xfatal("da_push");

    jev->jid = jid;
    jev->type = JCONTINUED;
}

job_event *pop_job_event(void) {
    static job_event jev;

    if (jevs.size == 0)
        return NULL;

    jev.jid = jevs.data[0].jid;
    jev.type = jevs.data[0].type;

    if (da_delete(&jevs, 0) == -1)
        xfatal("da_delete");

    return &jev;
}

void update_job_table(void) {

    da_wevent wevs;
    get_wstats(&wevs);

    jc_job *job;
    jc_proc *proc;

    for (size_t i = 0; i < wevs.size; ++i) {
        wait_event *wev = &wevs.data[i];

        identify_proc(wev->pid, &job, &proc);

        if (wev->type == PEXITED || wev->type == PSIGNALED) {
            proc->stat = PROC_EXIT;
        }

        else if (wev->type == PSTOPPED) {
            proc->stat = PROC_STOP;
        }

        else if (wev->type == PCONTINUED) {
            proc->stat = PROC_RUN;
        }

        job_stat stat = calc_job_stat(job);

        if (job->stat == JOB_RUN && stat == JOB_STOP) {
            push_job_stop_event(job->jid);
            job->stat = stat;

        } else if (job->stat == JOB_STOP && stat == JOB_RUN) {
            push_job_cont_event(job->jid);
            job->stat = stat;

        } else if (stat == JOB_EXIT) {
            push_job_exit_event(job->jid);
            job->stat = stat;

            size_t i = get_job_index(job->jid);

            free_job(&jctl.jobs.data[i]);

            if (da_delete(&jctl.jobs, i) == -1)
                xfatal("da_delete");
        }
    }

    da_free(&wevs);
}

pid_t add_job(da_pid *pids, pid_t pgid) {
    jc_job *job = da_push(&jctl.jobs);
    if (!job)
        xfatal("da_push");

    init_job(job);

    job->jid = create_job_id();
    job->stat = JOB_RUN;
    job->pgrp.pgid = pgid;

    for (size_t i = 0; i < pids->size; ++i) {
        jc_proc *proc = da_push(&job->pgrp.procs);
        if (!proc)
            xfatal("da_push");

        proc->pid = pids->data[i];
        proc->stat = PROC_RUN;

        if (i == pids->size - 1)
            job->last = proc;
    }

    job_event *jev = da_push(&jevs);
    if (!jev)
        xfatal("da_push");

    jev->jid = job->jid;
    jev->type = JSTARTED;

    return job->jid;
}
