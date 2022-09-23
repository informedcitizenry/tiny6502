/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "error.h"
#include "memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void grow_array(dynamic_array *arr)
{
    size_t current_cap = arr->capacity;
    arr->capacity *= 2; arr->data = tiny_realloc(arr->data, arr->capacity * arr->elem_size);
    if (arr->initialize) {
        memset(arr->data + current_cap, 0, current_cap);
    }
}

void dynamic_array_add(dynamic_array *arr, void *element)
{
    if (arr->count++ >= arr->capacity / 2){
        grow_array(arr);
    }
    arr->data[arr->count - 1] = element;
}

void dynamic_array_insert(dynamic_array *arr, void *element, size_t index)
{
    size_t original_count = arr->count;
    if (arr->count++ >= arr->capacity / 2) {
        grow_array(arr);
    }
    size_t trail_size = original_count - index;
    memmove(arr->data + index + 1, arr->data + index, trail_size * sizeof(void*));
    arr->data[index] = element;
}

void dynamic_array_destroy(dynamic_array * arr)
{
    if(arr){
        tiny_free(arr->data);
        tiny_free(arr);
    }
}

void dynamic_array_cleanup_and_destroy(dynamic_array *arr)
{
    if(arr){
        for(size_t i = 0; i < arr->count; i++) {
            if (arr->dtor && arr->data[i]) {
                arr->dtor(arr->data[i]);
            }
            else {
                tiny_free(arr->data[i]);
            }
        }
        tiny_free(arr->data);tiny_free(arr);
    }
}

void dynamic_array_insert_range(dynamic_array *dest, const dynamic_array *src, size_t index)
{
    size_t original_count = dest->count;
    dest->count += src->count;
    if (dest->count >= dest->capacity / 2) {
        grow_array(dest);
    }
    assert(index <= original_count);
    size_t trail_size = original_count - index;
    if (trail_size) {
        memmove(dest->data + src->count + index, dest->data + index, trail_size * sizeof(void*));
    }
    memcpy(dest->data + index, src->data, src->count * sizeof(void*));
}

#ifdef CHECK_LEAKS

void tiny_memory_report()
{
    printf("Remeber to do 'export MallocStackLogging=1' before executing process\n");
    system("leaks tiny6502");
}

#endif

static void buffer_check(const void *ptr)
{
     if (!ptr) {
        fputs(ERROR_TEXT "Fatal error: " DEFAULT_TEXT "Could not allocate a requested buffer. Sorry.", stderr);
        exit(1);
    }
}

void *tiny_malloc(size_t size)
{
    void *allocated = malloc(size);
    buffer_check(allocated);
    return allocated;
}

void *tiny_calloc(size_t count, size_t size)
{
    void *allocated = calloc(count, size);
    buffer_check(allocated);
    return allocated;
}

void *tiny_realloc(void *ptr, size_t size)
{
    if (ptr) {
        void *allocated = realloc(ptr, size);
        buffer_check(allocated);
        return allocated;
    }
    return tiny_malloc(size);
}

//#endif /* CHECK_LEAKS */
