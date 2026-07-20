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
#include "builtins.h"
#include "utils.h"
#include "executor.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"
#include "shell_state.h"

static void free_pgrp(jc_pgrp *pgrp) {
    assert(pgrp);

    da_free(&pgrp->procs);

    *pgrp = (jc_pgrp){0};
}

static void free_job(jc_job *job) {
    assert(job);

    free_pgrp(&job->pgrp);

    *job = (jc_job){0};
}

void free_jst(jc_jst *jctl) {
    assert(jctl);

    for (size_t i = 0; i < jctl->jobs.size; ++i)
        free_job(&jctl->jobs.data[i]);

    da_free(&jctl->jobs);

    *jctl = (jc_jst){0};
}

static int init_pgrp(jc_pgrp *pgrp) {
    assert(pgrp);
    *pgrp = (jc_pgrp){0};

    if (da_init(&pgrp->procs) == -1)
        return -1;

    return 0;
}

static int init_job(jc_job *job) {
    assert(job);
    *job = (jc_job){0};

    if (init_pgrp(&job->pgrp) == -1)
        xfatal("init_pgrp");

    return 0;
}

int init_jst(jc_jst *jctl) {
    assert(jctl);
    *jctl = (jc_jst){0};

    if (da_init(&jctl->jobs) == -1)
        return -1;

    return 0;
}

int sighup_shutdown(void) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        jc_job *job = &sh_env.jctl.jobs.data[i];

        if (getpgrp() == job->pgrp.pgid || job->pgrp.pgid <= 1)
            xfatal("unexpected pgid %d", job->pgrp.pgid);

        if (xkill(-job->pgrp.pgid, SIGHUP) == -1)
            if (errno != ESRCH)
                err_exit("kill");
    }

    if (jctl_wait(NULL) == -1)
        xfatal("jctl_wait");

    return 0;
}

static job_id identify_proc(pid_t pid, jc_proc** proc) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        jc_job *job = &sh_env.jctl.jobs.data[i];

        for (size_t y = 0; y < job->pgrp.procs.size; ++y) {
            jc_proc *out = &job->pgrp.procs.data[y];

            if (pid == out->pid) {
                if (proc)
                    *proc = out;
                return job->id;
            }
        }
    }

    return -1;
}

static jc_job *lookup_job(job_id jid, size_t *index) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        if (jid == sh_env.jctl.jobs.data[i].id) {
            if (index)
                *index = i;
            return &sh_env.jctl.jobs.data[i];
        }
    }

    return NULL;
}

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

static jc_job *create_job(void) {
    job_id jid = create_job_id();

    jc_job *job = da_push_init(&sh_env.jctl.jobs, init_job);
    if (!job)
        xfatal("da_push_init");

    job->id = jid;
    job->pgrp.job = job;
    job->stat = PRUNNING;

    return job;
}

static int remove_job(job_id jid) {
    size_t index;
    jc_job *job = lookup_job(jid, &index);
    if (!job)
        xfatal("lookup_job");

    free_job(job);

    if (da_delete(&sh_env.jctl.jobs, index) == -1)
        xfatal("da_delete");

    return 0;
}

char *get_pid_string(job_id jid) {
    jc_job *job = lookup_job(jid, NULL);
    if (!job)
        return NULL;

    d_str pid_str;
    if (d_str_init(&pid_str) == -1)
        return NULL;

    char buf[4096]; /* surely not longer then this */

    for (size_t i = 0; i < job->pgrp.procs.size; ++i) {
        jc_proc *proc = &job->pgrp.procs.data[i];
        snprintf(buf, 4096, "%d ", proc->pid);
        if (d_strcat(&pid_str, buf) == -1)
            goto fail;
    }

    return pid_str.c_str;

fail:
    d_str_free(&pid_str);
    return NULL;
}

int msg_job_start(job_id jid) {
    char *pid_str = get_pid_string(jid);
    if (!pid_str)
        xfatal("get_pid_string");

    LOG_INFO("[%d] %s", jid, pid_str);

    free(pid_str);

    return 0;
}

