#ifndef LEXER_TYPES_H
#define LEXER_TYPES_H

typedef enum {
    LX_TOK_PIPE,
    LX_TOK_BG,
    LX_TOK_RDR_IN,
    LX_TOK_RDR_OUT,
    LX_TOK_RDR_ERR,
    LX_TOK_APPEND,
    LX_TOK_AND_IF,
    LX_TOK_OR_IF,
    LX_TOK_UNKNOWN,
    LX_TOK_WORD,
} lx_kind;

typedef enum {
    LX_Q_SINGLE,
    LX_Q_DOUBLE,
    LX_Q_NONE,
} lx_quote;

typedef struct lx_part lx_part;
typedef struct lx_tok lx_tok;

#endif

