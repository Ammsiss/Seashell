#include <stdlib.h>
#include <string.h>

#include "dyn_str.h"

int d_str_init(d_str *str) {
    assert(str);

    if (!str)
        return -1;

    *str = (d_str){0};

    str->c_str = malloc(1);
    if (!str->c_str)
        return -1;
    str->c_str[0] = '\0';

    str->len = 0;
    str->size = 1;
    str->cap = 1;

    return 0;
}

void d_str_free(d_str *str) {
    assert(str);

    if (!str)
        return;

    free(str->c_str);
    *str = (d_str){0};
}

int d_str_reserve(d_str *str, size_t min) {
    assert(str);
    if (!str)
        return -1;

    if (min == 0 || min <= str->cap)
        return 0;

    void *tmp = realloc(str->c_str, min * sizeof(*str->c_str));
    if (!tmp)
        return -1;

    str->c_str = tmp;
    str->cap = min;

    return 0;
}

int d_str_push(d_str *str, char c) {
    assert(str);

    if (!str)
        return -1;

    if (d_str_reserve(str, str->size + 1) == -1)
        return -1;

    ++str->size;
    ++str->len;

    str->c_str[str->len - 1] = c;
    str->c_str[str->size - 1] = '\0';

    return 0;
}

int d_strcat(d_str *dst, char *src) {
    size_t src_len = strlen(src);

    if (d_str_reserve(dst, dst->size + src_len) == -1)
        return -1;
    dst->size += src_len;

    strncat(&dst->c_str[dst->len], src, src_len);
    dst->len += src_len;

    return 0;
}

int d_strcpy(d_str *dst, char *src) {
    size_t src_len = strlen(src);

    if (d_str_reserve(dst, dst->size + src_len) == -1)
        return -1;
    dst->size = src_len + 1;

    strncpy(dst->c_str, src, dst->size);
    dst->len = src_len;

    return 0;
}
