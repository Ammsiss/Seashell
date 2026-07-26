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

        } else if (WIFSIGNALED(wstat)) {
            wev->type = PSIGNALED;
            wev->exit_stat = WTERMSIG(wstat);

        } else if (WIFSTOPPED(wstat)) {
            wev->type = PSTOPPED;

        } else if (WIFCONTINUED(wstat)) {
            wev->type = PCONTINUED;
        }
    }

    if (cpid == -1 && errno != ECHILD)
        err_exit("waitpid");
}
