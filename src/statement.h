/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef statement_h
#define statement_h

#include <ctype.h>

typedef struct token token;
typedef struct operand operand;

typedef struct statement
{
    
    token *label;
    token *instruction;
    size_t index;
    operand *operand;
    
} statement;

#define LINE_DISPLAY_LEN    90

statement *statement_create(token *label, token *instruction);
void statement_destroy(statement *statement);

void statement_get_source_line_from_token(const token *token, char *dest);

#endif /* statement_h */
