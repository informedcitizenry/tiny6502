/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef pseudo_op_h
#define pseudo_op_h

typedef struct assembly_context assembly_context;
typedef struct operand operand;
typedef struct token token;

void pseudo_op_gen(assembly_context *context, const token *directive_token, const operand *operand);

#endif /* pseudo_op_h */
