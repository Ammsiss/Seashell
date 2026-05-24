#include <stdlib.h>

#include "parser.h"
#include "lexer.h"

ps_job *ps_parse(lx_tok *tok_list, size_t tok_list_size) {
    if (!tok_list || tok_list_size < 1)
        return NULL;

    // ps_job *cmd = malloc(sizeof(ps_cmd));

    /* <, [>, 1>], >>, 2> */
    for (size_t i = 0; i < tok_list_size; ++i) {
        switch (tok_list[i].kind) {
        case LX_TOK_RDR_IN:
            /* Next token must be a word (filepath) */
            if (i < tok_list_size - 1) {
                if (tok_list[i].kind == LX_TOK_WORD)
                    ;
                else
                    goto fail;
            }
        case LX_TOK_RDR_OUT:
        case LX_TOK_RDR_STDOUT:

        case LX_TOK_APPEND:

        case LX_TOK_RDR_STDERR:
        default:
            break;
        }
    }

fail:
    return NULL;
}
