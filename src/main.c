#define _GNU_SOURCE

#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <wait.h>

#include "expander.h"
#include "job_state.h"
#include "builtins.h"
#include "parser.h"
#include "lexer.h"
#include "input.h"
#include "shell_state.h"
#include "utils.h"
#include "log.h"

static void drain_job_events(void);

static volatile sig_atomic_t sigchld_caught = false;
static volatile sig_atomic_t sighup_caught = false;
static volatile sig_atomic_t sigint_caught = false;

void sigchld_handler(int _) {
    sigchld_caught = true;
}

void sighup_handler(int _) {
    sighup_caught = true;
}

void sigint_handler(int _) {
    sigint_caught = true;
}

static int set_sig_action(int sig, sighandler_t handler, int flags, sigset_t *mask) {
    struct sigaction sa;
    sa.sa_flags = flags;
    sa.sa_handler = handler;

    if (mask) {
        sa.sa_mask = *mask;
    } else {
        if (xsigemptyset(&sa.sa_mask) == -1)
            return -1;
    }

    if (xsigaction(sig, &sa, NULL) == -1)
        return -1;

    return 0;
}

static int procmask_add(int sig, int how) {
    sigset_t set;

    if (xsigemptyset(&set) == -1)
        return -1;
    if (xsigaddset(&set, sig) == -1)
        return -1;

    if (xsigprocmask(how, &set, NULL) == -1)
        return -1;

    return 0;
}

static int block_sig(int sig) {
    if (procmask_add(sig, SIG_BLOCK) == -1)
        return -1;

    return 0;
}

static int send_bg_jobs_hup(void) {
    for (size_t i = 0; i < get_jctl()->jobs.size; ++i) {
        jc_job *job = &get_jctl()->jobs.data[i];

        if (getpgrp() == job->pgrp.pgid || job->pgrp.pgid <= 1)
            xfatal("unexpected pgid %d", job->pgrp.pgid);

        if (xkill(-job->pgrp.pgid, SIGHUP) == -1 && errno != ESRCH)
            err_exit("kill");

        if (xkill(-job->pgrp.pgid, SIGCONT) == -1 && errno != ESRCH)
            err_exit("kill");
    }

    return 0;
}

static void process_signals(void) {
    if (sigchld_caught) {
        update_job_table();
        sigchld_caught = false;
    }

    if (sighup_caught) {
        send_bg_jobs_hup();
        exit(EXIT_SUCCESS);
        sighup_caught = false;
    }

    if (sigint_caught) {
        sigint_caught = false;
    }
}

