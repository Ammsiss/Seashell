#include <string.h>

#include "expander.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes

static char *expand_segment_none(ps_segment *segment) {
    return segment->raw;
}

static char *expand_segment_double(ps_segment *segment) {
    return segment->raw;
}

static char *expand_segment(ps_segment *segment) {
    switch (segment->quote) {
    case LX_Q_NONE:
        return expand_segment_none(segment);
    case LX_Q_DOUBLE:
        return expand_segment_double(segment);
    case LX_Q_SINGLE:
        return segment->raw;
    }
}

static int create_arg(ps_word *word) {
    size_t arg_len = 0;
    for (size_t i = 0; i < word->segments.size; ++i) {
        char *segment = expand_segment(&word->segments.data[i]);
        arg_len += strlen(segment);
    }

    char *arg = malloc(arg_len + 1);
    if (!arg)
        return -1;
    arg[0] = '\0';

    for (size_t i = 0; i < word->segments.size; ++i)
        strcat(arg, word->segments.data[i].raw);

    word->arg = arg;

    return 0;
}

// static int expand_redir(ps_redir *redir) {
// }

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

int ex_expand(ps_job *job) {
    for (size_t i = 0; i < job->andors.size; ++i) {
        ps_andor *andor = &job->andors.data[i];
        ps_pipeline *pipeline = &andor->pipeline;

        for (size_t j = 0; j < pipeline->cmds.size; ++j) {
            ps_cmd *cmd = &pipeline->cmds.data[j];

            for (size_t k = 0; k < cmd->words.size; ++k) {
                ps_word *word = &cmd->words.data[k];
                if (create_arg(word) == -1)
                    return -1;
            }

            if (create_argv(cmd) == -1)
                return -1;
        }
    }

    return 0;
}
