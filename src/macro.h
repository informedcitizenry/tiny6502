/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef macro_h
#define macro_h

#include <ctype.h>

/*

Macro grammar:
macro :: IDENT '.macro' '(' macro_arg_list? ')' NEWLINE macro_block '.endmacro'

macro_arg_list :: IDENT (',' IDENT)*
**/

typedef struct dynamic_array dynamic_array;
typedef struct string_htable string_htable;
typedef struct lexer lexer;
typedef struct token token;

typedef struct macro
{
    string_htable *arg_names;
    dynamic_array *block_tokens;
    dynamic_array *sources;
    token *define_token;

} macro;

dynamic_array *macro_expand_macro(const token *pre_expand_label, const token *expand_token, dynamic_array *params, macro *macro, lexer *lex);
macro *macro_create(string_htable *arg_names, dynamic_array *block_tokens);

void macro_destroy(macro *macro);

#endif /* macro_h */
