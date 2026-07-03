#ifndef DYN_ARR_H
#define DYN_ARR_H

#include <stdlib.h>
#include <sys/types.h>

#include "lexer_types.h"
#include "parser_types.h"

#define DECLARE_DYN_ARR(name, type) \
    typedef struct { \
        type *data; \
        size_t size; \
        size_t cap; \
    } name; \
    int name##_init(name *arr);\
    void name##_free(name *arr); \
    type *name##_push(name *arr); \
    int name##_reserve(name *arr, size_t min);

/* for tests */
DECLARE_DYN_ARR(da_int, int)

/* for executor */
DECLARE_DYN_ARR(da_pid, pid_t)

/* lexer */
DECLARE_DYN_ARR(da_part, lx_part)
DECLARE_DYN_ARR(da_tok, lx_tok)

/* parser */
DECLARE_DYN_ARR(da_segment, ps_segment)
DECLARE_DYN_ARR(da_word, ps_word)
DECLARE_DYN_ARR(da_redir, ps_redir)
DECLARE_DYN_ARR(da_cmd, ps_cmd)
DECLARE_DYN_ARR(da_andor, ps_andor)

#define get_da_init(arr) \
    _Generic(*(arr), \
        da_int: da_int_init, \
        da_pid: da_pid_init, \
        da_part: da_part_init, \
        da_tok: da_tok_init, \
        da_segment: da_segment_init, \
        da_word: da_word_init, \
        da_redir: da_redir_init, \
        da_cmd: da_cmd_init, \
        da_andor: da_andor_init \
    )

#define da_init(arr) \
    get_da_init(arr)(arr)

#define get_da_free(arr) \
    _Generic(*(arr), \
        da_int: da_int_free, \
        da_pid: da_pid_free, \
        da_part: da_part_free, \
        da_tok: da_tok_free, \
        da_segment: da_segment_free, \
        da_word: da_word_free, \
        da_redir: da_redir_free, \
        da_cmd: da_cmd_free, \
        da_andor: da_andor_free \
    )

#define da_free(arr) \
    get_da_free(arr)(arr)

#define get_da_reserve(arr) \
    _Generic(*(arr), \
        da_int: da_int_reserve, \
        da_pid: da_pid_reserve, \
        da_part: da_part_reserve, \
        da_tok: da_tok_reserve, \
        da_segment: da_segment_reserve, \
        da_word: da_word_reserve, \
        da_redir: da_redir_reserve, \
        da_cmd: da_cmd_reserve, \
        da_andor: da_andor_reserve \
    )

#define da_reserve(arr, min) \
    get_da_reserve(arr)(arr, min)

#define get_da_push(arr) \
    _Generic(*(arr), \
        da_int: da_int_push, \
        da_pid: da_pid_push, \
        da_part: da_part_push, \
        da_tok: da_tok_push, \
        da_segment: da_segment_push, \
        da_word: da_word_push, \
        da_redir: da_redir_push, \
        da_cmd: da_cmd_push, \
        da_andor: da_andor_push \
    )

#define da_push(arr) \
    get_da_push(arr)(arr)

/* Push an element and then initialize it
 *
 * Returns the initizlied element on success or NULL
 * if either da_push or init fails.
 *
 * This is only safe to call when the caller does
 * not need to disambiguate push failure vs init
 * failure, such as in ps_parse where the entire
 * owning object is destroyed on any failure. See
 * 2026-06-25 Notes. */
#define da_push_init(arr, init) \
    ({ \
        void *_p = da_push(arr); \
        if (_p && (init)(_p) == -1) \
            _p = NULL; \
        _p; \
    })

#endif
