#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

/* Expects *c to point to a null terminated string and
 * that it is not pointing at the terminating null byte */
enum lx_code lx_get_kind(const char *c) {
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

int lx_kind_is_double_char_op(enum lx_code kind) {
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

int lx_kind_is_delim(enum lx_code kind) {
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

/* Expects initially mallocated list of at least 1 element */
struct lx_tok *lx_push_tok(struct lx_tok **list, size_t *size,
        size_t *cap) {
    if (list == NULL || *list == NULL || size == NULL || cap == NULL ||
            *size > *cap || *cap <= 0 || *size < 0)
        return NULL;

    if (*size < *cap) {
        *size += 1;
        return &(*list)[*size - 1];
    }

    struct lx_tok *new_ptr = reallocarray(*list,
            *cap * 2, sizeof(struct lx_tok));
    if (new_ptr == NULL)
        return NULL;

    *size += 1;
    *cap *= 2;
    *list = new_ptr;

    return &new_ptr[*size - 1];
}

int lx_add_tok(struct lx_tok **list, size_t *size, size_t *cap,
        enum lx_code kind, const char *start, const char *end) {
    struct lx_tok *tok = lx_push_tok(list, size, cap);
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

int lx_tokenize(const char *cmd, struct lx_tok **out_list, size_t *out_len) {
    size_t cmd_len = 0;

    if (cmd == NULL || out_list == NULL || out_len == NULL ||
            (cmd_len = strlen(cmd)) == 0)
        return -1;

    *out_list = NULL;
    *out_len = 0;

    size_t size = 0;
    size_t cap = 1;

    struct lx_tok *list = malloc(cap * sizeof(struct lx_tok));
    if (list == NULL)
        return -1;

    enum lx_code tok_kind;
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
                    if (lx_add_tok(&list, &size, &cap, LX_TOK_WORD,
                                tok_start, c) == -1)
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
                if (lx_add_tok(&list, &size, &cap, LX_TOK_WORD,
                            tok_start, c) == -1)
                    goto fail;

            /* Tokenize Operator */
            if (lx_add_tok(&list, &size, &cap, tok_kind, NULL, NULL) == -1)
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
    if (tok_start != NULL) {
        if (lx_add_tok(&list, &size, &cap, LX_TOK_WORD,
                    tok_start, &cmd[cmd_len]) == -1)
            goto fail;
    }

    *out_list = list;
    *out_len = size;

    return 0;

fail:
    for (size_t i = 0; i < size; ++i)
        free(list[i].value);
    free(list);
    return -1;
}
