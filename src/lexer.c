#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

int get_tok(char c, enum lx_code *kind) {
    switch (c) {
    case '|': *kind = LX_TOK_PIPE; return 1;
    case '<': *kind = LX_TOK_RDR_IN; return 1;
    case '>': *kind = LX_TOK_RDR_OUT; return 1;
    case '&': *kind = LX_TOK_BG; return 1;
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
        enum lx_code kind, const char *start, size_t len) {
    struct lx_tok *tok = lx_push_tok(list, size, cap);
    if (tok == NULL)
        return -1;

    tok->kind = kind;

    if (start != NULL) {
    /* then remove dbqts and allocate storage for the word */
        tok->value = malloc(len + 1);
        if (tok->value == NULL)
            return -1;

        size_t value_idx = 0;
        for (size_t i = 0; i < len; ++i) {
            if (start[i] == '\"')
                continue;

            tok->value[value_idx++] = start[i];
        }

        tok->value[value_idx] = '\0';
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
    size_t tok_start_idx = 0;

    int delim_on = 1;

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

        if (tok_start == NULL) {
        /* then we are at the start of a new LX_TOK_WORD */
            tok_start = &cmd[i];
            tok_start_idx = i;
        }
    }

    /* Unmatched double quote */
    if (!delim_on)
        goto fail;

    if (tok_start != NULL) {
    /* then the command ended with a LX_TOK_WORD */
        if (lx_add_tok(&list, &size, &cap, LX_TOK_WORD,
                    tok_start, cmd_len - tok_start_idx) == -1)
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
