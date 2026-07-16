#ifndef DYN_ARR_H
#define DYN_ARR_H

#include <stdlib.h>
#include <sys/types.h>

#include "lexer_types.h"
#include "parser_types.h"
#include "shell_types.h"

#define DYN_ARR_TYPES(APPLY, arg) \
    /* lexer */ \
    APPLY(arg, da_part, lx_part) \
    APPLY(arg, da_tok, lx_tok) \
    /* parser */ \
    APPLY(arg, da_segment, ps_segment) \
    APPLY(arg, da_word, ps_word) \
    APPLY(arg, da_redir, ps_redir) \
    APPLY(arg, da_cmd, ps_cmd) \
    APPLY(arg, da_andor, ps_andor) \
    /* misc */ \
    APPLY(arg, da_int, int) \
    APPLY(arg, da_pid, pid_t) \
    APPLY(arg, da_vars, var_pair) \
    APPLY(arg, da_charp, char *) \

#define DECLARE_DYN_ARR(name, type) \
    typedef struct { \
        type *data; \
        size_t size; \
        size_t cap; \
    } name; \
    int name##_init(name *arr);\
    void name##_free(name *arr); \
    type *name##_push(name *arr); \
    int name##_reserve(name *arr, size_t min); \
    int name##_delete(name *arr, size_t remove_i);

#define DA_DECLARE(_, name, type) DECLARE_DYN_ARR(name, type)
    DYN_ARR_TYPES(DA_DECLARE, _)
#undef DA_DECLARE

#define DA_GENERIC_CASE(arg, name, type) , name: name##arg

#define DA_GET(suffix, arr) \
    _Generic(*(arr) DYN_ARR_TYPES(DA_GENERIC_CASE, suffix))

#define da_init(arr) \
    DA_GET(_init, (arr))(arr)

#define da_free(arr) \
    DA_GET(_free, (arr))(arr)

#define da_push(arr) \
    DA_GET(_push, (arr))(arr)

#define da_reserve(arr, size) \
    DA_GET(_reserve, (arr))(arr, size)

#define da_delete(arr, index) \
    DA_GET(_delete, (arr))(arr, index)

/* Push an element and then initialize it */
#define da_push_init(arr, init) \
    ({ \
        void *_p = da_push(arr); \
        if (_p && (init)(_p) == -1) \
            _p = NULL; \
        _p; \
    })

#endif
