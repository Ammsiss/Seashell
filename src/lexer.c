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

int get_token(char c, enum lx_token_kind *kind) {
    switch (c) {
    case '|': *kind = LX_TOKEN_PIPE; return 1;
    case '<': *kind = LX_TOKEN_REDL; return 1;
    case '>': *kind = LX_TOKEN_REDR; return 1;
    case '&': *kind = LX_TOKEN_INBG; return 1;
    case '\"': *kind = LX_TOKEN_DBQT; return 1;
    case ' ': case '\t': case '\v': case '\r':
        *kind = LX_TOKEN_WTSP; return 1;
    default: *kind = LX_TOKEN_WORD; return 0;
    }
}

/* Expects initially mallocated list of at least 1 element */
struct lx_token *lx_push_token(struct lx_token **list, int *size, int *cap) {
    if (list == NULL || *list == NULL || size == NULL || cap == NULL ||
            *size > *cap || *cap <= 0 || *size < 0) {
        errno = EINVAL;
        return NULL;
    }

    if (*size < *cap) {
        *size += 1;
        return &(*list)[*size - 1];
    }

    struct lx_token *new_ptr = reallocarray(*list,
            *cap * 2, sizeof(struct lx_token));
    if (new_ptr == NULL)
        return NULL;

    *size += 1;
    *cap *= 2;
    *list = new_ptr;

    return &new_ptr[*size - 1];
}

struct lx_token *lx_tokenize(const char *cmd) {
    if (cmd == NULL || strlen(cmd) == 0) {
        errno = EINVAL;
        return NULL;
    }

    int size = 0;
    int cap = 1;

    struct lx_token *list = malloc(cap * sizeof(struct lx_token));
    if (list == NULL)
        return NULL;

    struct lx_token *token;
    enum lx_token_kind token_kind;
    const char *token_start = NULL;
    size_t token_start_index;

    int delim_on = 1;


    for (size_t i = 0; i < strlen(cmd); ++i) {

        if (get_token(cmd[i], &token_kind)) {

            if (token_kind == LX_TOKEN_DBQT) {
                delim_on = (delim_on) ? 0 : 1;
                continue;
            }

            if (delim_on) {
            /* then delimiters be delimitting */
                if (token_start != NULL) {
                /* then this must point at a LX_TOKEN_WORD */
                    token = lx_push_token(&list, &size, &cap);
                    token->kind = LX_TOKEN_WORD;
                    token->start = token_start;
                    token->len = i - token_start_index;
                    token_start = NULL;
                }

                if (token_kind != LX_TOKEN_WTSP) {
                    token = lx_push_token(&list, &size, &cap);
                    if (token == NULL)
                        return NULL;

                    token->kind = token_kind;
                }

                continue;
            }
        }

        /* TODO: Check for builtins */

        if (token_start == NULL) {
        /* then we are at the start of a new LX_TOKEN_WORD */
            token_start = &cmd[i];
            token_start_index = i;
        }
    }

    if (token_start != NULL) {
    /* then the command ended with a LX_TOKEN_WORD */
        token = lx_push_token(&list, &size, &cap);
        token->kind = LX_TOKEN_WORD;
        token->start = token_start;
        token->len = strlen(cmd) - token_start_index;
        token_start = NULL;
    }

    token = lx_push_token(&list, &size, &cap);
    token->kind = LX_TOKEN_END;

    return list;
}
