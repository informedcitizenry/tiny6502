/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "memory.h"
#include "string_htable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPACITY 2039

static size_t get_hash(const char *str)
{
    size_t hash = 1099511628211UL;
    char c;
    while ((c = *str++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static char normalized[255];

string_htable *string_htable_create(size_t value_size)
{
    string_htable *table = tiny_malloc(sizeof(string_htable));
    table->value_size = value_size;
    table->count = 0;
    table->dtor = NULL;
    table->original_capacity = CAPACITY;
    table->capacity = CAPACITY;
    table->buckets = tiny_calloc(CAPACITY, sizeof(htable_entry));
    return table;
}

void string_htable_destroy(string_htable *table)
{
    if (!table) return;
    for(size_t i = 0; i < table->capacity; i++) {
        htable_entry *entry = table->buckets + i;
        if (!entry->used) continue;
        tiny_free(entry->key);
        tiny_free(entry->original_key);
        if (table->dtor) {
            table->dtor(entry->value);
        } else {
            tiny_free(entry->value);
        }
    }
    tiny_free(table->buckets);
    tiny_free(table);
}

static char* to_upper(const char *str)
{
    const char *c = str;
    char *n = normalized;
    while (*c) {
        if (*c >= 'a') {
            *n++ = *c++ & 0xdf;
        } else {
            *n++ = *c++;
        }
    }
    *n = *c;
    return normalized;
}

htable_entry *string_htable_find_bucket(string_htable *table, const char *key)
{
    if (!table->case_sensitive) {
        to_upper(key);
    } else {
        strncpy(normalized, key, 200);
    }
    size_t hash = get_hash(normalized);
    size_t ix = hash % table->original_capacity;
    htable_entry *entry = &table->buckets[ix];
    while (entry->used) {
        if (entry->hash == hash && strcmp(entry->key, normalized) == 0) {
            return entry;
        }
        if (++ix == table->capacity) {
            ix = 0;
        }
        entry = &table->buckets[ix];
    }
    return NULL;
}

int string_htable_contains(string_htable *table, const char *key)
{
    return string_htable_find_bucket(table, key) != NULL;
}

int string_htable_add(string_htable *table, const char *key, const htable_value_ptr value)
{
    if (++table->count > table->capacity / 2) {
        size_t current = table->capacity;
        table->capacity *= 2;
        table->buckets = tiny_realloc(table->buckets, table->capacity*sizeof(htable_entry));
        for(size_t i = current; i < table->capacity; i++) {
            table->buckets[i].used = 0;
        }
    }
    if (string_htable_contains(table, key)) {
        return 0;
    }
    if (!table->case_sensitive) {
        to_upper(key);
    }
    size_t hash = get_hash(normalized);
    size_t ix = hash % table->original_capacity;

    while (table->buckets[ix].used) {
        if (++ix == table->capacity) {
            ix = 0;
        }
    }
    htable_entry *bucket = table->buckets + ix;
    bucket->used = 1;
    bucket->hash = hash;

    size_t key_len = strlen(key);
    bucket->key = tiny_calloc(key_len + 1, sizeof(char));
    size_t original_key_len = key_len < 32 ? key_len + 1 : 32;
    
    bucket->original_key = tiny_calloc(original_key_len, sizeof(char));
    strncpy(bucket->key, normalized, key_len);
    strncat(bucket->original_key, key, original_key_len);
    
    bucket->value = tiny_malloc(table->value_size);
    memcpy(bucket->value, value, table->value_size);
    
    return 1;
}

const int *string_htable_get(string_htable *table, const char *key)
{
    htable_entry *bucket = string_htable_find_bucket(table, key);
    if (bucket) {
        return bucket->value;
    }
    return NULL;
}

int string_htable_update(string_htable *table, const char *key, const htable_value_ptr value)
{
    htable_entry *bucket = string_htable_find_bucket(table, key);
    if (bucket) {
        memcpy(bucket->value, value, table->value_size);
        return 1;
    }
    return 0;
}

string_htable *string_htable_create_from_lists(const char **keys, const htable_value_ptr values, size_t value_size, size_t count, int case_sensitive)
{
    const char *values_as_chars = (const char*)values;
    string_htable *table = string_htable_create(value_size);
    table->case_sensitive = case_sensitive;
    for(size_t i = 0; i < count; i++) {
        if (!string_htable_add(table, keys[i], (const htable_value_ptr)values_as_chars)) {
            string_htable_destroy(table);
            return NULL;
        }
        values_as_chars += value_size;
    }
    return table;
}

void string_htable_add_range(string_htable *table, size_t count, const char **keys, const htable_value_ptr values)
{
    const char *values_as_chars = (const char*)values;
    for(size_t i = 0; i < count; i++) {
        if (!string_htable_add(table, keys[i], (const htable_value_ptr)values_as_chars)) {
            fprintf(stderr, "Duplicate key '%s' added to string hash table", keys[i]);
            exit(1);
        }
        values_as_chars += table->value_size;
    }
}
