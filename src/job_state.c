#define _GNU_SOURCE

#include "log.h"
#include "job_state.h"
#include "wait_stat.h"

static job_table jctl;

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

void get_job_events(da_jevent *jevs) {

    if (da_init(jevs) == -1)
        xfatal("da_init");

    da_wevent wevs;
    get_wstats(&wevs);

    jc_job *job;
    jc_proc *proc;
    job_event *jev;

    for (size_t i = 0; i < wevs.size; ++i) {
        wait_event *wev = &wevs.data[i];

        identify_proc(wev->pid, &job, &proc);

        proc->wstat = wev->wstat;

        if (WIFEXITED(wev->wstat) || WIFSIGNALED(wev->wstat)) {
            proc->stat = PROC_EXIT;
        }

        else if (WIFSTOPPED(wev->wstat)) {
            proc->stat = PROC_STOP;
        }

        else if (WIFCONTINUED(wev->wstat)) {
            proc->stat = PROC_RUN;
        }

        job_stat stat = calc_job_stat(job);

        if (job->stat == JOB_RUN && stat == JOB_STOP) {
            jev = da_push(jevs);
            if (!jev)
                xfatal("da_push");

            jev->jid = job->jid;
            jev->type = JOB_STOPPED;

        } else if (job->stat == JOB_STOP && stat == JOB_RUN) {
            jev = da_push(jevs);
            if (!jev)
                xfatal("da_push");

            jev->jid = job->jid;
            jev->type = JOB_CONTINUED;
        }

        job->stat = stat;
    }

    da_free(&wevs);
}
