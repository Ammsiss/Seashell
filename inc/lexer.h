#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

enum lx_code {
    /* TOKENS */
    LX_TOK_WORD,
    /* Operators */
    LX_TOK_PIPE, // |
    LX_TOK_RDR_IN, // <
    LX_TOK_RDR_OUT, // >
    LX_TOK_BG, // &
    /* SPECIAL CHARACTERS */
    LX_DBQT, // Double quote
    LX_WTSP, // White space
};

struct lx_tok {
    enum lx_code kind;
    char *value;
};

int lx_tokenize(const char *cmd, struct lx_tok **list, size_t *out_len);
struct lx_tok *lx_push_tok(struct lx_tok **list, size_t *size, size_t *cap);
int lx_add_tok(struct lx_tok **list, size_t *size, size_t *cap,
        enum lx_code kind, const char *start, size_t len);

#endif
