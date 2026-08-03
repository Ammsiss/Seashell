#define _GNU_SOURCE

#include <stdio.h>

#include "utils.h"
#include "builtins.h"
#include "log.h"
#include "parser.h"
#include "sig_funcs.h"
#include "exec_funcs.h"

void move_fd(int fd1, int fd2) {
    if (fd1 == fd2)
        return;

    if (xdup2(fd1, fd2) == -1)
        err_exit("dup2");

    if (xclose(fd1) == -1)
        err_exit("close");
}

void child_fd_setup(bool first, bool last, int next_pipe[2], int prev_rfd) {
    /* Set up file descriptors */
    if (!first)
        move_fd(prev_rfd, STDIN_FILENO);

    if (!last) {
        move_fd(next_pipe[1], STDOUT_FILENO);

        if (close(next_pipe[0]) == -1) /* why here? */
            err_exit("close");
    }

}

void child_redir_setup(da_redir *redirs) {
    for (size_t i = 0; i < redirs->size; ++i) {
        ps_redir *redir = &redirs->data[i];
        const char *arg = redir->target.arg;

        int rfd;

        if (redir->io_num == STDIN_FILENO) {
            rfd = open(arg, O_RDONLY);
            if (rfd == -1)
                err_exit("open");
        } else {
            if (redir->append) {
                rfd = open(arg, O_WRONLY | O_CREAT | O_EXCL, 0600);
                if (rfd == -1) {
                    if (errno == EEXIST) {
                        rfd = open(arg, O_WRONLY | O_APPEND);
                        if (rfd == -1)
                            err_exit("open");
                    } else
                        err_exit("open");
                }
            } else {
                rfd = open(arg, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if (rfd == -1)
                    err_exit("open");
            }
        }

        move_fd(rfd, redir->io_num);
    }
}

void free_pline_data(pline_data *pld) {
    da_free(pld->pids);

    free(pld->pids);

    *pld = (pline_data){0};
}

void init_pline_data(pline_data *pld) {
    *pld = (pline_data){0};

    da_pid *pids = xmalloc(sizeof(da_pid));
    if (!pids)
        err_exit("malloc");

    if (da_init(pids) == -1)
        xfatal("da_init");

    pld->pids = pids;
}

pline_data exec_pline(const ps_pline *pline, bool bg) {
    if (!pline)
        assert(pline);

    int next_pipe[2];
    int prev_rfd;

    pline_data pld = {0};
    init_pline_data(&pld);

    for (size_t i = 0; i < pline->cmds.size; ++i) {
        ps_cmd *cur_cmd = &pline->cmds.data[i];
        bool first = (i == 0);
        bool last = (i == pline->cmds.size - 1);

        if (!last)
            if (xpipe(next_pipe) == -1)
                err_exit("pipe");

        int cpid = xfork();
        if (cpid == -1)
            err_exit("fork");

        if (first)
            pld.pgid = cpid;

        if (cpid == 0) {
            sh_env.subshell = true;

            /* subshell set up */

            if (xsetpgid(0, pld.pgid) == -1)
                err_exit("setpgid");

            if (!bg && first)
                if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
                    err_exit("tcsetpgrp");

            child_fd_setup(first, last, next_pipe, prev_rfd);
            child_redir_setup(&cur_cmd->redirs);

            sig_restore();

            /* is it a builtin? */

            int status;
            if (try_run_builtin(cur_cmd->argv, &status))
                _exit(status);

            /* exec program */

            xexecvp(cur_cmd->argv[0], cur_cmd->argv);

            if (errno == ENOENT) {
                err_msg("command not found: %s", cur_cmd->argv[0]);
                _exit(127);
            } else {
                err_exit("execvp");
            }

            _exit(EXIT_FAILURE);
        }

        if (xsetpgid(cpid, pld.pgid) == -1 && errno != EACCES)
            err_exit("setpgid");

        if (!bg && first)
            if (xtcsetpgrp(sh_env.tty_fd, pld.pgid) == -1)
                err_exit("tcsetpgrp");

        if (!first)
            if (xclose(prev_rfd) == -1)
                err_exit("close");

        if (!last) {
            prev_rfd = next_pipe[0];
            if (xclose(next_pipe[1]) == -1)
                err_exit("close");
        }

        pid_t *pid = da_push(pld.pids);
        if (!pid)
            xfatal("da_push");

        *pid = cpid;
    }

    return pld;
}
