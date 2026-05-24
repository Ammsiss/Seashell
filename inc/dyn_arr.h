#ifndef DYN_ARR_H
#define DYN_ARR_H

#include <stdlib.h>

#define DA_GET(arr, index, type) \
    ((type *) ((arr)->data) + (index))

typedef struct {
    void *data;
    size_t data_size;
    size_t size;
    size_t cap;
} dyn_arr;

int da_init(dyn_arr *arr, size_t size, size_t data_size);
void da_free(dyn_arr *arr);
void *da_push(dyn_arr *arr);

#endif
