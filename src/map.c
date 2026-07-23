#include <assert.h>
#include <string.h>

#include "map.h"

int mp_init(map_t *map, size_t key_size) {
    *map = (map_t){0};

    if (da_init(&map->pairs) == -1)
        return -1;

    map->key_size = key_size;

    return 0;
}

void mp_free(map_t *map) {
    for (size_t i = 0; i < map->pairs.size; ++i) {
        free(map->pairs.data[i].key);
        free(map->pairs.data[i].value);
    }

    da_free(&map->pairs);

    *map = (map_t){0};
}

static void *lookup_index(map_t *map, void *key, size_t *index) {
    for (size_t i = 0; i < map->pairs.size; ++i) {

        if (memcmp(key, map->pairs.data[i].key, map->key_size) == 0) {
            if (index)
                *index = i;
            return map->pairs.data[i].value;
        }
    }

    return NULL;
}

static char *str_canon_key(map_t *map, char *key) {
    assert(map && key && strlen(key) < map->key_size);

    char *canon_key = calloc(map->key_size, 1);
    if (!canon_key)
        return NULL;

    strcpy(canon_key, key);

    return canon_key;
}

int mp_str_lookup(map_t *map, char *key, void **value) {
    assert(map && key && value);

    char *canon_key = str_canon_key(map, key);
    if (!canon_key)
        return -1;

    void *map_value = lookup_index(map, canon_key, NULL);
    if (!map_value) {
        free(canon_key);
        *value = NULL;
        return 0;
    }

    *value = map_value;
    free(canon_key);

    return 0;
}

int mp_str_add(map_t *map, char *key, void *value) {
    char *canon_key = str_canon_key(map, key);
    if (!canon_key)
        goto fail;

    mpair *pair;
    size_t index;

    if (!lookup_index(map, canon_key, &index)) {
        pair = da_push(&map->pairs);
        if (!pair)
            goto fail;

        pair->key = canon_key;
        pair->value = value;

    } else {
        free(map->pairs.data[index].value);
        map->pairs.data[index].value = value;
        free(canon_key);
    }

    return 0;

fail:
    free(canon_key);
    return -1;
}

int mp_str_delete(map_t *map, char *key) {
    char *canon_key = str_canon_key(map, key);
    if (!canon_key)
        goto fail;

    size_t index;
    if (!lookup_index(map, canon_key, &index))
        goto fail;

    free(map->pairs.data[index].value);
    free(map->pairs.data[index].key);

    if (da_delete(&map->pairs, index) == -1)
        goto fail;

    free(canon_key);
    return 0;

fail:
    free(canon_key);
    return -1;
}
