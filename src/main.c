#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "expander.h"
#include "executor.h"

int main(int argc, char **argv) {
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
