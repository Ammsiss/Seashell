#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

#include "dyn_arr.h"

/* TOKENS */
typedef enum {
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
    LX_TOK_HDOC,       // <<, shell will set up pipe with hdoc contents
    LX_TOK_APPEND,     // >>
    LX_TOK_AND_IF,     // &&
    LX_TOK_OR_IF,      // ||
    LX_TOK_RDR_STDOUT, // 1>
    LX_TOK_RDR_STDERR, // 2>

    LX_TOK_WORD,
} lx_kind;

typedef struct {
    lx_kind kind;
    char *value;
} lx_tok;

void lx_free(dyn_arr *list);
int lx_add_tok(dyn_arr *list, lx_kind kind, const char *start,
    const char *end);
int lx_flush_word(dyn_arr *list, const char **tok_start, const char *tok_end);
int lx_tokenize(const char *cmd, dyn_arr *list);

#endif
