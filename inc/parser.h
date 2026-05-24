#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "lexer.h"

typedef struct {
    int argc;
    char **argv;
    char redirects[4];   /* stdin, stdout, stderr, append */
} ps_cmd;

typedef struct {
    size_t cmd_cnt;
} ps_pipeline;

typedef struct {
    /* ps_pipeline *pipelines; */
    /* ps_command *commands; */
    int background;
} ps_job;

ps_job *ps_parse(lx_tok *tok_list, size_t tok_list_size);

#endif
