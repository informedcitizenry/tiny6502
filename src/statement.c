/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "memory.h"
#include "operand.h"
#include "statement.h"
#include "token.h"
#include <stdlib.h>
#include <string.h>

statement *statement_create(token *label, token *instruction) 
{
    statement *stat = tiny_malloc(sizeof(statement));
    stat->operand = NULL;
    stat->label = label;
    stat->instruction = instruction;
    return stat;
}

void statement_get_source_line_from_token(const token *token, char *dest)
{
    strncpy(dest, token->src.ref, LINE_DISPLAY_LEN);
}

void statement_destroy(statement *statement)
{
    operand_destroy(statement->operand);
    tiny_free(statement);
}

