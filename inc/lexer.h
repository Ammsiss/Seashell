#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

enum lx_token_kind {
    LX_TOKEN_WORD,
    /* Operators */
    LX_TOKEN_PIPE, // |
    LX_TOKEN_REDL, // <
    LX_TOKEN_REDR, // >
    LX_TOKEN_INBG, // &
    /* Special characters */
    LX_TOKEN_DBQT, // Double quote
    LX_TOKEN_WTSP, // White space
    /* Builtins */
    LX_TOKEN_EXIT,
    LX_TOKEN_CHDR,
    LX_TOKEN_JOBS,
    LX_TOKEN_FG,
    LX_TOKEN_BG,

    LX_TOKEN_EOF,
    LX_TOKEN_END,
};

struct lx_token {
    enum lx_token_kind kind;
    const char *start;
    int len;
};

struct lx_token *lx_tokenize(const char *cmd);

#endif
