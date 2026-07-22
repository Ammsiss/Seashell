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

struct jc_job {
    pid_t pgid;
    job_id id;
    pstat stat;
};

typedef struct jc_job jc_job;

struct jc_jst {
    da_job jobs;
};

typedef struct jc_jst jc_jst;

void jc_init(jc_jst *jctl);
void jc_free(jc_jst *jctl);

jc_job *lookup_job_by_id(job_id jid);
int sighup_shutdown(void);

int jc_wait(void);
void sh_run(const ps_ast *ast);

#endif
