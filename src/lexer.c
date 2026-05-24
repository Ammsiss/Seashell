#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "dyn_arr.h"

/* Expects *c to point to a null terminated string and
 * that it is not pointing at the terminating null byte */
enum lx_kind lx_get_kind(const char *c) {
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

    return LX_TOK_WORD;
}

int lx_kind_is_double_char_op(enum lx_kind kind) {
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

int lx_kind_is_delim(enum lx_kind kind) {
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

void lx_free(struct dyn_arr *list) {
    for (size_t i = 0; i < list->size; ++i)
        if (DA_GET(list, i, struct lx_tok)->kind == LX_TOK_WORD)
            free(DA_GET(list, i, struct lx_tok)->value);
    da_free(list);
}

int lx_add_tok(struct dyn_arr *list, enum lx_kind kind, const char *start,
        const char *end) {
    struct lx_tok *tok = da_push(list);
    if (tok == NULL)
        return -1;

    tok->kind = kind;

    if (start != NULL && end != NULL) {
        size_t len = 0;
        for (const char *c = start; c != end; ++c)
            ++len;

        tok->value = malloc(len + 1);
        if (tok->value == NULL)
            return -1;

        memcpy(tok->value, start, len);
        tok->value[len] = '\0';
    } else
        tok->value = NULL;

    return 0;
}

int lx_tokenize(const char *cmd, struct dyn_arr *list) {
    size_t cmd_len = strlen(cmd);
    if (cmd == NULL || cmd_len == 0 || list == NULL)
        return -1;

    if (da_init(list, 0, sizeof(struct lx_tok)) == -1)
        return -1;

    enum lx_kind tok_kind;
    const char *tok_start = NULL;

    int delim_on = 1;

    for (const char *c = cmd; *c != '\0'; ++c) {

        switch (*c) {
        case '"':
            delim_on = (delim_on) ? 0 : 1;
            if (delim_on)
                continue;
            break;
        case ' ':
            if (delim_on) {
                if (tok_start != NULL)
                    if (lx_add_tok(list, LX_TOK_WORD, tok_start, c) == -1)
                        goto fail;
                tok_start = NULL;
            }
            continue;
        case '\\':
            if (c[1] == '\0')
                goto fail;
            if (!delim_on) {
                if (c[1] == '\"' || c[1] == '\\') {
                    ++c;
                    continue;
                }
            }
            break;
        default:
            break;
        }

        tok_kind = lx_get_kind(c);

        if (delim_on && lx_kind_is_delim(tok_kind)) {
            /* Tokenize Word */
            if (tok_start != NULL)
                if (lx_add_tok(list, LX_TOK_WORD, tok_start, c) == -1)
                    goto fail;

            /* Tokenize Operator */
            if (lx_add_tok(list, tok_kind, NULL, NULL) == -1)
                goto fail;

            /* This must be after tokenizing the word */
            if (lx_kind_is_double_char_op(tok_kind))
                ++c;

            tok_start = NULL;
            continue;
        }

        if (tok_start == NULL)
            tok_start = c;
    }

    /* Unmatched double quote */
    if (!delim_on)
        goto fail;

    /* Tokenize Final Word */
    if (tok_start != NULL)
        if (lx_add_tok(list, LX_TOK_WORD, tok_start, &cmd[cmd_len]) == -1)
            goto fail;

    return 0;
fail:
    lx_free(list);
    return -1;
}
