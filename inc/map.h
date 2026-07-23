#ifndef MAP_H
#define MAP_H

#include "dyn_arr.h"

struct mpair {
    void *key;
    void *value;
};

typedef struct mpair mpair;

struct map_t {
    da_mpair pairs;
    size_t key_size;
};

typedef struct map_t map_t;

int mp_init(map_t *map, size_t key_size);
void mp_free(map_t *map);

int mp_str_add(map_t *map, char *key, void *value);
int mp_str_lookup(map_t *map, char *key, void **value);
int mp_str_delete(map_t *map, char *key);

#endif
