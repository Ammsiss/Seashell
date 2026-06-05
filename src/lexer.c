#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static lx_kind get_kind(const char *cur_char) {
    if (strncmp("2>", cur_char, 2) == 0)
        return LX_TOK_RDR_ERR;
    if (strncmp(">>", cur_char, 2) == 0)
        return LX_TOK_APPEND;
    if (strncmp("&&", cur_char, 2) == 0)
        return LX_TOK_AND_IF;
    if (strncmp("||", cur_char, 2) == 0)
        return LX_TOK_OR_IF;

    if (strncmp("|", cur_char, 1) == 0)
        return LX_TOK_PIPE;
    if (strncmp("&", cur_char, 1) == 0)
        return LX_TOK_BG;
    if (strncmp("<", cur_char, 1) == 0)
        return LX_TOK_RDR_IN;
    if (strncmp(">", cur_char, 1) == 0)
        return LX_TOK_RDR_OUT;
    if (strncmp(";", cur_char, 1) == 0)
        return LX_TOK_SEMI;

    return LX_TOK_UNKNOWN;
}

static int kind_is_delim(lx_kind kind) {
    switch (kind) {
    case LX_TOK_PIPE:
    case LX_TOK_BG:
    case LX_TOK_RDR_IN:
    case LX_TOK_RDR_OUT:
    case LX_TOK_RDR_ERR:
    case LX_TOK_APPEND:
    case LX_TOK_SEMI:
    case LX_TOK_AND_IF:
    case LX_TOK_OR_IF:
        return 1;
    default:
        return 0;
    }
}

static int kind_is_double_char_op(lx_kind kind) {
    switch (kind) {
    case LX_TOK_RDR_ERR:
    case LX_TOK_APPEND:
    case LX_TOK_AND_IF:
    case LX_TOK_OR_IF:
        return 1;
    default:
        return 0;
    }
}

static int add_tok(da_lx_tok *tokens, lx_kind kind, lx_scanner *scanner) {
    assert(scanner);
    assert(!scanner->cur_tok);

    lx_tok *tok = da_lx_tok_push(tokens);
    if (!tok)
        return -1;

    tok->kind = kind;
    tok->parts = (da_lx_part){0};

    if (kind == LX_TOK_WORD)
        if (da_lx_part_init(&tok->parts, 0) == -1)
            return -1;

    scanner->cur_tok = tok;
    return 0;
}

static int ensure_tok(da_lx_tok *tokens, lx_kind kind, lx_scanner *scanner) {
    if (!scanner->cur_tok)
        if (add_tok(tokens, kind, scanner) == -1)
            return -1;
    return 0;
}

static void end_tok(lx_scanner *scanner) {
    assert(scanner);
    scanner->cur_tok = NULL;
}

static int add_part(lx_scanner *scanner, lx_quote quote) {
    assert(scanner);
    assert(scanner->cur_tok);
    assert(!scanner->part_start);

    lx_part *part = da_lx_part_push(&scanner->cur_tok->parts);
    if (!part)
        return -1;

    part->raw = NULL;
    part->quote = quote;

    scanner->part_start = scanner->cur_char;
    return 0;
}

static int ensure_part(lx_scanner *scanner, lx_quote quote) {
    if (!scanner->part_start)
        if (add_part(scanner, quote) == -1)
            return -1;
    return 0;
}

/* TODO: maybe refactor alloc from setting scanner->part_start */
static int end_part(lx_scanner *scanner) {
    assert(scanner);
    assert(scanner->cur_tok);
    assert(scanner->part_start);

    lx_part *part = da_lx_part_end(&scanner->cur_tok->parts);

    size_t len = 0;
    for (const char *c = scanner->part_start; c != scanner->cur_char; ++c)
        ++len;

    part->raw = malloc(len + 1);
    if (!part->raw)
        return -1;

    memcpy(part->raw, scanner->part_start, len);
    part->raw[len] = '\0';

    scanner->part_start = NULL;
    return 0;
}

static int clear_part(lx_scanner *scanner) {
    if (scanner->cur_tok && scanner->part_start)
        if (end_part(scanner) == -1)
            return -1;
    return 0;
}

static int clear_empty_part(lx_scanner *scanner) {
    lx_part *part = da_lx_part_end(&scanner->cur_tok->parts);

    part->raw = malloc(1);
    if (!part->raw)
        return -1;
    part->raw[0] = '\0';

    scanner->part_start = NULL;
    return 0;
}

