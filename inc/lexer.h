#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/* TOKENS */
enum lx_code {
    /* Single ops */
    LX_TOK_PIPE,    // |
    LX_TOK_BG,      // &
    LX_TOK_RDR_IN,  // <
    LX_TOK_RDR_OUT, // >
    LX_TOK_LPAREN,  // (
    LX_TOK_RPAREN,  // )
    LX_TOK_SEMI,    // ;
    LX_TOK_EOF,
    /* Double ops */
    LX_TOK_HDOC,       // <<
    LX_TOK_APPEND,     // >>
    LX_TOK_AND_IF,     // &&
    LX_TOK_OR_IF,      // ||
    LX_TOK_RDR_STDOUT, // 1>
    LX_TOK_RDR_STDERR, // 2>

    LX_TOK_WORD,
};

struct lx_tok {
    enum lx_code kind;
    char *value;
};

int lx_tokenize(const char *cmd, struct lx_tok **list, size_t *out_len);
struct lx_tok *lx_push_tok(struct lx_tok **list, size_t *size, size_t *cap);
int lx_add_tok(struct lx_tok **list, size_t *size, size_t *cap,
        enum lx_code kind, const char *start, const char *end);

#endif
