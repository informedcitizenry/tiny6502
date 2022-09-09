/**
 * tiny6502
 *
 * Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
 * 
 * Licensed under the MIT license. See LICENSE for full license information.
 *
 */

#ifndef string_htable_h
#define string_htable_h

#include <ctype.h>

typedef struct htable_entry
{
    
    char *original_key;
    char *key;
    int *value;
    int used;
    size_t hash;
    
} htable_entry;

typedef int* htable_value_ptr;

typedef void(*htable_value_dtor)(htable_value_ptr);

typedef struct string_htable
{
    
    size_t count;
    size_t original_capacity;
    size_t capacity;
    size_t value_size;
    int case_sensitive;
    htable_entry *buckets;
    htable_value_dtor dtor;
    
} string_htable;

string_htable *string_htable_create(size_t value_size);
string_htable *string_htable_create_from_lists(const char **keys, const htable_value_ptr values, size_t value_size, size_t count, int case_sensitive);

void string_htable_destroy(string_htable *table);

int string_htable_contains(string_htable *table, const char *key);
int string_htable_add(string_htable *table, const char *key, const htable_value_ptr value);
void string_htable_add_range(string_htable *table, size_t count, const char **keys, const htable_value_ptr values);
htable_entry *string_htable_find_bucket(string_htable *table, const char *key);
const int* string_htable_get(string_htable *table, const char *key);
int string_htable_update(string_htable *table, const char *key, const htable_value_ptr value);

#endif /* string_htable_h */
