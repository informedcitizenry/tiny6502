/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "builtin_symbols.h"
#include "memory.h"
#include "symbol_table.h"
#include "string_htable.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct symbol_table
{
    string_htable *table;
} symbol_table;

symbol_table *symbol_table_create(int case_sensitive)
{
    symbol_table *table = tiny_malloc(sizeof(symbol_table));
    table->table = string_htable_create(sizeof(value));
    table->table->case_sensitive = case_sensitive;
    return table;
}

size_t symbol_table_entry_count(symbol_table *table)
{
    return table->table->count;
}

int symbol_exists(symbol_table *table, char *name)
{
    return string_htable_contains(table->table, name) ||
            (BUILTIN_SYMBOL_TABLE && string_htable_contains(BUILTIN_SYMBOL_TABLE, name));
}

int symbol_table_define(symbol_table *table, char *name, value value)
{
    if (symbol_exists(table, name)) {
        return 0;
    }
    string_htable_add(table->table, name, (const htable_value_ptr)&value);
    return 1;
}

value symbol_table_lookup(symbol_table *table, char *name)
{
    htable_entry *entry = string_htable_find_bucket(table->table, name);
    if (entry) {
        return (value)*entry->value;
    }
    if (BUILTIN_SYMBOL_TABLE) {
        entry = string_htable_find_bucket(BUILTIN_SYMBOL_TABLE, name);
        if (entry) {
            return (value)*entry->value;
        }
    }
    return VALUE_UNDEFINED;
}

#define REPORT_LINE_LEN 80

#define HEADER(str) written = snprintf(p, REPORT_LINE_LEN - 1, str); p += written

char *symbol_table_report(symbol_table *table, char **buffer_ptr)
{
    string_htable *htable = table->table;
    size_t headers = 5;
    size_t cap = (htable->count + headers) * REPORT_LINE_LEN;
    char *buffer = tiny_malloc(sizeof(char) * cap), *p = buffer;
    size_t written;
    HEADER(";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n");
    HEADER(";;                                                                         ;;\n");
    HEADER(";; SYMBOL                         VALUE                                    ;;\n");
    HEADER(";;                                                                         ;;\n");
    HEADER(";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n");
    for(size_t i = 0; i < htable->capacity; i++) {
        htable_entry *bucket = htable->buckets + i;
        if (bucket->used) {
            written = snprintf(p, REPORT_LINE_LEN-1, "%-32s= $%x ;(%d)\n", bucket->original_key, *bucket->value, *bucket->value);
            p += written;
        }
    }
    if (buffer_ptr) *buffer_ptr = buffer;
    return buffer;
}

void symbol_table_update(symbol_table *table, char *name, value val)
{
    string_htable_update(table->table, name, (const htable_value_ptr)&val);
}

void symbol_table_destroy(symbol_table *table)
{
    string_htable_destroy(table->table);
    tiny_free(table);
}
