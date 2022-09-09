/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef operand_h
#define operand_h

typedef struct expression expression;
typedef struct dynamic_array expression_array;
typedef struct operand
{
    enum {
        FORM_IMMEDIATE,
        FORM_INDIRECT_S,
        FORM_INDIRECT_X,
        FORM_INDIRECT_Y,
        FORM_INDIRECT,
        FORM_INDEX_S,
        FORM_INDEX_X,
        FORM_INDEX_Y,
        FORM_DIRECT,
        FORM_DIRECT_Y,
        FORM_ZP_ABSOLUTE,
        FORM_TWO_OPERANDS,
        FORM_ACCUMULATOR,
        FORM_BIT_ZP,
        FORM_BIT_OFFS_ZP,
        FORM_EXPRESSION_LIST
    } form;
    union {
        struct {
            expression *expr;
            expression *bitwidth;
        } single_expression;
        struct {
            expression *bit;
            expression *expr;
        } bit_expression;
        struct {
            expression *expr0;
            expression *expr1;
        } two_expression;
        struct {
            expression *bit;
            expression *offs;
            expression *expr;
        } bit_offset_expression;
        struct {
            expression_array *expressions;
        } expression_list;
    };
} operand;

operand *operand_single_expression(int form, expression *expr, expression *bitwidth);
operand *operand_two_expressions(expression *expr0, expression *expr1);
operand *operand_bit(expression *bit, expression *expr);
operand *operand_bit_offset(expression *bit, expression *offs, expression *expr);
operand *operand_expression_list(expression_array *expressions);

void operand_destroy(operand *operand);

#endif /* operand_h */
