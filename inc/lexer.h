#ifndef LEXER_H
#define LEXER_H

#include "dyn_arr.h"
#include <stdint.h>

typedef enum {
    LX_TOK_PIPE,    // |
    LX_TOK_BG,      // &
    LX_TOK_RDR_IN,  // <
    LX_TOK_RDR_OUT, // >
    LX_TOK_RDR_ERR, // 2>
    LX_TOK_APPEND,  // >>
    LX_TOK_AND_IF,  // &&
    LX_TOK_OR_IF,   // ||

    LX_TOK_UNKNOWN,
    LX_TOK_WORD,
} lx_kind;

typedef enum {
    LX_M_NORMAL,
    LX_M_DOUBLEQ,
    LX_M_SINGLEQ,
} lx_mode;

typedef enum {
    LX_Q_SINGLE,
    LX_Q_DOUBLE,
    LX_Q_NONE,
} lx_quote;

typedef struct {
    char *raw;
    lx_quote quote;
} lx_part;
DEFINE_DYN_ARR(da_part, lx_part)

typedef struct {
    lx_kind kind;
    da_part parts;
} lx_tok;
DEFINE_DYN_ARR(da_tok, lx_tok)

typedef struct {
    lx_mode mode;
    lx_mode prev_mode;
    lx_tok *cur_tok;
    const char *part_start;
    const char *cur_char;
} lx_scanner;

void lx_free(da_tok *tokens);
int lx_tokenize(const char *cmd, da_tok *tokens);

#endif
