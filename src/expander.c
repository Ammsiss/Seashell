#include <string.h>

#include "expander.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "dyn_str.h"


/*

for each character:
    if character is $ then
        Store character pointer to char after $

        for each character:
            if character is whitespace or \0
                store pointer to previous char; end variable

        expand variable and append result to d_str

*/

static char *expand_segment_none(ps_segment *segment) {
    d_str big_seg;
    d_str_init(&big_seg);
    d_strcat(&big_seg, segment->raw);

    return big_seg.c_str;
}

static char *expand_segment_double(ps_segment *segment) {
    d_str big_seg;
    d_str_init(&big_seg);
    d_strcat(&big_seg, segment->raw);

    return big_seg.c_str;
}

static char *expand_segment_single(ps_segment *segment) {
    d_str big_seg;
    d_str_init(&big_seg);
    d_strcat(&big_seg, segment->raw);

    return big_seg.c_str;
}

static char *expand_segment(ps_segment *segment) {
    switch (segment->quote) {
    case LX_Q_NONE:
        return expand_segment_none(segment);
    case LX_Q_DOUBLE:
        return expand_segment_double(segment);
    case LX_Q_SINGLE:
        return expand_segment_single(segment);
    }
}

static int create_arg(ps_word *word) {
    d_str arg;
    d_str_init(&arg);

    char *big_seg;

    for (size_t i = 0; i < word->segments.size; ++i) {
        big_seg = expand_segment(&word->segments.data[i]);
        if (!big_seg)
            goto fail;

        if (d_strcat(&arg, big_seg) == -1)
            goto fail;
        free(big_seg);
    }

    word->arg = arg.c_str;
    return 0;

fail:
    d_str_free(&arg);
    free(big_seg);

    return -1;
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
