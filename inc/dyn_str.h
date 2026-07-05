#ifndef DYN_STR_H
#define DYN_STR_H

#include <assert.h>
#include <stddef.h>

typedef struct {
    char *c_str;
    size_t len;
    size_t size;
    size_t cap;
} d_str;

int d_str_init(d_str *str);
void d_str_free(d_str *str);
int d_str_reserve(d_str *str, size_t min);
int d_str_push(d_str *str, char c);

int d_strcpy(d_str *str, char *c);
int d_strcat(d_str *dst, char *src);

#endif
