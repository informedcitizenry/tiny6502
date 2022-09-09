/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef lexer_h
#define lexer_h

#include <ctype.h>

typedef struct lexer lexer;
typedef struct token token;
typedef struct source_file source_file;
typedef struct dynamic_array dynamic_array;

lexer *lexer_create(const source_file *source, int case_sensitive);
void lexer_destroy(lexer *lexer);

void lexer_include(lexer *lexer, const source_file *source);
dynamic_array *lexer_include_and_process(lexer *lexer, const source_file *included);
const source_file *lexer_get_source(lexer *lexer);

void lexer_add_reserved_word(lexer *lexer, const char *name);
void lexer_add_reserved_words(lexer *lexer, size_t word_count, const char** word_names, const int *word_values);

int lexer_is_reserved_word(const lexer *lexer, const char *word);

int lexer_is_case_sensitive(const lexer *lexer);
token *next_token(lexer *lexer);

#endif /* lexer_h */
