#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

int wait_for_all(void);
void sh_run(const ps_job *job);

#endif
