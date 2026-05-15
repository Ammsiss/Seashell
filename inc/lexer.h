#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

enum lx_code {
    /* TOKENS */
    LX_TOK_WORD,
    /* Operators */
    LX_TOK_PIPE, // |
    LX_TOK_REDL, // <
    LX_TOK_REDR, // >
    LX_TOK_INBG, // &
    /* SPECIAL CHARACTERS */
    LX_DBQT, // Double quote
    LX_WTSP, // White space
};

struct lx_tok {
    enum lx_code kind;
    char *value;
};

size_t lx_tokenize(const char *cmd, struct lx_tok **list);

#endif