static void sig_restore(void) {
    if (xsigprocmask(SIG_SETMASK, &sh_env.og_mask, NULL) == -1)
        err_exit("sigprocmask");

    if (set_sig_action(SIGTTOU, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sig_action");
    if (set_sig_action(SIGTTIN, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTSTP, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGQUIT, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTERM, SIG_DFL, 0, NULL) == -1)
        xfatal("set_sigaction");
}

static void sig_setup(void) {
    if (xsigprocmask(0, NULL, &sh_env.og_mask) == -1)
        err_exit("sigprocmask");

    if (set_sig_action(SIGTTOU, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sig_action");
    if (set_sig_action(SIGTTIN, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTSTP, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGQUIT, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");
    if (set_sig_action(SIGTERM, SIG_IGN, 0, NULL) == -1)
        xfatal("set_sigaction");

    if (block_sig(SIGCHLD) == -1)
        xfatal("block_sig");
    if (set_sig_action(SIGCHLD, sigchld_handler, 0, NULL) == -1)
        xfatal("set_sig_action");

    if (block_sig(SIGHUP) == -1)
        xfatal("block_sig");
    if (set_sig_action(SIGHUP, sighup_handler, 0, NULL) == -1)
        xfatal("set_sig_action");

    if (block_sig(SIGINT) == -1)
        xfatal("block_sig");
    if (set_sig_action(SIGINT, sigint_handler, 0, NULL) == -1)
        xfatal("set_sig_action");
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

static void child_redir_setup(da_redir *redirs) {
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

static pid_t exec_pline(const ps_pline *pline, bool bg) {
    if (!pline)
        assert(pline);

    int next_pipe[2];
    int prev_rfd;
    int pgid;

    da_pid pids = {0};

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
            pgid = cpid;

        if (cpid == 0) {

            sh_env.subshell = true;

            /* subshell set up */

            if (xsetpgid(0, pgid) == -1)
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

        if (xsetpgid(cpid, pgid) == -1 && errno != EACCES)
            err_exit("setpgid");

        if (!bg && first)
            if (xtcsetpgrp(sh_env.tty_fd, pgid) == -1)
                err_exit("tcsetpgrp");

        if (!first)
            if (xclose(prev_rfd) == -1)
                err_exit("close");

        if (!last) {
            prev_rfd = next_pipe[0];
            if (xclose(next_pipe[1]) == -1)
                err_exit("close");
        }

        pid_t *pid = da_push(&pids);
        if (!pid)
            xfatal("da_push");

        *pid = cpid;
    }

    pid_t jid = add_job(&pids, pgid);

    da_free(&pids);

    return jid;
}

static void log_job_event(job_event *jev) {
    switch (jev->type) {
    case JEXITED:
        LOG_INFO("[%d] exited", jev->jid);
        break;
    case JSTOPPED:
        LOG_INFO("[%d] stopped", jev->jid);
        break;
    case JCONTINUED:
        LOG_INFO("[%d] continued", jev->jid);
        break;
    case JSTARTED:
        LOG_INFO("[%d] started", jev->jid);
        break;
    }
}

static void drain_job_events_until_finished(pid_t fg_jid) {
    bool job_done = false;

    while (!job_done) {
        sigsuspend(&sh_env.og_mask); /* always returns -1 */

        if (errno != EINTR)
            err_exit("sigsuspend");

        process_signals();

        job_event *jev;

        while ((jev = pop_job_event())) {
            log_job_event(jev);

            if (fg_jid == jev->jid && jev->type == JEXITED)
                job_done = true;
        }
    }
}

static void drain_job_events(void) {
    job_event *jev;

    while ((jev = pop_job_event())) {
        log_job_event(jev);
    }
}

static void run_line(const char *line) {
    da_tok toks = {0};
    ps_ast ast = {0};

    if (lx_tokenize(line, &toks) == -1)
        xfatal("lx_tokenize");

    if (ps_parse(&toks, &ast) == -1)
        xfatal("lx_tokenize");

    if (ex_expand(&ast) == -1)
        xfatal("lx_tokenize");

    pid_t jid = exec_pline(&ast.andors.data[0].pline, ast.bg);

    if (!ast.bg) {
        drain_job_events_until_finished(jid);

        if (xtcsetpgrp(sh_env.tty_fd, getpgrp()) == -1)
            err_exit("tcsetpgrp");

    }

    lx_free(&toks);
    ps_free(&ast);
}

int main(void) {
    log_init();
    env_init();
    sig_setup();

    LOG_INFO("seashell PID(%d)", getpid());

    struct pollfd events = {
        .events = POLLIN,
        .fd = sh_env.tty_fd
    };

    display_prompt();

    while (true) {
        int ready = xppoll(&events, 1, 0, &sh_env.og_mask);

        if (ready == -1) {
            if (errno != EINTR)
                xfatal("ppoll");

            process_signals();
            drain_job_events();

        } else if (ready == 1) {
            char *line;
            input_stat iostat = get_line(&line);

            if (iostat == INPUT_ERR)
                xfatal("failed to read from terminal");

            if (iostat == INPUT_EOF)
                break;

            run_line(line);
            drain_job_events();
            display_prompt();
        }
    }

    return EXIT_SUCCESS;
}
