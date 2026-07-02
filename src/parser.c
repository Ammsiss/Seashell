#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "lexer.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "dyn_arr.h"

static int init_ps_segment(ps_segment *segment) {
    assert(segment);

    *segment = (ps_segment){0};

    return 0;
}

static void free_ps_segment(ps_segment *segment) {
    free(segment->raw);
    *segment = (ps_segment){0};
}

static int init_word(ps_word *word) {
    assert(word);

    *word = (ps_word){0};
    if (da_init(&word->segments) == -1)
        return -1;

    return 0;
}

static void free_word(ps_word *word) {
    if (!word)
        return;

    free(word->arg);

    for (size_t i = 0; i < word->segments.size; ++i)
        free_ps_segment(&word->segments.data[i]);
    da_free(&word->segments);
    *word = (ps_word){0};
}

static int init_redir(ps_redir *redir) {
    assert(redir);

    *redir = (ps_redir){0};
    if (da_init(&redir->target.segments) == -1)
        return -1;

    return 0;
}

static void free_redir(ps_redir *redir) {
    if (!redir)
        return;

    free_word(&redir->target);
    *redir = (ps_redir){0};
}

static int init_cmd(ps_cmd *cmd) {
    assert(cmd);

    *cmd = (ps_cmd){0};
    if (da_init(&cmd->words) == -1)
        return -1;
    if (da_init(&cmd->redirs) == -1)
        return -1;

    return 0;
}

static void free_cmd(ps_cmd *cmd) {
    if (!cmd)
        return;

    free(cmd->argv);

    for (size_t i = 0; i < cmd->words.size; ++i)
        free_word(&cmd->words.data[i]);
    for (size_t i = 0; i < cmd->redirs.size; ++i)
        free_redir(&cmd->redirs.data[i]);

    da_free(&cmd->words);
    da_free(&cmd->redirs);

    *cmd = (ps_cmd){0};
}

static int init_pipeline(ps_pipeline *pipeline) {
    assert(pipeline);

    *pipeline = (ps_pipeline){0};
    if (da_init(&pipeline->cmds) == -1)
        return -1;

    return 0;
}

static void free_pipeline(ps_pipeline *pipeline) {
    if (!pipeline)
        return;

    for (size_t i = 0; i < pipeline->cmds.size; ++i)
        free_cmd(&pipeline->cmds.data[i]);
    da_free(&pipeline->cmds);

    *pipeline = (ps_pipeline){0};
}

static int init_andor(ps_andor *andor) {
    assert(andor);

    *andor = (ps_andor){0};
    if (init_pipeline(&andor->pipeline) == -1)
        return -1;

    return 0;
}

static void free_andor(ps_andor *andor) {
    if (!andor)
        return;

    free_pipeline(&andor->pipeline);

    *andor = (ps_andor){0};
}

static int init_job(ps_job *job) {
    assert(job);

    *job = (ps_job){0};
    if (da_init(&job->andors) == -1)
        return -1;

    return 0;
}

static int cpy_word(ps_word *dst, lx_tok *src) {
    assert(dst);
    assert(dst->segments.size == 0);

    for (size_t i = 0; i < src->parts.size; ++i) {
        lx_part *src_part = &src->parts.data[i];

        ps_segment *dst_segment = da_push_init(&dst->segments, init_ps_segment);
        if (!dst_segment)
            return -1;

        size_t n = strlen(src_part->raw) + 1;

        dst_segment->raw = malloc(n);
        if (!dst_segment->raw)
            return -1;

        memcpy(dst_segment->raw, src_part->raw, n);

        switch (src_part->quote) {
            case LX_Q_SINGLE: dst_segment->quote = PS_Q_SINGLE; break;
            case LX_Q_DOUBLE: dst_segment->quote = PS_Q_DOUBLE; break;
            case LX_Q_NONE:   dst_segment->quote = PS_Q_NONE;   break;
        }
    }

    return 0;
}

static int add_andor(da_andor *andors, ps_andor_op op,
        ps_scanner *scanner) {
    assert(andors);

    ps_andor *andor = da_push_init(andors, init_andor);
    if (!andor)
        return -1;

    andor->op = op;

    scanner->cur_pipeline = &andor->pipeline;

    return 0;
}

