#define _GNU_SOURCE

#include <assert.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <stdlib.h>
#include <dirent.h>
#include <dyn_str.h>
#include <sys/statfs.h>
#include <linux/magic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <valgrind/valgrind.h>

#include "builtins.h"
#include "utils.h"
#include "executor.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"
#include "shell_state.h"

void verify_fd_count(int exp_pfd_n) {
    char dir_prefix[PATH_MAX];
    snprintf(dir_prefix, PATH_MAX, "/proc/%d/fd/", getpid());

    DIR *dir = xopendir(dir_prefix);
    if (!dir)
        return;

    d_str fd_entry;
    if (d_str_init(&fd_entry) == -1)
        return;

    struct statfs sfsb;
    struct dirent *dirent;

    int pfd_n = 0;
    int total_n = 0;

    while (true) {
        errno = 0;
        dirent = xreaddir(dir);
        if (!dirent) {
            if (errno != 0) {
                goto fail;
            } else
                break;
        }

        if (strcmp(".", dirent->d_name) == 0 ||
            strcmp("..", dirent->d_name) == 0)
            continue;

        if (d_strcpy(&fd_entry, dir_prefix) == -1)
            goto fail;
        if (d_strcat(&fd_entry, dirent->d_name) == -1)
            goto fail;

        if (xstatfs(fd_entry.c_str, &sfsb) == -1)
            goto fail;

        if (sfsb.f_type == PIPEFS_MAGIC)
            ++pfd_n;

        ++total_n;
    }

    if (exp_pfd_n != pfd_n)
        LOG_WARN("unexpected pfd count (exp %d got %d)", exp_pfd_n, pfd_n);

    if (6 != total_n) /* expected: logfd + opendir + 3 io nums + tty_fd */
        LOG_WARN("unexpected fd count: (exp 7 got %d)", total_n);
fail:
    xclosedir(dir);
    d_str_free(&fd_entry);
}

void verify_pline_child_fds(bool first, bool last) {
    if (first && last)
        verify_fd_count(0);
    else if ((!first && last) || (first && !last))
        verify_fd_count(1);
    else if (!first && !last)
        verify_fd_count(2);
}

static int wait_for_pids(da_pid *pids, int *status) {
    int wstat;
    int last_status;

    for (size_t i = 0; i < pids->size; ++i) {
        pid_t pid = pids->data[i];

        if (xwaitpid(pid, &wstat, WUNTRACED) == -1) {
            errno_msg("waitpid");
            goto fail;
        }

        if (i == pids->size - 1)
            last_status = WEXITSTATUS(wstat);

        if (WIFSIGNALED(wstat)) {
            int signum = WTERMSIG(wstat);
            fprintf(stderr, "process %d terminated by signal %d (%s)",
                    pid, signum, strsignal(signum));
#ifdef WCOREDUMP
            if (WCOREDUMP(wstat))
                printf(" (core dumped)");
#endif
            printf("\n");

            last_status = 128 + signum;
        } else if (WIFSTOPPED(wstat)) {
            printf("seashell: process %d stopped\n", pid);
#ifdef WIFCONTINUED
        } else if (WIFCONTINUED(wstat)) {
            printf("seashell: process %d continued\n", pid);
#endif
        } else if (!WIFEXITED(wstat))
            goto fail;
    }

    da_free(pids);
    *status = last_status;
    return 0;

fail:
    da_free(pids);
    return -1;
}

static int move_fd(int fd1, int fd2) {
    if (fd1 == fd2)
        return 0;

    if (xdup2(fd1, fd2) == -1) {
        errno_msg("dup2");
        return -1;
    }

    if (xclose(fd1) == -1) {
        errno_msg("close");
        return -1;
    }

    return 0;
}

typedef struct {
    const ps_pline *pline;
    const ps_cmd *cur_cmd;
    int next_pipe[2];
    int prev_read_fd;
    bool first;
    bool last;
    int inputfd;
    int outputfd;
} pline_st;

static void child_fd_setup(const pline_st *pline_st) {
    /* Set up file descriptors */
    if (pline_st->first) {
        if (move_fd(pline_st->inputfd, STDIN_FILENO) == -1)
            _exit(EXIT_FAILURE);
    } else if (!pline_st->first)
        if (move_fd(pline_st->prev_read_fd, STDIN_FILENO) == -1)
            _exit(EXIT_FAILURE);

    if (pline_st->last) {
        if (move_fd(pline_st->outputfd, STDOUT_FILENO) == -1)
            _exit(EXIT_FAILURE);
    } else if (!pline_st->last) {
        if (move_fd(pline_st->next_pipe[1], STDOUT_FILENO) == -1)
            _exit(EXIT_FAILURE);

        if (close(pline_st->next_pipe[0]) == -1)
            err_exit(true, "close");
    }

    if (!RUNNING_ON_VALGRIND) /* valgrind opens fds */
        verify_pline_child_fds(pline_st->first, pline_st->last);
}

