#include <assert.h>

#include "jobctl.h"

static job_ctl_st jctl = {0};

static int pgroup_init(pgroup *pg) {
    assert(pg);
    *pg = (pgroup){0};
    if (da_init(&pg->procs) == -1)
        return -1;
    return 0;
}

static int pgroup_free(pgroup *pg) {
    assert(pg);
    da_free(&pg->procs);
    *pg = (pgroup){0};
    return 0;
}

int job_ctl_init(void) {
    if (da_init(&jctl.pgroups) == -1)
        return -1;
    return 0;
}

int job_ctl_free(void) {
    for (size_t i = 0; i < jctl.pgroups.size; ++i)
        pgroup_free(&jctl.pgroups.data[i]);
    da_free(&jctl.pgroups);
    return 0;
}

int job_ctl_add(da_pid *pids) {
    pgroup *pg = da_push(&jctl.pgroups);
    if (!pg)
        return -1;
    if (pgroup_init(pg) == -1)
        return -1;

    for (size_t i = 0; i < pids->size; ++i) {
        process *p = da_push(&pg->procs);
        if (!p)
            return -1;
        p->pid = pids->data[i];
        p->status = PRUNNING;
    }

    return 0;
}
