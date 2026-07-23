#define _GNU_SOURCE

#include "variable.h"
#include "log.h"
#include "map.h"
#include "utils.h"

char *var_errstr(var_err err) {
    switch (err) {
    case VAR_ERR_KEY: return "invalid key";
    default:          return "???";
    }
}

static bool verify_key(const void *key) {
    assert(key);

    if (strlen(key) >= VAR_KEY_SIZE)
        return false;

    for (const char *c = key; *c != '\0'; ++c) {
        bool valid_key =
                ((*c >= 'a' && *c <= 'z') ||
                (*c >= 'A' && *c <= 'Z') ||
                (*c >= '0' && *c <= '9') ||
                *c == '-' || *c == '_');

        if (!valid_key)
            return false;
    }

    return true;
}

int add_var(map_t *vars, char *key, char *value) {
    if (!verify_key(key))
        return VAR_ERR_KEY;

    char *new_val = malloc(strlen(value) + 1);
    if (!new_val)
        xfatal("malloc");

    strcpy(new_val, value);

    if (mp_str_add(vars, key, new_val) == -1)
        xfatal("mp_str_add");

    return 0;
}

char *lookup_var(map_t *vars, char *key) {
    void *value;

    if (mp_str_lookup(vars, key, &value) == -1)
        xfatal("mp_str_lookup");

    return value;
}

int delete_var(map_t *vars, char *key) {
    if (!verify_key(key))
        return VAR_ERR_KEY;

    if (mp_str_delete(vars, key) == -1)
        xfatal("mp_str_add");

    return 0;
}
