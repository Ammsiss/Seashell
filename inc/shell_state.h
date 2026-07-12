#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <unistd.h>

#include "shell_types.h"
#include "dyn_arr.h"

#define SHELL_VAR_MAX 4096

struct var_pair {
    char key[SHELL_VAR_MAX];
    char value[SHELL_VAR_MAX];
};

typedef struct {
    bool subshell;
    int tty_fd;
} sh_env;

static sh_env shell_env = { .subshell = false };

int st_add_var(var_pair *var);
int st_delete_var(char *key);
char *st_lookup_var(char *key);

typedef enum {
    PSTOPPED,
    PRUNNING,
    PEXITED,
} proc_stat;

struct process {
    proc_stat status;
    pid_t pid;
};

struct pgroup {
    bool fg;
    bool stopped;
    pid_t pgid;
    da_process procs;
};

typedef struct {
    da_pgroup pgroups;
} job_ctl_st;

int job_ctl_init(void);
int job_ctl_free(void);
int job_ctl_add(da_pid *pids);

#endif
