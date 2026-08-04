#ifndef SHELL_TYPES_H
#define SHELL_TYPES_H

#include "parser.h"

struct job_plan {
    pid_t jid;
    size_t index;
    ps_ast *ast;
};

typedef struct job_plan job_plan;

#endif
