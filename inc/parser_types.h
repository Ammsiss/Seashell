#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

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

typedef struct ps_segment ps_segment;
typedef struct ps_word ps_word;
typedef struct ps_redir ps_redir;
typedef struct ps_cmd ps_cmd;
typedef struct ps_pline ps_pline;
typedef struct ps_andor ps_andor;
typedef struct ps_job ps_job;

#endif
