#ifndef VARIABLE_H
#define VARIABLE_H

#define VAR_KEY_SIZE 4096

#include "map.h"

typedef enum {
    VAR_OK,
    VAR_ERR_KEY,
} var_err;

char *var_errstr(var_err err);
char *lookup_var(map_t *vars, char *key);
int add_var(map_t *vars, char *key, char *value);
int delete_var(map_t *vars, char *key);

#endif
