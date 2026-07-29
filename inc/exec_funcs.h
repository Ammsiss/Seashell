#ifndef EXEC_FUNCS_H
#define EXEC_FUNCS_H

#define _GNU_SOURCE

#include "parser.h"

struct pline_data {
    pid_t pgid;
    da_pid *pids;
};

typedef struct pline_data pline_data;

void init_pline_data(pline_data *pld);
void free_pline_data(pline_data *pld);

void move_fd(int fd1, int fd2);
void child_fd_setup(bool first, bool last, int next_pipe[2], int prev_rfd);
void child_redir_setup(da_redir *redirs);

pline_data exec_pline(const ps_pline *pline, bool bg);

#endif
