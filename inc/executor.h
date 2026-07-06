#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "executor_types.h"
#include "parser_types.h"

#define SH_OK 0
#define SH_FAIL 1
#define SH_EXIT 2

#define BUILTIN_COUNT 3

#define BUF_SIZE 1024

struct sh_env {
    bool subshell;
};

struct sh_builtin {
    const char *name;
    builtin_func func;
};

void sh_run(const ps_job *job, int inputfd, int outputfd);

#endif
