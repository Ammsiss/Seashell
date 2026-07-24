#define _GNU_SOURCE

#include <poll.h>
#include <unistd.h>
#include <wait.h>

#include "noti.h"
#include "ast_man.h"
#include "input.h"
#include "shell_state.h"
#include "utils.h"
#include "log.h"

int main(void) {
    if (log_init() == -1)
        fatal("log_init");
    if (env_init() == -1)
        fatal("env_init");

    LOG_INFO("seashell PID(%d)", getpid());

    struct pollfd events = {
        .events = POLLIN,
        .fd = sh_env.tty_fd
    };

    if (display_prompt() == -1)
        fatal("display_prompt");

    while (true) {
        int ready = xppoll(&events, 1, 0, &sh_env.og_mask);

        if (ready == -1) {
            if (errno != EINTR)
                xfatal("ppoll");

            if (process_signals() == -1)
                fatal("process_signals");

            if (noti_jobs(&sh_env.jctl, true))
                if (display_prompt() == -1)
                    xfatal("display_prompt");

            // TODO: remove all exited jobs here */
        }

        else if (ready == 1) {
            char *line;
            input_stat iostat = get_line(&line);

            if (iostat == INPUT_ERR)
                xfatal("failed to read from terminal");

            if (iostat == INPUT_EOF)
                break;

            job_plan *plan = register_plan(line);
            if (!plan)
                xfatal("register_plan");

            run_next(plan, true);
            noti_jobs(&sh_env.jctl, false);

            // TODO: remove all exited jobs here */

            if (display_prompt() == -1)
                fatal("display_prompt");
        }
    }

    env_free();
    log_free();

    return EXIT_SUCCESS;
}
