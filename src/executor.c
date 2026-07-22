#define _GNU_SOURCE

#include <sys/wait.h>
#include <linux/magic.h>
#include <valgrind/valgrind.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell_state.h"
#include "dyn_str.h"
#include "parser.h" // IWYU pragma: keep
#include "executor.h"
#include "dyn_arr.h"
#include "log.h"
#include "shell_state.h"
#include "utils.h"
#include "builtins.h"

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

void verify_child_fd_count(bool first, bool last) {
    if (first && last)
        verify_fd_count(0);
    else if ((!first && last) || (first && !last))
        verify_fd_count(1);
    else if (!first && !last)
        verify_fd_count(2);
}

static void move_fd(int fd1, int fd2) {
    if (fd1 == fd2)
        return;

    if (xdup2(fd1, fd2) == -1)
        err_exit("dup2");

    if (xclose(fd1) == -1)
        err_exit("close");
}

static void child_fd_setup(bool first, bool last, int next_pipe[2], \
        int prev_rfd) {
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

pstat get_job_stat(da_procs *procs) {
    bool one_stopped = false;

    for (size_t i = 0; i < procs->size; ++i) {
        ex_proc *proc = &procs->data[i];

        if (proc->stat == PRUNNING)
            return PRUNNING;

        if (proc->stat == PSTOPPED)
            one_stopped = true;
    }

    return (one_stopped) ? PSTOPPED : PEXITED;
}

ex_proc *lookup_proc(da_procs *procs, pid_t pid) {
    for (size_t i = 0; i < procs->size; ++i)
        if (pid == procs->data[i].pid)
            return &procs->data[i];

    xfatal("waited for unexpected child");
}

static void run_pline(const ps_pline *pline, da_procs *procs) {
    int status;
    int next_pipe[2];
    int prev_rfd;

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

        if (cpid == 0) {
            child_fd_setup(first, last, next_pipe, prev_rfd);
            child_redir_setup(&cur_cmd->redirs);

            if (try_run_builtin(cur_cmd->argv, &status))
                _exit(status);

            xexecvp(cur_cmd->argv[0], cur_cmd->argv);

            if (errno == ENOENT) {
                err_msg("command not found: %s", cur_cmd->argv[0]);
                _exit(127);
            }

            err_exit("execvp");
        }

        ex_proc *proc = da_push(procs);
        if (!proc)
            xfatal("da_push");

        proc->pid = cpid;
        proc->stat = PRUNNING;

        if (!first)
            if (xclose(prev_rfd) == -1)
                err_exit("close");

        if (!last) {
            prev_rfd = next_pipe[0];
            if (xclose(next_pipe[1]) == -1)
                err_exit("close");
        }
    }

    if (!RUNNING_ON_VALGRIND)
        verify_fd_count(0);
}

static int ex_wait(da_procs *procs) {
    int wstat;
    int child_pid;
    int wopts = WUNTRACED | WCONTINUED;

    while ((child_pid = xwaitpid(-1, &wstat, wopts)) > 0) {

        ex_proc *proc = lookup_proc(procs, child_pid);

        if (WIFEXITED(wstat)) {
            proc->stat = PEXITED;
            proc->exit_stat = WEXITSTATUS(wstat);

        } else if (WIFSIGNALED(wstat)) {
            proc->stat = PEXITED;
            proc->exit_stat = WTERMSIG(wstat) + 128;

        } else if (WIFSTOPPED(wstat)) {
            proc->stat = PSTOPPED;

        } else if (WIFCONTINUED(wstat)) {
            proc->stat = PRUNNING;

        } else {
            xfatal("unexpected wstat value");
        }

        pstat job_stat = get_job_stat(procs);

        if (job_stat == PEXITED) {
            break;

        } else if (job_stat == PSTOPPED) {
            if (xkill(getpid(), SIGTSTP) == -1)
                err_exit("kill");

            for (size_t i = 0; i < procs->size; ++i)
                procs->data[i].stat = PRUNNING;
        }
    }

    if (child_pid == -1 && errno != ECHILD)
        err_exit("waitpid");

    return procs->data[procs->size - 1].exit_stat;
}

static void ex_run_andor_chain(const ps_ast *ast) {
    int prev_exit_stat;
    da_procs procs;

    for (size_t i = 0; i < ast->andors.size; ++i) {
        ps_andor *andor = &ast->andors.data[i];

        if (andor->op == PS_AND_IF && prev_exit_stat != EXIT_SUCCESS)
            continue;

        if (andor->op == PS_OR_IF && prev_exit_stat == EXIT_SUCCESS)
            continue;

        if (da_init(&procs) == -1)
            xfatal("da_init");

        run_pline(&andor->pline, &procs);
        prev_exit_stat = ex_wait(&procs);

        da_free(&procs);
    }
}

void ex_exec(const ps_ast *ast) {
    sh_env.subshell = true;

    if (restore_signals() == -1)
        xfatal("restore_signals");

    ex_run_andor_chain(ast);

    _exit(EXIT_SUCCESS);
}
