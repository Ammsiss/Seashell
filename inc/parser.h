#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "lexer.h"
#include "dyn_arr.h"

typedef enum {
    ERR_OP_POS,
} err_codes;

typedef enum {
    PS_AND_IF,
    PS_OR_IF,
    PS_NO_IF,
} ps_andor_op;

typedef enum {
    PS_Q_SINGLE,
    PS_Q_DOUBLE,
    PS_Q_NONE,
} ps_quote;

typedef struct {
    char *raw;
    ps_quote quote;
} ps_segment;
DEFINE_DYN_ARR(da_segment, ps_segment)

typedef struct {
    da_segment segments;
} ps_word;
DEFINE_DYN_ARR(da_word, ps_word)

typedef struct {
    ps_word target;
    int io_num;
    int append;
} ps_redir;
DEFINE_DYN_ARR(da_redir, ps_redir)

typedef struct {
    da_word words;
    da_redir redirs;
} ps_cmd;
DEFINE_DYN_ARR(da_cmd, ps_cmd)

typedef struct {
    da_cmd cmds;
} ps_pipeline;

typedef struct {
    ps_pipeline pipeline;
    ps_andor_op op;
} ps_andor;
DEFINE_DYN_ARR(da_andor, ps_andor)

typedef struct {
    da_andor andors;
    int bg;
} ps_job;

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
