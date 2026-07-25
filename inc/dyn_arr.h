#ifndef DYN_ARR_H
#define DYN_ARR_H

#include <stdlib.h>
#include <sys/types.h>

/* Declare any struct types to avoid circular
 * includes. See 2026-07-16 Notes */

/* lexer.h */
struct lx_part;
struct lx_tok;

/* parser.h */
struct ps_segment;
struct ps_word;
struct ps_redir;
struct ps_cmd;
struct ps_andor;
struct ps_ast;

/* map */
struct mpair;

#define DYN_ARR_TYPES(APPLY, arg) \
    /* lexer */ \
    APPLY(arg, da_part, struct lx_part) \
    APPLY(arg, da_tok, struct lx_tok) \
    /* parser */ \
    APPLY(arg, da_segment, struct ps_segment) \
    APPLY(arg, da_word, struct ps_word) \
    APPLY(arg, da_redir, struct ps_redir) \
    APPLY(arg, da_cmd, struct ps_cmd) \
    APPLY(arg, da_andor, struct ps_andor) \
    /* misc */ \
    APPLY(arg, da_int, int) \
    APPLY(arg, da_pid, pid_t) \
    APPLY(arg, da_charp, char *) \
    APPLY(arg, da_mpair, struct mpair)

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
