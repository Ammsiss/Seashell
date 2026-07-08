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

    if (7 != total_n) /* expected: logfd + . + .. + opendir + first 3 io nums */
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

static int wait_for_pids(da_pid *pids) {
    int wstat;
    int last_status;

    for (size_t i = 0; i < pids->size; ++i) {
        pid_t pid = pids->data[i];

        if (xwaitpid(pid, &wstat, 0) == -1) {
            errno_msg("waitpid");
            goto fail;
        }

        if (i == pids->size - 1) {
            if (WIFEXITED(wstat)) {
                last_status = WEXITSTATUS(wstat);
            } else if (WIFSIGNALED(wstat)) {
                int signum = WTERMSIG(wstat);
                fprintf(stderr, "terminated by signal %d (%s)",
                        signum, strsignal(signum));
#ifdef WCOREDUMP
                if (WCOREDUMP(wstat))
                    printf(" (core dumped)");
#endif
                printf("\n");

                last_status = 128 + signum;
            } else
                goto fail;
        }
    }

    da_free(pids);
    return last_status;

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

static int exec_pline(const ps_pline *pline, da_pid *pids, \
        int inputfd, int outputfd) {
    if (da_init(pids) == -1)
        LOG_ERR("da_init");

    pid_t child_pid;

    int next_pipe[2];
    int prev_read_fd;

    size_t cmd_cnt = pline->cmds.size;

    for (size_t i = 0; i < cmd_cnt; ++i) {
        ps_cmd *cmd = &pline->cmds.data[i];
        bool first = (i == 0);
        bool last = (i == cmd_cnt - 1);

        if (!last) {
            if (xpipe(next_pipe) == -1) {
                errno_msg("pipe");
                goto fail;
            }
        }

        child_pid = xfork();
        if (child_pid == -1) {
            errno_msg("fork");
            goto fail;
        }

        pid_t *pid = da_push(pids);
        if (!pid) {
            err_msg("da_push");
            LOG_ERR("da_push");
            goto fail;
        }
        *pid = child_pid;

        if (child_pid == 0) {
            shell_env.subshell = true;

            /* Set up file descriptors */
            if (first) {
                if (move_fd(inputfd, STDIN_FILENO) == -1)
                    _exit(EXIT_FAILURE);
            } else if (!first)
                if (move_fd(prev_read_fd, STDIN_FILENO) == -1)
                    _exit(EXIT_FAILURE);

            if (last) {
                if (move_fd(outputfd, STDOUT_FILENO) == -1)
                    _exit(EXIT_FAILURE);
            } else if (!last) {
                if (move_fd(next_pipe[1], STDOUT_FILENO) == -1)
                    _exit(EXIT_FAILURE);

                if (close(next_pipe[0]) == -1)
                    err_exit(true, "close");
            }

            /* Set up redirects */
            for (size_t i = 0; i < cmd->redirs.size; ++i) {
                ps_redir *redir = &cmd->redirs.data[i];
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

            int status;
            if (try_run_builtin(cmd->argv, &status))
                _exit(status);

            if (!RUNNING_ON_VALGRIND) /* valgrind opens fds */
                verify_pline_child_fds(first, last);

            xexecvp(cmd->argv[0], cmd->argv);

            if (errno == ENOENT) {
                err_msg("command not found: %s", cmd->argv[0]);
                _exit(127);
            } else {
                errno_msg("execvp");
                _exit(EXIT_FAILURE);
            }
        }

        if (!first)
            if (xclose(prev_read_fd) == -1) {
                errno_msg("close");
                goto fail;
            }

        if (!last) {
            prev_read_fd = next_pipe[0];

            if (xclose(next_pipe[1]) == -1) {
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
    ps_cmd *cmd = &pline->cmds.data[0];

    if (pline->cmds.size == 1) {

        int bltin_status;
        if (try_run_builtin(cmd->argv, &bltin_status))
            return bltin_status == EXIT_SUCCESS;
    }

    da_pid pids;
    if (exec_pline(pline, &pids, inputfd, outputfd) == -1)
        return false;

    int last_status = wait_for_pids(&pids);
    if (last_status == -1)
        fatal("fatal: bad job control state");

    return last_status == EXIT_SUCCESS;
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
