#ifndef DYN_ARR_H
#define DYN_ARR_H

#include <stdlib.h>

#define DA_GET(arr, index, type) \
    ((type *) ((arr)->data) + (index))

struct dyn_arr {
    void *data;
    size_t data_size;
    size_t size;
    size_t cap;
};

int da_init(struct dyn_arr *arr, size_t size, size_t data_size);
void da_free(struct dyn_arr *arr);
void *da_push(struct dyn_arr *arr);

#endif
