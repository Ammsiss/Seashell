#include <stdlib.h>
#include <string.h>

// #include "parser.h"
#include "lexer.h"

/*
WHEN CREATING A COMMAND:
 - Error on more then 1 word after redirect
 - Error on no word after redirect
*/

// ps_job *ps_parse(dyn_arr *tok_list) {
//     if (tok_list->size < 1)
//         return NULL;
//
//     for (size_t i = 0; i < tok_list->size; ++i) {
//         lx_tok *tok = DA_GET(tok_list, i, lx_tok);
//
//         switch (tok->kind) {
//         case LX_TOK_RDR_IN:
//         case LX_TOK_RDR_OUT:
//         case LX_TOK_RDR_STDOUT:
//         case LX_TOK_RDR_STDERR:
//         case LX_TOK_APPEND:
//
//         case LX_TOK_PIPE:
//         case LX_TOK_AND_IF:
//         case LX_TOK_OR_IF:
//
//         case LX_TOK_WORD:
//
//         default:
//             break;
//         }
//     }
//
//     return NULL;
// }