void child_redir_setup(const pline_st *pst) {
    /* Set up redirects */
    for (size_t i = 0; i < pst->cur_cmd->redirs.size; ++i) {
        ps_redir *redir = &pst->cur_cmd->redirs.data[i];
        const char *arg = redir->target.arg;

        int rfd;

        if (redir->io_num == STDIN_FILENO) {
            rfd = open(arg, O_RDONLY);
            if (rfd == -1)
                err_exit(true, "open");
        } else {
            if (redir->append) {
                rfd = open(arg, O_WRONLY | O_CREAT | O_EXCL, 0600);
                if (rfd == -1) {
                    if (errno == EEXIST) {
                        rfd = open(arg, O_WRONLY | O_APPEND);
                        if (rfd == -1)
                            err_exit(true, "open");
                    } else
                        err_exit(true, "open");
                }
            } else {
                rfd = open(arg, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if (rfd == -1)
                    err_exit(true, "open");
            }
        }

        move_fd(rfd, redir->io_num);
    }

    if (!RUNNING_ON_VALGRIND) /* valgrind opens fds */
        verify_pline_child_fds(pst->first, pst->last);
}

static void child_exec(const pline_st *pst) {
    xexecvp(pst->cur_cmd->argv[0], pst->cur_cmd->argv);

    if (errno == ENOENT) {
        err_msg("command not found: %s", pst->cur_cmd->argv[0]);
        _exit(127);
    } else
        err_exit(true, "execvp");
}

static int exec_pline(pline_st *pst, da_pid *pids) {
    if (da_init(pids) == -1) {
        LOG_ERR("da_init");
        goto fail;
    }

    pid_t pipeline_pgid;

    for (size_t i = 0; i < pst->pline->cmds.size; ++i) {
        pst->cur_cmd = &pst->pline->cmds.data[i];
        pst->first = (i == 0);
        pst->last = (i == pst->pline->cmds.size - 1);

        if (!pst->last) {
            if (xpipe(pst->next_pipe) == -1) {
                errno_msg("pipe");
                goto fail;
            }
        }

        int child_pid = xfork();
        if (child_pid == -1) {
            errno_msg("fork");
            goto fail;
        }

        if (pst->first)
            pipeline_pgid = child_pid;

        /********** CHILD START *****************/
        if (child_pid == 0) {
            get_env()->subshell = true;

            if (xsetpgid(0, pipeline_pgid) == -1)
                err_exit(true, "setpgid");

            if (pst->first)
                if (xtcsetpgrp(get_env()->tty_fd, getpgrp()) == -1)
                    err_exit(true, "tcsetpgrp");

            child_fd_setup(pst);
            child_redir_setup(pst);

            int status;
            if (try_run_builtin(pst->cur_cmd->argv, &status))
                _exit(status);

            child_exec(pst);
            _exit(EXIT_FAILURE);
        }
        /********** CHILD END *******************/

        pid_t *pid = da_push(pids);
        if (!pid) {
            err_msg("da_push");
            LOG_ERR("da_push");
            goto fail;
        }
        *pid = child_pid;

        if (xsetpgid(child_pid, pipeline_pgid) == -1) {
            if (errno != EACCES) {
                errno_msg("setpgid");
                goto fail;
            }
        }

        if (pst->first) {
            if (xtcsetpgrp(get_env()->tty_fd, pipeline_pgid) == -1) {
                errno_msg("tcsetpgrp");
                goto fail;
            }
        }

        if (!pst->first)
            if (xclose(pst->prev_read_fd) == -1) {
                errno_msg("close");
                goto fail;
            }

        if (!pst->last) {
            pst->prev_read_fd = pst->next_pipe[0];

            if (xclose(pst->next_pipe[1]) == -1) {
                errno_msg("close");
                goto fail;
            }
        }
    }

    if (!RUNNING_ON_VALGRIND)
        verify_fd_count(0);
    return 0;

fail:
    da_free(pids);
    return -1;
}

static bool run_pline(const ps_pline *pline, int inputfd, int outputfd) {
    pline_st pst = {0};
    pst.pline = pline;
    pst.cur_cmd = &pline->cmds.data[0];
    pst.inputfd = inputfd;
    pst.outputfd = outputfd;

    int status = {0};

    if (pst.pline->cmds.size == 1)
        if (try_run_builtin(pst.cur_cmd->argv, &status))
            return status == EXIT_SUCCESS;

    da_pid pids;
    if (exec_pline(&pst, &pids) == -1)
        return false;

    if (wait_for_pids(&pids, &status) == -1)
        fatal("fatal: bad job control state");

    /* restore fg pgroup status; SIGTTOU must be blocked/ignored */
    if (xtcsetpgrp(get_env()->tty_fd, getpgrp()) == -1)
        errExit(true, "tcsetpgrp");

    return status == EXIT_SUCCESS;
}

void sh_run(const ps_job *job, int inputfd, int outputfd) {
    bool pline_success = true;

    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];

        if (andor->op == PS_OR_IF && pline_success)
            continue;

        if (andor->op == PS_AND_IF && !pline_success)
            continue;

        pline_success = run_pline(&andor->pline, inputfd, outputfd);
    }
}
