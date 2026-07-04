#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "lexer_types.h"
#include "dyn_arr.h"

typedef enum {
    LX_OK,
    LX_ERRMEM,
    LX_ERRINPUT,
    LX_ERRNOENDQUOTE,
    LX_ERREMPTYESC
} lx_status;

struct lx_errinfo {
    int line;
    char *msg;
};

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

void lx_free(da_tok *tokens);
int lx_tokenize(const char *cmd, da_tok *tokens);

#endif
