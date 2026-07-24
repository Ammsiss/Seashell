#define _GNU_SOURCE

#include <stdio.h>

#include "noti.h"
#include "log.h"
#include "runner.h"
#include "utils.h"

char *get_pid_string(job_id jid) {
    jc_job *job = lookup_job(jid, NULL);
    if (!job)
        return NULL;

    d_str pid_str;
    if (d_str_init(&pid_str) == -1)
        return NULL;

    char buf[4096]; /* surely not longer then this */

    for (size_t i = 0; i < job->pgrp.procs.size; ++i) {
        jc_proc *proc = &job->pgrp.procs.data[i];

        if (i != 0) {
            snprintf(buf, 4096, " %d", proc->pid);
        } else
            snprintf(buf, 4096, "%d", proc->pid);

        if (d_strcat(&pid_str, buf) == -1)
            goto fail;
    }

    return pid_str.c_str;

fail:
    d_str_free(&pid_str);
    return NULL;
}

char *get_cmd_string(job_id jid) {
    jc_job *job = lookup_job(jid, NULL);
    if (!job)
        return NULL;

    d_str cmd_str;
    if (d_str_init(&cmd_str) == -1)
        return NULL;

    for (size_t i = 0; i < job->pgrp.procs.size; ++i) {
        jc_proc *proc = &job->pgrp.procs.data[i];

        if (i != 0)
            if (d_strcat(&cmd_str, " | ") == -1)
                goto fail;

        if (d_strcat(&cmd_str, proc->cmd.c_str) == -1)
            goto fail;
    }

    return cmd_str.c_str;

fail:
    d_str_free(&cmd_str);
    return NULL;
}

void noti_job_start(job_id jid, const char *pid_str, const char *cmd_str) {
    printf("[%d] %s  %s\n", jid, pid_str, cmd_str);
    LOG_INFO("[%d] %s  %s", jid, pid_str, cmd_str);
}

void noti_job_stop(job_id jid,  const char *cmd_str) {
    printf("[%d] stopped %s\n", jid, cmd_str);
    LOG_INFO("[%d] stopped  %s", jid, cmd_str);
}

void noti_job_done(job_id jid, const char *cmd_str) {
    printf("[%d] done %s\n", jid, cmd_str);
    LOG_INFO("[%d] done  %s", jid, cmd_str);
}

void noti_job_resume(job_id jid, const char *cmd_str) {
    printf("[%d] %s\n", jid, cmd_str);
    LOG_INFO("[%d] resumed  %s", jid, cmd_str);
}

/* NOTE: this relies on the job not being removed by the time
 * this function is called. In the future perhaps we should not have
 * that reliance (pid and cmd strings should be stored elsewhere)
 * and the events themselves could contain the needed info. We
 * could also set up reference counting for the jobs <-> consumers */

bool noti_jobs(jc_jst *jctl, bool from_sig) {
    da_job *jobs = &jctl->jobs;
    bool sent_msg = false;

    for (size_t i = 0; i < jobs->size; ++i) {
        sent_msg = jobs->data[i].ev != 0;

        char *pid_str = get_pid_string(jobs->data[i].jid);
        if (!pid_str)
            xfatal("get_pid_string");

        char *cmd_str = get_cmd_string(jobs->data[i].jid);
        if (!cmd_str)
            xfatal("get_cmd_string");

        if (JSTARTED & jobs->data[i].ev) {
            if (from_sig)
                printf("\n");
            noti_job_start(jobs->data[i].jid, pid_str, cmd_str);
            jobs->data[i].ev &= ~JSTARTED;
        }

        if (JRESUMED & jobs->data[i].ev) {
            if (from_sig)
                printf("\n");
            noti_job_resume(jobs->data[i].jid, pid_str);
            jobs->data[i].ev &= ~JRESUMED;
        }

        if (JSTOPPED & jobs->data[i].ev) {
            if (from_sig)
                printf("\n");
            noti_job_stop(jobs->data[i].jid, pid_str);
            jobs->data[i].ev &= ~JSTOPPED;
        }

        /* stat is killing the job before it can get here */
        if (jobs->data[i].stat == PEXIT) {
            if (from_sig)
                printf("\n");
            noti_job_done(jobs->data[i].jid, pid_str);
            sent_msg = true;
        }

        free(pid_str);
        free(cmd_str);
    }

    return sent_msg;
}

