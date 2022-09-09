/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef evaluator_h
#define evaluator_h

#include "value.h"

typedef struct assembly_context assembly_context;
typedef struct expression expression;

value evaluate_char_literal(const char *string, const char **str_ptr);
value evaluate_expression(assembly_context *context, const expression *expression);
value evaluate_token_value(const token *token);

#endif /* evaluator_h */
