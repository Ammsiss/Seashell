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

#include "runner.h"
#include "builtins.h"
#include "utils.h"
#include "executor.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"
#include "shell_state.h"

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
            LOG_INFO("process %d terminated by signal %d (%s)",
                    pid, signum, strsignal(signum));
            last_status = 128 + signum;
        } else if (WIFSTOPPED(wstat)) {
            LOG_INFO("seashell: process %d stopped\n", pid);
#ifdef WIFCONTINUED
        } else if (WIFCONTINUED(wstat)) {
            LOG_INFO("seashell: process %d continued\n", pid);
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

static int run_pline(const ps_pline *pline) {
    pline_info info;
    if (exec_pline(pline, false, &info) == -1)
        fatal("exec_pline");

    int stat;
    if (wait_for_pids(&info.pids, &stat) == -1)
        fatal("bad job control state");

    if (xtcsetpgrp(get_env()->tty_fd, getpgrp()) == -1)
        errExit(true, "tcsetpgrp");

    return stat;
}

void sh_run(const ps_job *job) {
    int prev_stat;

    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];

        if (andor->op == PS_OR_IF && prev_stat == EXIT_SUCCESS)
            continue;

        if (andor->op == PS_AND_IF && prev_stat != EXIT_SUCCESS)
            continue;

        if (andor->pline.cmds.size == 1)
            try_run_builtin(andor->pline.cmds.data[0].argv, &prev_stat);

        prev_stat = run_pline(&andor->pline);
    }
}
