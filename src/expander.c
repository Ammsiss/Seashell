#include <stdio.h>
#include <string.h>

#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "expander.h"
#include "dstr.h"

static int create_arg(ps_word *word) {
    dstr arg;
    dstr_init(&arg);

    for (size_t i = 0; i < word->segments.size; ++i) {
        ps_segment *segment = &word->segments.data[i];
        dstrcat(&arg, segment->raw);
    }

    word->arg = arg.c_str;
    return 0;
}

static int create_argv(ps_cmd *cmd) {
    size_t argc = cmd->words.size;

    char **argv = calloc(argc + 1, sizeof(char *));
    if (!argv)
        return -1;

    for (size_t i = 0; i < argc; ++i)
        argv[i] = cmd->words.data[i].arg;

    argv[argc] = NULL;

    cmd->argv = argv;

    return 0;
}

int ex_expand(ps_ast *ast) {
    for (size_t i = 0; i < ast->andors.size; ++i) {
        ps_andor *andor = &ast->andors.data[i];
        ps_pline *pline = &andor->pline;

        for (size_t j = 0; j < pline->cmds.size; ++j) {
            ps_cmd *cmd = &pline->cmds.data[j];

            for (size_t k = 0; k < cmd->words.size; ++k) {
                ps_word *word = &cmd->words.data[k];
                if (create_arg(word) == -1)
                    return -1;
            }

            if (create_argv(cmd) == -1)
                return -1;

            for (size_t k = 0; k < cmd->redirs.size; ++k) {
                ps_redir *redir = &cmd->redirs.data[k];
                if (create_arg(&redir->target) == -1)
                    return -1;
            }
        }
    }

    return 0;
}
