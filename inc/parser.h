#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

struct lx_tok;

struct ps_cmd {
    int argc;
    char **argv;
    char redirects[4];   /* stdin, stdout, stderr, append */
};

struct ps_pipeline {
    size_t cmd_cnt;
};

struct ps_job {
    /* struct ps_pipeline *pipelines; */
    /* struct ps_command *commands; */
    int background;
};

struct ps_job *ps_parse(struct lx_tok *tok_list, size_t tok_list_size);

#endif
