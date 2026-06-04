#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>

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

    LX_TOK_UNKNOWN,
    LX_TOK_WORD,
} lx_kind;

typedef enum {
    LX_M_NORMAL,
    LX_M_Q_DOUBLE,
    LX_M_BACKSLASH,
} lx_mode;

typedef enum {
    LX_Q_SINGLE,
    LX_Q_DOUBLE,
    LX_Q_NONE,
} lx_quote;

typedef struct {
    lx_mode mode;
    const char *tok_start;
    const char *tok_cur;
} lx_scanner;

typedef struct {
    lx_kind kind;
    char *value;
} lx_tok;

DEFINE_DYN_ARR(da_lx_tok, lx_tok)

/* New

typedef struct {
    char *raw;
    lx_quote quote;
} lx_part;

typedef struct {
    lx_kind kind;
    dyn_arr parts;
} lx_tok_better;

*/

int lx_add_tok(da_lx_tok *list, lx_kind kind, lx_scanner *scanner);
int lx_tokenize(const char *cmd, da_lx_tok *list);
void lx_free(da_lx_tok *list);

#endif
