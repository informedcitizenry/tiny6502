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

struct object 
{
    const void *address;
    int freed;
    char location[100];
};

#define MAX_TRACK   100000

static int at_exit_set = 0;

static struct object allocations[MAX_TRACK] = {};
static size_t allocation_count = 0;
static size_t allocations_freed = 0;

void tiny_free_diag(void *ptr, const char *file, int line)
{
    if (!ptr) {
        return;
    }
    int freed = 0;
    for(size_t i = 0; i < allocation_count; i++) {
        if (ptr == allocations[i].address) {
            if (!allocations[i].freed) {
                allocations_freed++;
                allocations[i].freed = 1;
                allocations[i].address = NULL;
                freed = 1;
            }
        }
    }
    if (!freed) {
        printf("Object %p freed allocated at %s:%d was never malloced\n",
            ptr, file, line);
            return;
    }
    free(ptr);
}

void tiny_memory_report()
{
    for(size_t i = 0; i < allocation_count; i++) {
        if (allocations[i].address) {
            printf("Object with address %p allocated at %s was never freed\n",
                allocations[i].address, allocations[i].location);
        }
    }
    size_t unfreed = allocation_count - allocations_freed;
    if (unfreed) {
        printf("%zu objects were never freed", unfreed);
    }
}

static void buffer_check(const void *ptr, const char *file, int line)
{
    if (!at_exit_set) {
        atexit(tiny_memory_report);
        at_exit_set = 1;
    }
    if (!ptr) {
        fputs(ERROR_TEXT "Fatal error: " DEFAULT_TEXT "Could not allocate a requested buffer. Sorry.", stderr);
        exit(1);
    }
    if (allocation_count < MAX_TRACK) {
        struct object obj;
        obj.address = ptr;
        obj.freed = 0;
        snprintf(obj.location, 100, "%s:%d", file, line);
        allocations[allocation_count++] = obj;
    }
}

void *tiny_malloc_diag(size_t size, const char *file, int line)
{
    void *allocated = malloc(size);
    buffer_check(allocated, file, line);
    return allocated;
}

void *tiny_calloc_diag(size_t count, size_t size, const char *file, int line)
{
    void *allocated = calloc(count, size);
    buffer_check(allocated, file, line);
    return allocated;
}

void *tiny_realloc_diag(void *ptr, size_t size, const char *file, int line)
{
    if (!ptr) return tiny_malloc_diag(size, file, line);
    void *allocated = realloc(ptr, size);
    if (ptr != allocated) {
        for(size_t i = 0; i < allocation_count; i++) {
            if (allocations[i].address == ptr) {
                if (!allocations[i].freed) {
                    allocations[i].freed = 1;
                    allocations[i].address = NULL;
                    allocations_freed++;
                }
            }
        }
    }       
    buffer_check(allocated, file, line);
    return allocated;
}

#else

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

#endif /* CHECK_LEAKS */