static int ensure_pipeline(da_andor *andors, ps_andor_op op,
        ps_scanner *scanner) {
    if (!scanner->cur_pipeline)
        if (add_andor(andors, op, scanner) == -1)
            return -1;

    return 0;
}

static int add_cmd(da_cmd *cmds, ps_scanner *scanner) {
    assert(scanner);

    ps_cmd *cmd = da_push_init(cmds, init_cmd);
    if (!cmd)
        return -1;

    scanner->cur_cmd = cmd;

    return 0;
}

static int ensure_cmd(ps_pipeline *pipeline, ps_scanner *scanner) {
    if (!scanner->cur_cmd)
        if (add_cmd(&pipeline->cmds, scanner) == -1)
            return -1;

    return 0;
}

static int queue_redir(lx_kind kind, int append, ps_scanner *scanner) {
    assert(scanner);

    ps_redir *redir = da_push_init(&scanner->cur_cmd->redirs, init_redir);
    if (!redir)
        return -1;

    redir->append = append;

    switch (kind) {
    case LX_TOK_RDR_IN:  redir->io_num = 0; break;
    case LX_TOK_RDR_OUT: redir->io_num = 1; break;
    case LX_TOK_RDR_ERR: redir->io_num = 2; break;
    case LX_TOK_APPEND:  redir->io_num = 1; break;
    default:             return -1;
    }

    scanner->queued_redir = redir;
    return 0;
}

void ps_free(ps_job *job) {
    if (!job)
        return;

    for (size_t i = 0; i < job->andors.size; ++i)
        free_andor(&job->andors.data[i]);
    da_free(&job->andors);

    *job = (ps_job){0};
}

int ps_parse(da_tok *tokens, ps_job *job) {
    if (!tokens || !job)
        return -1;

    if (tokens->size == 0)
        return -1;

    if (init_job(job) == -1)
        goto fail;

    ps_scanner scanner = {0};
    scanner.cur_tok = &tokens->data[0];
    scanner.cur_andor_op = PS_NO_IF;

    for (size_t i = 0; i < tokens->size; ++i, ++scanner.cur_tok) {
        if (!scanner.cur_pipeline || !scanner.cur_cmd || scanner.queued_redir) {
            if (scanner.cur_tok->kind != LX_TOK_WORD)
                goto fail;
        }

        switch (scanner.cur_tok->kind) {
        case LX_TOK_WORD: {
            if (scanner.queued_redir) {
                if (cpy_word(&scanner.queued_redir->target, scanner.cur_tok) == -1)
                    goto fail;
                scanner.queued_redir = NULL;
                continue;
            }

            ensure_pipeline(&job->andors, scanner.cur_andor_op, &scanner);
            ensure_cmd(scanner.cur_pipeline, &scanner);

            ps_word *word = da_push_init(&scanner.cur_cmd->words, init_word);
            if (init_word(word) == -1)
                goto fail;

            if (cpy_word(word, scanner.cur_tok) == -1)
                goto fail;

            break;
        }

        case LX_TOK_PIPE:
            scanner.cur_cmd = NULL;
            break;

        case LX_TOK_AND_IF:
            scanner.cur_cmd = NULL;
            scanner.cur_pipeline = NULL;
            scanner.cur_andor_op = PS_AND_IF;
            break;

        case LX_TOK_OR_IF:
            scanner.cur_cmd = NULL;
            scanner.cur_pipeline = NULL;
            scanner.cur_andor_op = PS_OR_IF;
            break;

        case LX_TOK_BG: {
            if (i != tokens->size - 1)
                goto fail;

            job->bg = 1;
            break;
        }

        case LX_TOK_RDR_IN:
        case LX_TOK_RDR_OUT:
        case LX_TOK_RDR_ERR:
        case LX_TOK_APPEND: {
            if (!scanner.cur_pipeline || !scanner.cur_cmd)
                goto fail;

            const lx_kind kind = scanner.cur_tok->kind;
            const int append = kind == LX_TOK_APPEND;

            if (queue_redir(kind, append, &scanner) == -1)
                goto fail;
            break;
        }
        default:
            break;
        }
    }

    if (scanner.queued_redir || !scanner.cur_cmd)
        goto fail;

    return 0;

fail:
    ps_free(job);
    return -1;
}
