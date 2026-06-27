#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <wait.h>
#include <signal.h>

#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "executor.h"

void reap_children(int sig) {
    (void) sig;

    int saved_errno = errno;

    /* TODO: add diagnostic on waitpid() failure */
    while (waitpid(0, NULL, WNOHANG) > 0)
        continue;

    errno = saved_errno;
}

int main(int argc, char **argv) {
    printf("seashell PID(%d)\n", getpid());

    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = reap_children;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        fprintf(stderr, "Error: sigaction (%s)\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage error\n");
        return EXIT_FAILURE;
    }

    da_tok tokens = {0};
    if (lx_tokenize(argv[1], &tokens) == -1) {
        fprintf(stderr, "Lexer error\n");
        return EXIT_FAILURE;
    }

    ps_job job = {0};
    if (ps_parse(&tokens, &job) == -1) {
        fprintf(stderr, "Parser error\n");
        return EXIT_FAILURE;
    }

    if (ex_expand(&job) == -1) {
        fprintf(stderr, "Expansion error\n");
        return EXIT_FAILURE;
    }

    if (sh_run(&job) == -1) {
        fprintf(stderr, "Executor error\n");
        return EXIT_FAILURE;
    }


    lx_free(&tokens);
    ps_free(&job);
}
