#define _GNU_SOURCE

#include <limits.h>
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

#include "dyn_str.h"
#include "runner.h"
#include "utils.h"
#include "executor.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"
#include "shell_state.h"

static job_id create_job_id(void) {
    job_id new_id = 1;

    if (sh_env.jctl.jobs.size == 0)
        return new_id;

    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        if (new_id == sh_env.jctl.jobs.data[i].id) {
            ++new_id;
            continue;
        }
    }

    return new_id;
}

static jc_job *create_job(pid_t pgid) {
    /* this iterates over jobs so can't call after pushing */
    job_id jid = create_job_id();

    jc_job *job = da_push(&sh_env.jctl.jobs);
    if (!job)
        xfatal("da_push_init");

    job->pgid = pgid;
    job->id = jid;
    job->stat = PRUNNING;

    return job;
}

static jc_job *lookup_job(pid_t pid, size_t *index) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        if (pid == sh_env.jctl.jobs.data[i].pgid) {

            if (index)
                *index = i;

            return &sh_env.jctl.jobs.data[i];
        }
    }

    xfatal("failed to find tracked job");
}

static void remove_job(pid_t pid) {
    size_t index;
    lookup_job(pid, &index);

    if (da_delete(&sh_env.jctl.jobs, index) == -1)
        xfatal("da_delete");
}

static void job_exited(jc_job *job, int wstat) {
    if (WIFEXITED(wstat)) {
        LOG_INFO("[%d] exited with status %d", job->id, WEXITSTATUS(wstat));

    } else if (WIFSIGNALED(wstat)) {
        LOG_INFO("[%d] terminated by signal %d (%s)", job->id,
                WTERMSIG(wstat), strsignal(WTERMSIG(wstat)));
    }

    remove_job(job->pgid);
}

static void job_toggled(jc_job *job, int wstat) {
    if (WIFCONTINUED(wstat) && job->stat == PSTOPPED) {
        job->stat = PRUNNING;
        LOG_INFO("[%d] continued", job->id);


    } else if (WIFSTOPPED(wstat) && job->stat == PRUNNING) {
        job->stat = PSTOPPED;
        LOG_INFO("[%d] stopped", job->id);

    } else
        xfatal("unexpected job status");
}

static void job_updated(pid_t pid, int wstat) {
    jc_job *job = lookup_job(pid, NULL);

    if (WIFEXITED(wstat) || WIFSIGNALED(wstat)) {
        job_exited(job, wstat);

    } else if (WIFSTOPPED(wstat) || WIFCONTINUED(wstat)) {
        job_toggled(job, wstat);

    } else {
        xfatal("unexpected wstat value");
    }
}

jc_job *lookup_job_by_id(job_id jid) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i)
        if (jid == sh_env.jctl.jobs.data[i].id)
            return &sh_env.jctl.jobs.data[i];

    return NULL;
}

int sighup_shutdown(void) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        jc_job *job = &sh_env.jctl.jobs.data[i];

        if (getpgrp() == job->pgid || job->pgid <= 1)
            xfatal("unexpected pgid %d", job->pgid);

        if (xkill(-job->pgid, SIGHUP) == -1)
            if (errno != ESRCH)
                err_exit("kill");

        if (xkill(-job->pgid, SIGCONT) == -1)
            if (errno != ESRCH)
                err_exit("kill");
    }

    if (jc_wait() == -1)
        xfatal("jctl_wait");

    return 0;
}

int jc_wait(void) {
    int wstat;
    int child_pid;
    int wopts = WUNTRACED | WCONTINUED | WNOHANG;

    while ((child_pid = xwaitpid(-1, &wstat, wopts)) > 0)
        job_updated(child_pid, wstat);

    if (child_pid == -1 && errno != ECHILD)
        err_exit("waitpid");

    return 0;
}

int jc_wait_pid(pid_t pid) {
    int wstat;
    int child_pid;
    int wopts = WUNTRACED | WCONTINUED;

    child_pid = xwaitpid(pid, &wstat, wopts);

    if (child_pid == -1)
        err_exit("waitpid");

    job_updated(child_pid, wstat);

    return 0;
}

void jc_free(jc_jst *jctl) {
    da_free(&jctl->jobs);

    *jctl = (jc_jst){0};
}

void jc_init(jc_jst *jctl) {
    *jctl = (jc_jst){0};

    if (da_init(&jctl->jobs) == -1)
        xfatal("init_jst");
}

void sh_run(const ps_ast *ast) {
    pid_t subsh_pid = xfork();

    if (subsh_pid == -1)
        err_exit("fork");

    if (xsetpgid(subsh_pid, subsh_pid) == -1 && errno != EACCES)
        err_exit("setpgid");

    if (!ast->bg && xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
        err_exit("tcsetpgrp");

    if (subsh_pid == 0)
        ex_exec(ast);

    jc_job *job = create_job(subsh_pid);
    if (!job)
        xfatal("failed to create job");

    LOG_INFO("[%d]", job->id);

    if (!ast->bg) {
        jc_wait_pid(subsh_pid);

        if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
            err_exit("tcsetpgrp");
    }
}
