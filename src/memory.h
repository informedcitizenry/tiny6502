/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef memory_h
#define memory_h

#include <ctype.h>
#include <stdlib.h>

typedef void(*element_dtor)(void*);

typedef struct dynamic_array
{
    size_t count;
    size_t capacity;
    size_t elem_size;
    void **data;
    element_dtor dtor;
    int initialize;
} dynamic_array;

void dynamic_array_add(dynamic_array *arr, void *element);
void dynamic_array_insert(dynamic_array *arr, void *element, size_t index);
void dynamic_array_insert_range(dynamic_array *dest, const dynamic_array *src, size_t index);

void dynamic_array_destroy(dynamic_array * arr);
void dynamic_array_cleanup_and_destroy(dynamic_array *arr);

#ifdef CHECK_LEAKS
void tiny_memory_report(void);
#endif
void *tiny_malloc(size_t size);
void *tiny_calloc(size_t count, size_t size);
void *tiny_realloc(void *ptr, size_t size);
#define tiny_free(p) free(p)

#define DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(arr, type, cap)\
arr = tiny_calloc(1, sizeof(struct dynamic_array));\
arr->elem_size = sizeof(type);\
arr->capacity = cap; arr->elem_size = sizeof(type); arr->data = tiny_calloc(cap, arr->elem_size);

#define DYNAMIC_ARRAY_CREATE(arr, type) DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(arr, type, 16)

#endif /* memory_h */
