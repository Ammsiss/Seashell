#include <stdio.h>
#include <string.h>

#include "shell_state.h"
#include "expander.h"
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "dyn_str.h"

static char *get_key(char **c) {
    d_str var_key;
    if (d_str_init(&var_key) == -1)
        return NULL;

    for (; **c != '\0'; ++*c) {
        if (**c == ' ' || **c == '\\' || **c == '$') {
            --*c;
            break;
        }

        if (d_str_push(&var_key, **c) == -1) {
            d_str_free(&var_key);
            return NULL;
        }
    }

    return var_key.c_str;
}

static char *expand_variable(char **c) {
    char *key = get_key(c);
    if (!key) {
        free(key);
        return NULL;
    }

    char *value = st_lookup_var(key);

    free(key);
    return value;
}

static char *expand_segment_expansion(ps_segment *segment) {
    d_str big_seg;
    if (d_str_init(&big_seg) == -1)
        return NULL;

    for (char *c = &segment->raw[0]; *c != '\0'; ++c) {
        if (*c == '\\') {
            ++c;
            if (*c == '\0')
                break;

            if (d_str_push(&big_seg, *c) == -1)
                goto fail;

            continue;
        }

        if (*c == '$') {
            ++c;
            char *value = expand_variable(&c);
            if (value)
                if (d_strcat(&big_seg, value) == -1)
                    goto fail;

            if (*c == '\0')
                break;

            continue;
        }

        if (d_str_push(&big_seg, *c) == -1)
            goto fail;
    }

    return big_seg.c_str;

fail:
    d_str_free(&big_seg);
    return NULL;
}

static char *expand_segment_plain(ps_segment *segment) {
    d_str big_seg;
    if (d_str_init(&big_seg) == -1)
        return NULL;
    if (d_strcat(&big_seg, segment->raw) == -1)
        goto fail;

    return big_seg.c_str;

fail:
    d_str_free(&big_seg);
    return NULL;
}

static char *expand_segment(ps_segment *segment) {
    switch (segment->quote) {
    case LX_Q_NONE:
    case LX_Q_DOUBLE:
        return expand_segment_expansion(segment);
    case LX_Q_SINGLE:
        return expand_segment_plain(segment);
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
