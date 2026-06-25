#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "lexer_types.h"
#include "dyn_arr.h"

typedef enum {
    LX_M_NORMAL,
    LX_M_DOUBLEQ,
    LX_M_SINGLEQ,
} lx_mode;

struct lx_part {
    char *raw;
    lx_quote quote;
};

struct lx_tok {
    lx_kind kind;
    da_part parts;
};

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