static int handle_single_quote(lx_scanner *scanner) {
    if (*scanner->cur_char == '\'') {
        clear_part(scanner);
        scanner->mode = LX_M_NORMAL;
    }

    return 0;
}

static int handle_double_quote(lx_scanner *scanner) {
    if (*scanner->cur_char == '"') {
        clear_part(scanner);
        scanner->mode = LX_M_NORMAL;
    }

    return 0;
}

static int handle_normal(da_lx_tok *tokens, lx_scanner *scanner) {
    if (*scanner->cur_char == '"') {
        if (scanner->cur_char[1] == '\0')
            return -1;

        clear_part(scanner);
        ++scanner->cur_char;

        if (ensure_tok(tokens, LX_TOK_WORD, scanner) == -1)
            return -1;
        if (ensure_part(scanner, LX_Q_DOUBLE) == -1)
                return -1;

        if (*scanner->cur_char == '"') {
            clear_empty_part(scanner);
        } else {
            if (*scanner->cur_char == '\\')
                if (*++scanner->cur_char == '\0')
                    return -1;
            scanner->mode = LX_M_DOUBLEQ;
        }
    }

    else if (*scanner->cur_char == '\'') {
        if (scanner->cur_char[1] == '\0')
            return -1;

        clear_part(scanner);
        ++scanner->cur_char;

        if (ensure_tok(tokens, LX_TOK_WORD, scanner) == -1)
            return -1;
        if (ensure_part(scanner, LX_Q_SINGLE) == -1)
                return -1;

        if (*scanner->cur_char == '\'') {
            clear_empty_part(scanner);
        } else {
            scanner->mode = LX_M_SINGLEQ;
        }
    }

    else if (*scanner->cur_char == ' ') {
        if (clear_part(scanner) == -1)
            return -1;
        end_tok(scanner);
    } else {
        lx_kind kind = get_kind(scanner->cur_char);

        if (kind_is_delim(kind)) {
            if (clear_part(scanner) == -1)
                return -1;
            end_tok(scanner);

            if (ensure_tok(tokens, kind, scanner) == -1)
                return -1;
            end_tok(scanner);

            if (kind_is_double_char_op(kind))
                ++scanner->cur_char;
        }

        else if (kind == LX_TOK_UNKNOWN) {
            if (ensure_tok(tokens, LX_TOK_WORD, scanner) == -1)
                return -1;
            if (ensure_part(scanner, LX_Q_NONE) == -1)
                return -1;
            if (*scanner->cur_char == '\\')
                if (*++scanner->cur_char == '\0')
                    return -1;
        }
    }

    return 0;
}

void lx_free(da_lx_tok *tokens) {
    for (size_t i = 0; i < tokens->size; ++i) {
        lx_tok *tok = &tokens->data[i];
        if (tok->kind == LX_TOK_WORD) {
            for (size_t y = 0; y < tok->parts.size; ++y) {
                lx_part *part = &tok->parts.data[y];
                free(part->raw);
            }
            da_lx_part_free(&tok->parts);
        }
    }
    da_lx_tok_free(tokens);
}

int lx_tokenize(const char *cmd, da_lx_tok *tokens) {
    if (!cmd || !tokens)
        return -1;

    size_t cmd_len = strlen(cmd);
    if (cmd_len <= 0)
        return -1;

    if (da_lx_tok_init(tokens, 0) == -1)
        return -1;

    lx_scanner scanner = { LX_M_NORMAL, LX_M_NORMAL, NULL, NULL, cmd };

    for (; *scanner.cur_char != '\0'; ++scanner.cur_char) {
        switch (scanner.mode) {
        case LX_M_NORMAL:
            if (handle_normal(tokens, &scanner) == -1)
                goto fail;
            break;
        case LX_M_DOUBLEQ:
            if (handle_double_quote(&scanner) == -1)
                goto fail;
            break;
        case LX_M_SINGLEQ:
            if (handle_single_quote(&scanner) == -1)
                goto fail;
            break;
        }
    }

    if (scanner.mode == LX_M_DOUBLEQ || scanner.mode == LX_M_SINGLEQ)
        goto fail;

    if (tokens->size == 0)
        da_lx_tok_free(tokens);

    if (clear_part(&scanner) == -1)
        goto fail;
    end_tok(&scanner);

    return 0;

fail:
    lx_free(tokens);
    return -1;
}
