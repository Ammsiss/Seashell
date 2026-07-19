#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include "dyn_arr.h"
#include "variable.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "lexer.h" // IWYU pragma: keep
#include "parser.h" // IWYU pragma: keep
#include "shell_state.h" // IWYU pragma: keep
#include "runner.h" // IWYU pragma: keep

#define DEFINE_DYN_ARR(name, type) \
    int name##_init(name *arr) { \
        assert(arr); \
        \
        if (!arr) \
            return -1; \
        \
        *arr = (name){0}; \
        \
        return 0; \
    } \
    \
    void name##_free(name *arr) { \
        assert(arr); \
        \
        if (!arr) \
            return; \
        \
        free(arr->data); \
        *arr = (name){0}; \
    } \
    \
    int name##_reserve(name *arr, size_t min) { \
        assert(arr); \
        assert(arr->size <= arr->cap); \
        \
        if (!arr || arr->size > arr->cap) \
            return -1; \
        \
        if (min == 0 || min <= arr->cap) \
            return 0; \
        \
        void *tmp = realloc(arr->data, min * sizeof(*arr->data)); \
        if (!tmp) \
            return -1; \
        \
        arr->data = tmp; \
        arr->cap = min; \
        \
        return 0; \
    } \
    \
    type *name##_push(name *arr) { \
        assert(arr); \
        \
        if (!arr) \
            return NULL; \
        \
        if (name##_reserve(arr, arr->size + 1) == -1) \
            return NULL; \
        \
        ++arr->size; \
        \
        type *p = &arr->data[arr->size - 1]; \
        *p = (type){0}; \
        \
        return p; \
    } \
    \
    int name##_delete(name *arr, size_t remove_i) { \
        assert(arr); \
        if (!arr) \
            return -1; \
        \
        if (remove_i >= arr->size || remove_i < 0) \
            return -1; \
        \
        for (size_t i = remove_i + 1; i < arr->size; ++i) \
            arr->data[i - 1] = arr->data[i]; \
        \
        arr->size -= 1; \
        \
        return 0; \
    }

#define DA_DEFINE(_, name, type) DEFINE_DYN_ARR(name, type)
    DYN_ARR_TYPES(DA_DEFINE, _)
#undef X
