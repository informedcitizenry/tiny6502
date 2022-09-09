/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef symbol_table_h
#define symbol_table_h

#include "value.h"

typedef struct symbol_table symbol_table;

int symbol_table_define(symbol_table *table, char *name, value value);
void symbol_table_update(symbol_table *table, char *name, value value);
int symbol_exists(symbol_table *table, char *name);
value symbol_table_lookup(symbol_table *table, char *name);

symbol_table *symbol_table_create(int case_sensitive);
void symbol_table_destroy(symbol_table *table);

char *symbol_table_report(symbol_table *table, char **buffer_ptr);
size_t symbol_table_entry_count(symbol_table *table);

#endif /* symbol_table_h */
