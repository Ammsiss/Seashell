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

#include "ast_man.h"
#include "dyn_str.h"
#include "runner.h"
#include "builtins.h"
#include "utils.h"
#include "executor.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "log.h"
#include "shell_state.h"

static void free_proc(jc_proc *proc) {
    assert(proc);

    d_str_free(&proc->cmd);

    *proc = (jc_proc){0};
}

static void free_pgrp(jc_pgrp *pgrp) {
    assert(pgrp);

    for (size_t i = 0; i < pgrp->procs.size; ++i)
        free_proc(&pgrp->procs.data[i]);

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

static int init_proc(jc_proc *proc) {
    *proc = (jc_proc){0};

    if (d_str_init(&proc->cmd) == -1)
        return -1;

    return 0;
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

jc_proc *add_proc(jc_pgrp *pgrp, char **argv) {
    jc_proc *proc = da_push(&pgrp->procs);
    if (!proc)
        return NULL;

    if (init_proc(proc) == -1)
        return NULL;

    for (char **arg = argv; *arg != NULL; ++arg) {
        if (arg != argv)
            if (d_strcat(&proc->cmd, " ") == -1)
                return NULL;

        if (d_strcat(&proc->cmd, *arg) == -1)
            return NULL;
    }

    return proc;
}

int sighup_shutdown(void) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        jc_job *job = &sh_env.jctl.jobs.data[i];

        if (getpgrp() == job->pgrp.pgid || job->pgrp.pgid <= 1)
            xfatal("unexpected pgid %d", job->pgrp.pgid);

        if (xkill(-job->pgrp.pgid, SIGHUP) == -1 && errno != ESRCH)
            err_exit("kill");

        if (xkill(-job->pgrp.pgid, SIGCONT) == -1 && errno != ESRCH)
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
                return job->jid;
            }
        }
    }

    return -1;
}

jc_job *lookup_job(job_id jid, size_t *index) {
    for (size_t i = 0; i < sh_env.jctl.jobs.size; ++i) {
        if (jid == sh_env.jctl.jobs.data[i].jid) {
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
        if (new_id == sh_env.jctl.jobs.data[i].jid) {
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

    job->jid = jid;
    job->pgrp.job = job;
    job->stat = PRUN;
    job->ev = JSTARTED;

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

// TODO: change name of PRUNNING etc
// TODO: this should return jstat
static pstat calc_job_stat(jc_job *job) {
    bool stopped = false;

    da_proc *procs = &job->pgrp.procs;

    for (size_t i = 0; i < procs->size; ++i) {

        if (procs->data[i].stat == PRUN)
            return PRUN;

        if (procs->data[i].stat == PSTOP)
            stopped = true;
    }

    return stopped ? PSTOP : PEXIT;
}

static void set_job_stat(job_id jid) {
    jc_job *job = lookup_job(jid, NULL);
    if (!job)
        xfatal("lookup_job");

    pstat stat = calc_job_stat(job);

    if (job->stat == PRUN && stat == PSTOP)
        job->ev |= JSTOPPED;

    if (job->stat == PSTOP && stat == PRUN)
        job->ev |= JRESUMED;

    job->stat = stat;

    if (job->stat == PEXIT) {
        job_id jid = job->jid;
        bool success = job->pgrp.procs.data[job->pgrp.procs.size - 1].success;

        if (remove_job(job->jid) == -1)
            xfatal("remove_job");

        run_next_if_more(jid, success);
    }
}

int jctl_wait(job_id *jid) {
    bool wait_on_job = jid;

    int wstat;
    int cpid;
    int wopts = WUNTRACED | WCONTINUED;
    jc_proc *proc;

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

        int cjid;

        if ((cjid = identify_proc(cpid, &proc)) == -1)
            xfatal("identify_proc");

        if (WIFEXITED(wstat)) {
            proc->stat = PEXIT;
            proc->exit_stat = WEXITSTATUS(wstat);
            proc->success = proc->exit_stat == EXIT_SUCCESS;
            LOG_INFO("[%d] %d exited with status %d", cjid, cpid,
                    WEXITSTATUS(wstat));

        } else if (WIFSIGNALED(wstat)) {
            proc->stat = PEXIT;
            proc->exit_stat = WTERMSIG(wstat) + 128;
            proc->success = false;
            LOG_INFO("[%d] %d terminated by signal %d (%s)", cjid, cpid,
                    WTERMSIG(wstat), strsignal(WTERMSIG(wstat)));

        } else if (WIFSTOPPED(wstat)) {
            proc->stat = PSTOP;
            LOG_INFO("[%d] %d stopped", cjid, cpid);

        } else if (WIFCONTINUED(wstat)) {
            proc->stat = PRUN;
            LOG_INFO("[%d] %d continued", cjid, cpid);

        } else {
            xfatal("unexpected wstat value");
        }

        set_job_stat(cjid);

        if (wait_on_job && (job->stat == PEXIT || job->stat == PSTOP)) {
            if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
                err_exit("tcsetpgrp");

            break;
        }
    }

    return 0;
}

pstat sh_run_job(const ps_pline *pline, bool bg, job_id *jid) {
    assert(pline && jid);

    if (pline->cmds.size == 1 && !bg)
        if (try_run_builtin(pline->cmds.data[0].argv, NULL))
            return PEXIT;

    jc_job *job = create_job();
    if (!job)
        xfatal("failed to create job");

    *jid = job->jid;

    if (exec_pline(pline, bg, &job->pgrp) == -1)
        xfatal("exec_pline");

    if (!bg) {
        if (jctl_wait(&job->jid) == -1)
            xfatal("wait_for_pids");

        return job->stat;
    }

    return job->stat;
}
