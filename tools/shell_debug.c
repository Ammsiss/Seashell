#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

#define NORETURN __attribute__ ((__noreturn__))
#define PFFORMAT(x, y) __attribute__ ((format(printf, (x), (y))))

NORETURN PFFORMAT(1, 2) void err_exit(const char *fmt, ...)  {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

NORETURN PFFORMAT(1, 2) void usage_err(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "usage: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

void print_tok_list(const da_lx_tok *tokens) {

    printf("Token count = %ld\n\n", tokens->size);

    for (size_t i = 0; i < tokens->size; ++i) {
        lx_tok *tok = &tokens->data[i];

        if (tok->kind == LX_TOK_WORD) {
            printf("WORD {\n");

            for (size_t y = 0; y < tok->parts.size; ++y) {
                lx_part *part = &tok->parts.data[y];
                printf("    { raw = [%s] , quotes: ", part->raw);

                switch (part->quote) {
                case LX_Q_NONE:   printf("none");   break;
                case LX_Q_DOUBLE: printf("double"); break;
                case LX_Q_SINGLE: printf("single"); break;
                }

                printf(" }\n");
            }

            printf("}\n");
        } else {
            printf("TOKEN { ");

            switch (tok->kind) {
            case LX_TOK_PIPE:       printf("PIPE(|)");        break;
            case LX_TOK_BG:         printf("INBG(&)");        break;
            case LX_TOK_RDR_IN:     printf("RDR_IN(<)");      break;
            case LX_TOK_RDR_OUT:    printf("RDR_OUT(>)");     break;
            case LX_TOK_LPAREN:     printf("LPAREN(()");      break;
            case LX_TOK_RPAREN:     printf("RPAREN())");      break;
            case LX_TOK_SEMI:       printf("SEMI(;)");        break;
            case LX_TOK_EOF:        printf("EOF())");         break;
            case LX_TOK_HDOC:       printf("HDOC(<<)");       break;
            case LX_TOK_APPEND:     printf("APPEND(>>)");     break;
            case LX_TOK_AND_IF:     printf("AND_IF(&&)");     break;
            case LX_TOK_OR_IF:      printf("OR_IF(||)");      break;
            case LX_TOK_RDR_STDOUT: printf("RDR_STDOUT(1>)"); break;
            case LX_TOK_RDR_STDERR: printf("RDR_STDERR(2>)"); break;
            default:                                           break;
            }

            printf(" }\n");
        }
    }

}

int main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usage_err("%s [--stop-after STAGE] COMMAND\n", argv[0]);

    da_lx_tok list;
    if (lx_tokenize(argv[1], &list) == -1)
        err_exit("lx_tokenize failed\n");

    print_tok_list(&list);

    lx_free(&list);
}
