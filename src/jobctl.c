#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <sys/wait.h>

#include "jobctl.h"
#include "utils.h"

static job_ctl_st jctl = {0};

static job_id jc_new_id(void) {
    static int id_counter = 0;
    return id_counter++;
}

static int jc_pgrp_init(jc_pgrp *pg) {
    assert(pg);
    *pg = (jc_pgrp){0};

    if (da_init(&pg->procs) == -1)
        return -1;

    return 0;
}

static void jc_pgrp_free(jc_pgrp *pg) {
    assert(pg);

    da_free(&pg->procs);

    *pg = (jc_pgrp){0};
}

static int jc_job_init(jc_job *job) {
    assert(job);
    *job = (jc_job){0};

    if (da_init(&job->pgrps) == -1)
        return -1;

    return 0;
}

static void jc_job_free(jc_job *job) {
    assert(job);

    for (size_t i = 0; i < jctl.jobs.size; ++i) {
        jc_pgrp *pgrp = &job->pgrps.data[i];
        jc_pgrp_free(pgrp);
    }

    *job = (jc_job){0};
}

int jc_init(void) {
    jctl = (job_ctl_st){0};

    if (da_job_init(&jctl.jobs) == -1)
        return -1;

    return 0;
}

void jc_free(void) {
    for (size_t i = 0; i < jctl.jobs.size; ++i) {
        jc_job *job = &jctl.jobs.data[i];
        jc_job_free(job);
    }

    jctl = (job_ctl_st){0};
}

jc_job *jc_job_lookup(job_id id) {
    for (size_t i = 0; i < jctl.jobs.size; ++i)
        if (id == jctl.jobs.data[i].job_id)
            return &jctl.jobs.data[i];

    return NULL;
}

int jc_add_pgroup(job_id id) {
}

job_id jc_create(void) {
    jc_job *job = da_push(&jctl.jobs);
    if (!job)
        return -1;

    job->job_id = jc_new_id();

    return job->job_id;
}

int jc_wait_for_job(job_id id) {
}

int jc_wait_for_all() {
}

