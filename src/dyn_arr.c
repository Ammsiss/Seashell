#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include "dyn_arr.h"
#include "lexer.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "parser.h" // IWYU pragma: keep - See 2026-06-25 Notes
#include "shell_state.h" // IWYU pragma: keep - See 2026-06-25 Notes

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

/* for tests */
DEFINE_DYN_ARR(da_int, int)

DEFINE_DYN_ARR(da_pid, pid_t)
DEFINE_DYN_ARR(da_vars, var_pair)
DEFINE_DYN_ARR(da_charp, char *)

/* lexer */
DEFINE_DYN_ARR(da_part, lx_part)
DEFINE_DYN_ARR(da_tok, lx_tok)

/* parser */
DEFINE_DYN_ARR(da_segment, ps_segment)
DEFINE_DYN_ARR(da_word, ps_word)
DEFINE_DYN_ARR(da_redir, ps_redir)
DEFINE_DYN_ARR(da_cmd, ps_cmd)
DEFINE_DYN_ARR(da_andor, ps_andor)
