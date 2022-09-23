/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "expression.h"
#include "operand.h"
#include "memory.h"
#include <stdlib.h>

static operand *operand_create(int form)
{
    operand *operand = tiny_calloc(1, sizeof(struct operand));
    operand->form = form;
    return operand;
}

static void expression_dtor(void *expr_ptr)
{
    expression_destroy((expression*)expr_ptr);
}

static void pseudo_op_arg_dtor(void *ptr)
{
    pseudo_op_arg *arg = (pseudo_op_arg*)ptr;
    if (arg->arg_type == PSEUDO_OP_EXPRESSION) {
        expression_destroy(arg->arg.expression);
    }
    tiny_free(arg);
}

operand *operand_single_expression(int form, expression *expr, expression *bitwidth)
{
    operand *operand = operand_create(form);
    operand->single_expression.expr = expr;
    operand->single_expression.bitwidth = bitwidth;
    return operand;
}

operand *operand_two_expressions(expression *expr0, expression *expr1)
{
    operand *operand = operand_create(FORM_TWO_OPERANDS);
    
    operand->two_expression.expr0 = expr0;
    operand->two_expression.expr1 = expr1;
    return operand;
}

operand *operand_bit(expression *bit, expression *expr)
{
    operand *operand = operand_create(FORM_BIT_ZP);
    operand->bit_expression.bit = bit;
    operand->bit_expression.expr = expr;
    return operand;
}

operand *operand_bit_offset(expression *bit, expression *offs, expression *expr)
{
    operand *operand = operand_create(FORM_BIT_OFFS_ZP);
    operand->bit_offset_expression.bit = bit;
    operand->bit_offset_expression.offs = offs;
    operand->bit_offset_expression.expr = expr;
    return operand;
}

operand *operand_expression_list(expression_array *expressions)
{
    operand *operand = operand_create(FORM_EXPRESSION_LIST);
    operand->expression_list.expressions = expressions;
    operand->expression_list.expressions->dtor = expression_dtor;
    return operand;
}

operand *operand_pseudo_op_args(pseudo_op_arg_array *args)
{
    operand *operand = operand_create(FORM_PSEUDO_OP_LIST);
    operand->pseudo_op_arg_args.args = args;
    operand->pseudo_op_arg_args.args->dtor = pseudo_op_arg_dtor;
    return operand;
}

void operand_destroy(operand *operand)
{
    if (!operand) {
        return;
    }
    switch (operand->form) {
        case FORM_EXPRESSION_LIST:
            dynamic_array_cleanup_and_destroy(operand->expression_list.expressions);
            break;
        case FORM_BIT_ZP:
            expression_destroy(operand->bit_expression.bit);
            expression_destroy(operand->bit_expression.expr);
            break;
        case FORM_BIT_OFFS_ZP:
            expression_destroy(operand->bit_offset_expression.bit);
            expression_destroy(operand->bit_offset_expression.offs);
            expression_destroy(operand->bit_offset_expression.expr);
            break;
        case FORM_TWO_OPERANDS:
            expression_destroy(operand->two_expression.expr0);
            expression_destroy(operand->two_expression.expr1);
            break;
        case FORM_PSEUDO_OP_LIST:
            dynamic_array_cleanup_and_destroy(operand->pseudo_op_arg_args.args);
            break;
        default:
            expression_destroy(operand->single_expression.bitwidth);
            expression_destroy(operand->single_expression.expr);
    }
    tiny_free(operand);
 }
