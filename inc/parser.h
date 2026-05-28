#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "dyn_arr.h"

#define PS_STDIN  0
#define PS_STDOUT 1
#define PS_STDERR 2
#define PS_APPEND 3
#define PS_RDR_ARR_LEN 4

typedef struct {
    char redirects[PS_RDR_ARR_LEN];
    dyn_arr words; // lx_tok
} ps_cmd;

typedef struct {
    size_t cmd_cnt;
} ps_pipeline;

typedef struct {
    int bg;
    dyn_arr cmds;
} ps_job;

ps_job *ps_parse(dyn_arr *tok_list);

#endif
