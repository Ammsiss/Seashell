/* Finish a word and add a token */
/* Add a token for a operator */
/* Keep track of double quote count */
/* White space should delim except when double quote count is 1 */
/* White space does not create a token */

#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

int get_tok(char c, enum lx_code *kind) {
    switch (c) {
    case '|': *kind = LX_TOK_PIPE; return 1;
    case '<': *kind = LX_TOK_REDL; return 1;
    case '>': *kind = LX_TOK_REDR; return 1;
    case '&': *kind = LX_TOK_INBG; return 1;
    case '\"': *kind = LX_DBQT; return 1;
    case ' ': case '\t': case '\v': case '\r':
        *kind = LX_WTSP; return 1;
    default: *kind = LX_TOK_WORD; return 0;
    }
}

/* Expects initially mallocated list of at least 1 element */
struct lx_tok *lx_push_tok(struct lx_tok **list, size_t *size,
        size_t *cap) {
    if (list == NULL || *list == NULL || size == NULL || cap == NULL ||
            *size > *cap || *cap <= 0 || *size < 0) {
        errno = EINVAL;
        return NULL;
    }

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
        enum lx_code kind, const char *start, size_t len) {
    struct lx_tok *tok = lx_push_tok(list, size, cap);
    if (tok == NULL)
        return -1;

    tok->kind = kind;

    if (start != NULL) {
        tok->value = malloc(len + 1); /* +1 for '\0' */
        if (tok->value == NULL)
            return -1;
        memcpy(tok->value, start, len);
        tok->value[len] = '\0';
    } else
        tok->value = NULL;

    return 0;
}

struct lx_tok *lx_tokenize(const char *cmd) {
    if (cmd == NULL || strlen(cmd) == 0) {
        errno = EINVAL;
        return NULL;
    }

    size_t size = 0;
    size_t cap = 1;

    struct lx_tok *list = malloc(cap * sizeof(struct lx_tok));
    if (list == NULL)
        return NULL;

    enum lx_code tok_kind;
    const char *tok_start = NULL;
    size_t tok_start_idx = 0;

    int delim_on = 1;

    size_t cmd_len = strlen(cmd);
    for (size_t i = 0; i < cmd_len; ++i) {

        if (get_tok(cmd[i], &tok_kind)) {
            if (tok_kind == LX_DBQT) {
                delim_on = (delim_on) ? 0 : 1;

                if (delim_on)
                    continue;
            }

            if (delim_on) {
            /* then delimiters be delimitting */
                if (tok_start != NULL) {
                /* then this must point at a LX_TOK_WORD */
                    if (lx_add_tok(&list, &size, &cap, LX_TOK_WORD,
                                tok_start, i - tok_start_idx) == -1)
                        goto fail;
                    tok_start = NULL;
                }

                if (tok_kind != LX_WTSP) {
                    if (lx_add_tok(&list, &size, &cap, tok_kind, NULL, 0) == -1)
                        goto fail;
                }

                continue;
            }
        }

        /* TODO: Check for builtins */

        if (tok_start == NULL) {
        /* then we are at the start of a new LX_TOK_WORD */
            tok_start = &cmd[i];
            tok_start_idx = i;
        }
    }

    if (tok_start != NULL) {
    /* then the command ended with a LX_TOK_WORD */
        if (lx_add_tok(&list, &size, &cap, LX_TOK_WORD,
                    tok_start, cmd_len - tok_start_idx) == -1)
            goto fail;
    }

    if (lx_add_tok(&list, &size, &cap, LX_TOK_END, NULL, 0) == -1)
        goto fail;

    return list;

fail:
    free(list);
    return NULL;
}
