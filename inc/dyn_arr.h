#ifndef DYN_ARR_H
#define DYN_ARR_H

#include <stdlib.h>

#define DEFINE_DYN_ARR(name, type) \
    typedef struct { \
        type *data; \
        size_t size; \
        size_t cap; \
    } name; \
    \
    static inline int name##_init(name *arr, size_t init_cap) { \
        if (!arr) \
            return -1; \
        \
        if (init_cap == 0) \
            init_cap = 1; \
        \
        arr->data = NULL; \
        arr->cap = 0; \
        arr->size = 0; \
        \
        arr->data = malloc(init_cap * sizeof(*arr->data)); \
        if (!arr->data) \
            return -1; \
        \
        arr->cap = init_cap; \
        arr->size = 0; \
        \
        return 0; \
    } \
    \
    static inline void name##_free(name *arr) { \
        if (!arr) \
            return; \
        \
        free(arr->data); \
        *arr = (name){0}; \
    } \
    \
    static inline type *name##_push(name *arr) { \
        if (!arr) \
            return NULL; \
        \
        if (arr->size >= arr->cap) { \
            if (arr->cap > SIZE_MAX / 2) \
                return NULL; \
            \
            size_t new_cap = arr->cap * 2; \
            \
            void *tmp = realloc(arr->data, new_cap * sizeof(*arr->data)); \
            if (!tmp) \
                return NULL; \
            \
            arr->data = tmp; \
            arr->cap = new_cap; \
        } \
        \
        return &arr->data[arr->size++]; \
    } \
    \
    static inline type *name##_end(name *arr) { \
        return &arr->data[arr->size - 1]; \
    }

#endif