static int set_job_stat(job_id jid) {
    jc_job *job = lookup_job(jid, NULL);
    if (!job)
        xfatal("lookup_job");

    bool one_proc_stopped = false;

    for (size_t i = 0; i < job->pgrp.procs.size; ++i) {
        switch (job->pgrp.procs.data[i].stat) {
        case PRUNNING:
            if (job->stat != PRUNNING)
                LOG_INFO("[%d] continued", job->id);
            job->stat = PRUNNING;

            return 0;
        case PSTOPPED:
            one_proc_stopped = true;
            break;

        case PEXITED:
            continue;
        }
    }

    if (one_proc_stopped) {
        if (job->stat != PSTOPPED) {
            job->stat = PSTOPPED;
            LOG_INFO("[%d] stopped", job->id);
        }
    } else {
        if (job->stat == PEXITED)
            xfatal("unexpected job status");

        job->stat = PEXITED;
        LOG_INFO("[%d] done", job->id);

        if (remove_job(job->id) == -1)
            xfatal("remove_job");
    }

    return 0;
}

static int proc_exited(pid_t pid, int wstat) {
    jc_proc *proc;
    job_id jid = identify_proc(pid, &proc);
    if (jid == -1)
        xfatal("identify_proc");

    proc->wstat = wstat;
    proc->stat = PEXITED;

    if (WIFEXITED(wstat)) {
        LOG_INFO("[%d] %d exited with status %d", jid, pid,
                WEXITSTATUS(wstat));

    } else if (WIFSIGNALED(wstat)) {
        LOG_INFO("[%d] %d terminated by signal %d (%s)", jid, pid,
                WTERMSIG(wstat), strsignal(WTERMSIG(wstat)));
    } else
        xfatal("unexpected wstat");

    if (set_job_stat(jid) == -1)
        xfatal("set_job_stat");

    return 0;
}

static int proc_stopped(pid_t pid) {
    jc_proc *proc;
    job_id jid = identify_proc(pid, &proc);
    if (jid == -1)
        xfatal("identify_proc");

    proc->stat = PSTOPPED;
    LOG_INFO("[%d] %d stopped", jid, pid);

    if (set_job_stat(jid) == -1)
        xfatal("set_job_stat");

    return 0;
}

static int proc_continued(pid_t pid) {
    jc_proc *proc;
    job_id jid = identify_proc(pid, &proc);
    if (jid == -1)
        xfatal("identify_proc");

    proc->stat = PRUNNING;
    LOG_INFO("[%d] %d continued", jid, pid);

    if (set_job_stat(jid) == -1)
        xfatal("set_job_stat");

    return 0;
}

int jctl_wait(job_id *jid) {
    bool wait_on_job = jid;

    int wstat;
    int cpid;
    int wopts = WUNTRACED | WCONTINUED;

    jc_job *job = NULL;

    if (wait_on_job) {
        job = lookup_job(*jid, NULL);
        if (!job)
            xfatal("lookup_job");
    } else
        wopts |= WNOHANG;

    while ((cpid = xwaitpid(-1, &wstat, wopts)) != 0) {
        if (cpid == -1 && errno == ECHILD) {
            if (wait_on_job)
                xfatal("shouldn't get echild here");
            break;
        }
        if (cpid == -1)
            err_exit("waitpid");

        if (WIFEXITED(wstat) || WIFSIGNALED(wstat)) {
            if (proc_exited(cpid, wstat) == -1)
                xfatal("proc_exited");

        } else if (WIFSTOPPED(wstat)) {
            if (proc_stopped(cpid) == -1)
                xfatal("proc_stopped");

        } else if (WIFCONTINUED(wstat)) {
            if (proc_continued(cpid) == -1)
                xfatal("proc_continued");

        } else {
            xfatal("unexpected wstat value");
        }

        if (wait_on_job && (job->stat == PEXITED || job->stat == PSTOPPED))
            break;
    }

    return 0;
}

void sh_run_job(const ps_pline *pline, bool bg) {
    if (pline->cmds.size == 1 && !bg)
        if (try_run_builtin(pline->cmds.data[0].argv, NULL))
            return;

    jc_job *job = create_job();
    if (!job)
        xfatal("failed to create job");

    if (exec_pline(pline, bg, &job->pgrp) == -1)
        xfatal("exec_pline");

    if (msg_job_start(job->id) == -1)
        xfatal("msg_job_start");

    if (bg)
        return;

    if (jctl_wait(&job->id) == -1)
        xfatal("wait_for_pids");

    if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
        err_exit("tcsetpgrp");
}
