#ifndef RUNNER_H
#define RUNNER_H

#include <stdnoreturn.h>

#include "parser.h"
#include "runner.h"

struct ex_proc {
    pid_t pid;
    int exit_stat;
    pstat stat;
};

typedef struct ex_proc ex_proc;

noreturn void ex_exec(const ps_ast *ast);

#endif
