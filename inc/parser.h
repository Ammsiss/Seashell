#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#define PS_STDIN  0
#define PS_STDOUT 1
#define PS_STDERR 2
#define PS_APPEND 3
#define PS_RDR_ARR_LEN 4

/*

echo "hello" > file.txt

job {
    pipeline {
        commnads [
            command {
                da_lx_part argv[]
                da_lx_part redirects[]
            }
        ]
    }
}

*/

// typedef struct {
//     char redirects[PS_RDR_ARR_LEN];
// } ps_cmd;
//
// typedef struct {
//     size_t cmd_cnt;
// } ps_pipeline;
//
// typedef struct {
//     int bg;
//     dyn_arr cmds;
// } ps_job;
//
// ps_job *ps_parse(dyn_arr *tok_list);

#endif
