#ifndef EXECUTOR_TYPES_H
#define EXECUTOR_TYPES_H

typedef enum {
    SH_ERRSYS,
    SH_ERRREG
} sh_errcode;

typedef struct sh_result sh_result;
typedef struct sh_builtin sh_builtin;
typedef struct sh_builtin_data sh_builtin_data;
typedef void (*builtin_func)(char **, sh_builtin_data *);

#endif
