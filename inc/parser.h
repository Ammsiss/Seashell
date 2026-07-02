#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "parser_types.h"
#include "dyn_arr.h"

struct ps_segment {
    char *raw;
    ps_quote quote;
};

struct ps_word {
    da_segment segments;
    char *arg;
};

struct ps_redir {
    ps_word target;
    int io_num;
    int append;
};

struct ps_cmd {
    da_word words;
    da_redir redirs;
    char **argv;
};

struct ps_pipeline {
    da_cmd cmds;
};

struct ps_andor {
    ps_pipeline pipeline;
    ps_andor_op op;
};

struct ps_job {
    da_andor andors;
    int bg;
};

typedef struct {
    ps_pipeline *cur_pipeline;
    ps_cmd *cur_cmd;
    lx_tok *cur_tok;
    ps_redir *queued_redir;
    ps_andor_op cur_andor_op;
} ps_scanner;

void ps_free(ps_job *job);
int ps_parse(da_tok *tokens, ps_job *job);

#endif
