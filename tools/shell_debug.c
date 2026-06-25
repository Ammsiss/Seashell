#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

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

typedef enum {
    LEXER,
    PARSER,
} endpoint;

const char *get_quote(lx_quote quote) {
    switch (quote) {
    case LX_Q_NONE:   return "none";
    case LX_Q_DOUBLE: return "double";
    case LX_Q_SINGLE: return "single";
    default:          return NULL;
    }
}

const char *get_andor_op(ps_andor_op quote) {
    switch (quote) {
    case PS_AND_IF:   return "AND_IF";
    case PS_OR_IF:    return "OR_IF";
    case PS_NO_IF:    return "NO_IF";
    default:          return NULL;
    }
}

const char *get_kind(lx_kind kind) {
    switch (kind) {
    case LX_TOK_PIPE:    return "PIPE(|)";
    case LX_TOK_BG:      return "INBG(&)";
    case LX_TOK_RDR_IN:  return "RDR_IN(<)";
    case LX_TOK_RDR_OUT: return "RDR_OUT(>)";
    case LX_TOK_APPEND:  return "APPEND(>>)";
    case LX_TOK_AND_IF:  return "AND_IF(&&)";
    case LX_TOK_OR_IF:   return "OR_IF(||)";
    case LX_TOK_RDR_ERR: return "RDR_STDERR(2>)";
    case LX_TOK_WORD:    return "WORD";
    default:             return "???";
    }
}

void print_tok_list(const da_tok *tokens) {

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
            case LX_TOK_APPEND:     printf("APPEND(>>)");     break;
            case LX_TOK_AND_IF:     printf("AND_IF(&&)");     break;
            case LX_TOK_OR_IF:      printf("OR_IF(||)");      break;
            case LX_TOK_RDR_ERR: printf("RDR_STDERR(2>)"); break;
            default:                                           break;
            }

            printf(" }\n");
        }
    }

}

static int indent_level = 0;
static int hide_darrays = 0;

PFFORMAT(1, 2) void indented_print(const char *fmt, ...) {
    va_list ap;

    // if (indent_level > 0) {
        for (int i = 0; i < (indent_level/* - 1*/) * 2; ++i) {
            // if (i % 2 == 0)
            //     printf("%s", "│");
            // else
                printf("%c", ' ');
        }
    // }

    // switch (child) {
    // case FIRST:
    // case MIDDLE:
    //     printf("├─");
    //     break;
    // case LAST:
    //     printf("└─");
    //     break;
    // case NOT:
    //     printf("  ");
    // }

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void print_part(const lx_part *part) {
    indent_level++;
    indented_print("LX_PART\n");
    indented_print("  raw: %s\n", part->raw);
    indented_print("  quote: %s\n", get_quote(part->quote));
    indent_level--;
}

void print_parts(const da_part *parts) {
    if (parts->size == 0)
        return;

    for (size_t i = 0; i < parts->size; ++i) {
        const lx_part *part = &parts->data[i];

        switch (part->quote) {
        case LX_Q_DOUBLE:
            printf("\"%s\"", part->raw);
            break;
        case LX_Q_SINGLE:
            printf("\'%s\'", part->raw);
            break;
        case LX_Q_NONE:
            printf("%s", part->raw);
            break;
        }
    }
}

void print_tok(const lx_tok *tok) {
    if (!hide_darrays) {
        indent_level++;
        indented_print("WORD[");
    }

    print_parts(&tok->parts);
    printf("]\n");

    if (!hide_darrays) {
        indent_level--;
    }
}

void print_redir(const ps_redir *redir) {
    indent_level++;
    indented_print("REDIR(io_num=%d, append=%d)\n",
        redir->io_num, redir->append);
    print_tok(&redir->target);
    indent_level--;
}

void print_redirs(const da_redir *redirs) {
    if (redirs->size == 0)
        return;

    if (!hide_darrays) {
        indent_level++;
        indented_print("REDIR[%ld]\n", redirs->size);
    }

    for (size_t i = 0; i < redirs->size; ++i) {
        const ps_redir *redir = &redirs->data[i];
        print_redir(redir);
    }

    if (!hide_darrays)
        indent_level--;
}

void print_words(const da_tok *words) {
    if (words->size == 0)
        return;

    if (!hide_darrays) {
        indent_level++;
        indented_print("WORD[%ld]\n", words->size);
    }

    for (size_t i = 0; i < words->size; ++i) {
        const lx_tok *tok = &words->data[i];
        print_tok(tok);
    }

    if (!hide_darrays)
        indent_level--;
}

void print_cmd(const ps_cmd *cmd) {
    indent_level++;
    indented_print("CMD\n");
    print_words(&cmd->words);
    print_redirs(&cmd->redirs);
    indent_level--;
}

void print_pipeline(const da_cmd *pipeline) {
    if (pipeline->size == 0)
        return;

    if (!hide_darrays) {
        indent_level++;
        indented_print("PIPELINE[%ld]\n", pipeline->size);
    }

    for (size_t i = 0; i < pipeline->size; ++i) {
        const ps_cmd *cmd = &pipeline->data[i];
        print_cmd(cmd);
    }

    if (!hide_darrays) {
        indent_level--;
    }
}

void print_andor(const ps_andor *andor) {
    indent_level++;
    indented_print("%s\n", get_andor_op(andor->op));
    print_pipeline(&andor->pipeline);
    indent_level--;
}

void print_andors(const da_andor *rest) {
    if (rest->size == 0)
        return;

    if (!hide_darrays) {
        indent_level++;
        indented_print("ANDOR[%ld]\n", rest->size);
    }

    for (size_t i = 0; i < rest->size; ++i) {
        const ps_andor *andor = &rest->data[i];
        print_andor(andor);
    }

    if (!hide_darrays) {
        indent_level--;
    }
}

void print_job(const ps_job *job) {
    printf("JOB ");

    if (job->bg)
        printf("(background) [\n");
    else
        printf("[\n");

    print_pipeline(&job->first);
    print_andors(&job->rest);

    printf("]\n");
}

int main(int argc, char **argv)
{
    if (argc == 3 || argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usage_err("%s [--stop-after STAGE] COMMAND\n", argv[0]);

    const char *cmd;
    endpoint go_until;

    if (argc == 4) {
        cmd = argv[3];

        if (strcmp(argv[1], "--stop-after") == 0) {
            if (strcmp(argv[2], "lexer") == 0)
                go_until = LEXER;
            else if (strcmp(argv[2], "parser") == 0)
                go_until = PARSER;
            else
                usage_err("invalid argument: %s\n", argv[2]);
        } else {
            usage_err("invalid option: %s\n", argv[1]);
        }
    } else
        cmd = argv[1];

    da_tok list;
    if (lx_tokenize(cmd, &list) == -1)
        err_exit("lx_tokenize failed\n");

    if (go_until == LEXER) {
        print_tok_list(&list);
        lx_free(&list);
        return 0;
    }

    ps_job job;
    if (ps_parse(&list, &job) == -1)
        err_exit("%s: lx_parse failed\n", argv[0]);

    if (go_until == PARSER) {
        print_job(&job);
        lx_free(&list);
        ps_free_job(&job);
        return 0;
    }

}
