#include "dyn_arr.h"

int da_init(dyn_arr *arr, size_t size, size_t data_size) {
    if (data_size < 1 || size < 0)
        return -1;

    arr->cap = (size == 0) ? 1 : size;

    if (!(arr->data = malloc(arr->cap * data_size)))
        return -1;

    arr->size = size;
    arr->data_size = data_size;

    return 0;
}

void da_free(dyn_arr *arr) {
    free(arr->data);
}

void *da_push(dyn_arr *arr) {
    if (arr->size >= arr->cap) {
        size_t cap = arr->cap * 2;

        void *tmp = realloc(arr->data, cap * arr->data_size);
        if (!tmp)
            return NULL;

        arr->data = tmp;
        arr->cap = cap;
    }

    return DA_GET(arr, arr->size++ * arr->data_size, char);
}
