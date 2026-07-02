#include <string.h>

#include "expander.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes

static int expand_word(ps_word *word) {
    char *expanded_word = malloc(1);
    if (!expanded_word)
        return -1;
    expanded_word[0] = '\0';

    size_t expanded_len = 1;

    for (size_t i = 0; i < word->segments.size; ++i) {
        const ps_segment *segment = &word->segments.data[i];

        expanded_len += strlen(segment->raw);

        char *tmp = realloc(expanded_word, expanded_len);
        if (!tmp)
            return -1;
        expanded_word = tmp;

        strcat(expanded_word, segment->raw);
    }

    word->arg = expanded_word;

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

int ex_expand(ps_job *job) {
    for (size_t i = 0; i < job->andors.size; ++i) {
        ps_andor *andor = &job->andors.data[i];
        ps_pipeline *pipeline = &andor->pipeline;

        for (size_t j = 0; j < pipeline->cmds.size; ++j) {
            ps_cmd *cmd = &pipeline->cmds.data[j];

            for (size_t k = 0; k < cmd->words.size; ++k) {
                ps_word *word = &cmd->words.data[k];
                if (expand_word(word) == -1)
                    return -1;
            }

            if (create_argv(cmd) == -1)
                return -1;
        }
    }

    return 0;
}
