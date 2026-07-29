#define _GNU_SOURCE

#include <errno.h>
#include <sys/wait.h>

#include "utils.h"
#include "log.h"
#include "wait_stat.h"
#include "dyn_arr.h"

void get_wstats(da_wevent *wevs) {
    pid_t cpid;
    int wstat;
    int wopts = WUNTRACED | WCONTINUED | WNOHANG;

    if (da_init(wevs) == -1)
        xfatal("da_init");

    while ((cpid = xwaitpid(-1, &wstat, wopts)) > 0) {

        wait_event *wev = da_push(wevs);
        if (!wev)
            xfatal("da_push");

        wev->pid = cpid;

        if (WIFEXITED(wstat)) {
            wev->type = PEXITED;
            wev->exit_stat = WEXITSTATUS(wstat);
            LOG_INFO("%d exited with status %d", wev->pid, wev->exit_stat);

        } else if (WIFSIGNALED(wstat)) {
            wev->type = PSIGNALED;
            wev->term_sig = WTERMSIG(wstat);
            LOG_INFO("%d terminated by signal %d (%s)", wev->pid,
                    wev->term_sig, strsignal(wev->term_sig));

        } else if (WIFSTOPPED(wstat)) {
            wev->type = PSTOPPED;
            LOG_INFO("%d stopped", wev->pid);

        } else if (WIFCONTINUED(wstat)) {
            wev->type = PCONTINUED;
            LOG_INFO("%d continued", wev->pid);
        }
    }

    if (cpid == -1 && errno != ECHILD)
        err_exit("waitpid");
}
