#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "executor_types.h"
#include "parser_types.h"

#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))

#define SH_OK 0
#define SH_FAIL 1

struct sh_result {
    sh_errcode err_code;
    int exit_code;
    char msg[256];
};

sh_result sh_run(const ps_job *job);

#endif
