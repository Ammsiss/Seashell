#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

#include "executor.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes

char **create_argv(const ps_cmd *cmd) {
    size_t argc = cmd->words.size + 1;

    char **argv = calloc(argc, sizeof(char *));
    if (!argv)
        return NULL;

    for (size_t i = 0; i < cmd->words.size; ++i)
        argv[i] = cmd->words.data[i].arg;

    argv[argc - 1] = NULL;

    return argv;
}

int run_cmd(const ps_cmd *cmd) {
    int child_pid;
    switch (child_pid = fork()) {
    case -1:
        return -1;
    case 0: /* child continues below */
        break;
    default: /* parent waits then returns (for now) */
        int wstat;
        if (waitpid(child_pid, &wstat, 0) == -1)
            return -1;

        printf("waited for child(%d) that ", child_pid);
        if (WIFEXITED(wstat))
            printf("exited normally and returned %d\n", WEXITSTATUS(wstat));
        else
            printf("didn't exit normally\n");

        return 0;
    }

    char **argv = create_argv(cmd);
    execvp(argv[0], argv);
    return -1;
}

int run_pipeline(const ps_pipeline *pipeline) {
    for (size_t i = 0; i < pipeline->cmds.size; ++i) {
        const ps_cmd *cmd = &pipeline->cmds.data[i];
        if (run_cmd(cmd) == -1)
            return -1;
    }

    return 0;
}

int sh_run(const ps_job *job) {
    for (size_t i = 0; i < job->andors.size; ++i) {
        const ps_andor *andor = &job->andors.data[i];
        switch (andor->op) {
        case PS_NO_IF:
            if (run_pipeline(&andor->pipeline) == -1)
                return -1;
        case PS_OR_IF:
        case PS_AND_IF:
        }
    }

    return 0;
}
