#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <wait.h>

#include "log.h"
#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "executor.h"

void run_cmd(const char *line, da_tok *tokens, ps_job *job) {
    if (lx_tokenize(line, tokens) == -1) {
        fprintf(stderr, "Lexer error\n");
        exit(EXIT_FAILURE);
    }

    if (ps_parse(tokens, job) == -1) {
        fprintf(stderr, "Parser error\n");
        exit(EXIT_FAILURE);
    }

    if (ex_expand(job) == -1) {
        fprintf(stderr, "Expansion error\n");
        exit(EXIT_FAILURE);
    }

    if (sh_run(job) == -1) {
        fprintf(stderr, "Executor error\n");
        exit(EXIT_FAILURE);
    }

    lx_free(tokens);
    ps_free(job);
}

int main(void) {
    if (log_init() == -1)
        return -1;

    printf("seashell PID(%d)\n", getpid());

    da_tok tokens = {0};
    ps_job job = {0};

    while (true) {
        printf("> ");
        fflush(stdout);

        char *line = NULL;
        size_t len;

        int num_read = getline(&line, &len, stdin);
        if (num_read == -1) {
            free(line);
            if (feof(stdin))
                break;
            return EXIT_FAILURE;
        }

        if (line[num_read - 1] == '\n')
            line[num_read - 1] = '\0';

        if (strlen(line) != 0)
            run_cmd(line, &tokens, &job);

        free(line);
    }

    lx_free(&tokens);
    ps_free(&job);
    printf("Bye!\n");
}
