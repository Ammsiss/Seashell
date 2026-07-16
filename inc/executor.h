#ifndef RUNNER_H
#define RUNNER_H

#include "dyn_arr.h"
#include "parser.h"

int exec_pline(const ps_pline *pline, bool bg, da_pid *pids, pid_t *pgid);

#endif
