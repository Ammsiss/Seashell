#ifndef EXECUTOR_TYPES_H
#define EXECUTOR_TYPES_H

typedef struct sh_builtin sh_builtin;
typedef struct sh_env sh_env;
typedef int (*builtin_func)(char **, sh_env *);

#endif
