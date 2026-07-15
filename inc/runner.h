#ifndef RUNNER_H
#define RUNNER_H

#include "dyn_arr.h"

typedef struct {
    da_pid pids;
    pid_t pgid;
} pline_info;

pline_info *run_pline(ps_pline *pline, bool bg);

#endif
