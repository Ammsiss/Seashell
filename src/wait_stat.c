#define _GNU_SOURCE

#include <errno.h>
#include <sys/wait.h>

#include "utils.h"
#include "log.h"
#include "wait_stat.h"

#define WEV_EXIT(_pid, _exit_stat) \
    ({ \
        LOG_INFO("%d exited with status %d", _pid, _exit_stat); \
        (wait_event){ .pid = _pid, .type = PEXITED, { .exit_stat = _exit_stat }}; \
    })

#define WEV_SIG(_pid, _term_sig) \
    ({ \
        LOG_INFO("%d terminated by signal %d (%s)", _pid, _term_sig, \
                strsignal(_term_sig)); \
        (wait_event){ .pid = _pid, .type = PSIGNALED, { .term_sig = _term_sig }}; \
    })

#define WEV_STOP(_pid) \
    ({ \
        LOG_INFO("%d stopped", _pid); \
        (wait_event){ .pid = _pid, .type = PSTOPPED, {0}}; \
    })

#define WEV_CONT(_pid) \
    ({ \
        LOG_INFO("%d continued", _pid); \
        (wait_event){ .pid = _pid, .type = PCONTINUED, {0}}; \
    })

int get_wstat(wait_event *wev) {
    assert(wev);

    int wstat;
    int wopts = WUNTRACED | WCONTINUED | WNOHANG;

    pid_t cpid = xwaitpid(-1, &wstat, wopts);

    if (cpid == -1 && errno != ECHILD)
        err_exit("waitpid");

    if (cpid == 0 || errno == ECHILD)
        return -1;

    if (WIFEXITED(wstat)) {
        *wev = WEV_EXIT(cpid, WEXITSTATUS(wstat));

    } else if (WIFSIGNALED(wstat)) {
        *wev = WEV_SIG(cpid, WTERMSIG(wstat));

    } else if (WIFSTOPPED(wstat)) {
        *wev = WEV_STOP(cpid);

    } else if (WIFCONTINUED(wstat)) {
        *wev = WEV_CONT(cpid);
    }


    return 0;
}
