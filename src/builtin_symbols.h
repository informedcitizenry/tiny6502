/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/
#ifndef builtin_symbols_h
#define builtin_symbols_h

typedef struct string_htable string_htable;

extern string_htable *BUILTIN_SYMBOL_TABLE;

void builtin_init(int case_sensitive);
void builtin_cleanup(void);

#endif /* builtin_symbols_h */
