#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

/* Expects *c to point to a null terminated string and
 * that it is not pointing at the terminating null byte */
lx_kind lx_get_kind(const char *c) {
    if (strncmp("<<", c, 2) == 0)
        return LX_TOK_HDOC;
    if (strncmp(">>", c, 2) == 0)
        return LX_TOK_APPEND;
    if (strncmp("&&", c, 2) == 0)
        return LX_TOK_AND_IF;
    if (strncmp("||", c, 2) == 0)
        return LX_TOK_OR_IF;
    if (strncmp("1>", c, 2) == 0)
        return LX_TOK_RDR_STDOUT;
    if (strncmp("2>", c, 2) == 0)
        return LX_TOK_RDR_STDERR;

    if (strncmp("|", c, 1) == 0)
        return LX_TOK_PIPE;
    if (strncmp("&", c, 1) == 0)
        return LX_TOK_BG;
    if (strncmp("<", c, 1) == 0)
        return LX_TOK_RDR_IN;
    if (strncmp(">", c, 1) == 0)
        return LX_TOK_RDR_OUT;
    if (strncmp("(", c, 1) == 0)
        return LX_TOK_LPAREN;
    if (strncmp(")", c, 1) == 0)
        return LX_TOK_RPAREN;
    if (strncmp(";", c, 1) == 0)
        return LX_TOK_SEMI;

    return LX_TOK_UNKNOWN;
}

int lx_kind_is_double_char_op(lx_kind kind) {
    switch (kind) {
    case LX_TOK_HDOC:
    case LX_TOK_APPEND:
    case LX_TOK_AND_IF:
    case LX_TOK_OR_IF:
    case LX_TOK_RDR_STDOUT:
    case LX_TOK_RDR_STDERR:
        return 1;
    default:
        return 0;
    }
}

int lx_kind_is_delim(lx_kind kind) {
    switch (kind) {
    /* Single ops */
    case LX_TOK_PIPE:
    case LX_TOK_BG:
    case LX_TOK_RDR_IN:
    case LX_TOK_RDR_OUT:
    case LX_TOK_LPAREN:
    case LX_TOK_RPAREN:
    case LX_TOK_SEMI:
    /* Double ops */
    case LX_TOK_HDOC:
    case LX_TOK_APPEND:
    case LX_TOK_AND_IF:
    case LX_TOK_OR_IF:
    case LX_TOK_RDR_STDOUT:
    case LX_TOK_RDR_STDERR:
        return 1;
    default:
        return 0;
    }
}

void lx_free(da_lx_tok *list) {
    for (size_t i = 0; i < list->size; ++i)
        if (list->data[i].kind == LX_TOK_WORD)
            free(list->data[i].value);
    da_lx_tok_free(list);
}

int lx_add_tok(da_lx_tok *list, lx_kind kind, lx_scanner *scanner) {
    lx_tok *tok = da_lx_tok_push(list);
    if (!tok)
        return -1;

    tok->kind = kind;

    if (scanner) {
        size_t len = 0;
        for (const char *c = scanner->tok_start; c != scanner->tok_cur; ++c)
            ++len;

        tok->value = malloc(len + 1);
        if (!tok->value)
            return -1;

        memcpy(tok->value, scanner->tok_start, len);
        tok->value[len] = '\0';
    } else
        tok->value = NULL;

    return 0;
}

int lx_flush_word(da_lx_tok *list, lx_scanner *scanner) {
    if (scanner->tok_start) {
        if (lx_add_tok(list, LX_TOK_WORD, scanner) == -1)
            return -1;
        scanner->tok_start = NULL;
    }

    return 0;
}

int lx_handle_normal(da_lx_tok *list, lx_scanner *scanner) {
    if (*scanner->tok_cur == '"') {
        scanner->mode = LX_M_Q_DOUBLE;
    } else if (*scanner->tok_cur == '\\') {
        scanner->mode = LX_M_BACKSLASH;
    } else if (*scanner->tok_cur == ' ') {
        if (lx_flush_word(list, scanner) == -1)
            return -1;
        return 0;
    }

    lx_kind tok_kind = lx_get_kind(scanner->tok_cur);

    if (tok_kind == LX_TOK_UNKNOWN) {
        if (!scanner->tok_start)
            scanner->tok_start = scanner->tok_cur;
    } else {
        if (lx_flush_word(list, scanner) == -1)
            return -1;
        if (lx_add_tok(list, tok_kind, NULL) == -1)
            return -1;
        if (lx_kind_is_double_char_op(tok_kind))
            ++scanner->tok_cur;
    }

    return 0;
}

void lx_handle_double_quote(lx_scanner *scanner) {
    if (*scanner->tok_cur == '"')
        scanner->mode = LX_M_NORMAL;

    if (*scanner->tok_cur == '\\')
        scanner->mode = LX_M_BACKSLASH;

    if (!scanner->tok_start)
        scanner->tok_start = scanner->tok_cur;
}

void lx_handle_after_backslash(lx_scanner *scanner, lx_mode prev_mode) {
    if (!scanner->tok_start)
        scanner->tok_start = scanner->tok_cur;

    scanner->mode = prev_mode;
}

int lx_tokenize(const char *cmd, da_lx_tok *list) {
    if (!cmd || !list)
        return -1;

    size_t cmd_len = strlen(cmd);
    if (cmd_len <= 0)
        return -1;

    if (da_lx_tok_init(list, 0) == -1)
        return -1;

    lx_scanner scanner = { LX_M_NORMAL, NULL, cmd };
    lx_mode prev_mode = LX_M_NORMAL;

    for (; *scanner.tok_cur != '\0'; ++scanner.tok_cur) {
        if (scanner.mode == LX_M_NORMAL) {
            if (lx_handle_normal(list, &scanner) == -1)
                goto fail;
            prev_mode = LX_M_NORMAL;
        } else if (scanner.mode == LX_M_Q_DOUBLE) {
            lx_handle_double_quote(&scanner);
            prev_mode = LX_M_Q_DOUBLE;
        } else if (scanner.mode == LX_M_BACKSLASH) {
            lx_handle_after_backslash(&scanner, prev_mode);
            prev_mode = LX_M_BACKSLASH;
        }
    }

    if (scanner.mode == LX_M_Q_DOUBLE)
        goto fail;

    if (lx_flush_word(list, &scanner) == -1)
        goto fail;

    return 0;
fail:
    lx_free(list);
    return -1;
}
