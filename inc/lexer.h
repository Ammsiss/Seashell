#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

enum lx_code {
    /* TOKENS */
    LX_TOK_WORD,
    LX_TOK_END,
    /* Operators */
    LX_TOK_PIPE, // |
    LX_TOK_REDL, // <
    LX_TOK_REDR, // >
    LX_TOK_INBG, // &
    /* Builtins */
    LX_TOK_EXIT,
    LX_TOK_CHDR,
    LX_TOK_JOBS,
    LX_TOK_FG,
    LX_TOK_BG,

    /* SPECIAL CHARACTERS */
    LX_DBQT, // Double quote
    LX_WTSP, // White space
};

struct lx_tok {
    enum lx_code kind;
    char *value;
};

struct lx_tok *lx_tokenize(const char *cmd);

#endif
