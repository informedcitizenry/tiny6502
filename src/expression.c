/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "memory.h"
#include "expression.h"
#include "evaluator.h"
#include "token.h"

#include <stdio.h>

static void destroy_params(expression_array *params)
{
    for(size_t i = 0; i < params->count; i++) {
        expression_destroy((expression*)params->data[i]);
    }
    dynamic_array_destroy(params);
}

void expression_destroy(expression *expression)
{
    if (!expression) {
        return;
    }
    switch (expression->type) {
        case TYPE_UNARY:
            expression_destroy(expression->unary.expr);
            break;
        case TYPE_BINARY:
            expression_destroy(expression->binary.lhs);
            expression_destroy(expression->binary.rhs);
            break;
        case TYPE_TERNARY:
            expression_destroy(expression->ternary.cond);
            expression_destroy(expression->ternary.then);
            expression_destroy(expression->ternary.else_);
            break;
        case TYPE_FCN_CALL:
            destroy_params(expression->fcn_call.params);
        default:
            break;
    }
    tiny_free(expression);
}

static expression *create_with_type(const token *token, int type)
{
    expression *expr = tiny_malloc(sizeof(expression));
    expr->token = token;
    expr->type = type;
    expr->value = VALUE_UNDEFINED;
    return expr;
}

expression *expression_literal_ident(const token *token, int is_ident)
{
    expression *expr = create_with_type(token, is_ident ? TYPE_IDENT : TYPE_LITERAL);
    if (!is_ident && token->type != TOKEN_STRINGLITERAL) {
        expr->value = evaluate_token_value(token);
    }
    return expr;
}

expression *expression_unary(const token *oper, expression *expr)
{
    expression *unary = create_with_type(oper, TYPE_UNARY);
    unary->unary.expr = expr;
    if (expr->value != VALUE_UNDEFINED) {
        unary->value = evaluate_expression(NULL, unary);
    }
    return unary;
}

expression *expression_binary(const token *oper, expression *lhs, expression *rhs)
{
    expression *binary = create_with_type(oper, TYPE_BINARY);
    binary->binary.lhs = lhs;
    binary->binary.rhs = rhs;
    binary->value = evaluate_expression(NULL, binary);
    return binary;
}

expression *expression_ternary(const token *oper, expression *cond, expression *then, expression *else_)
{
    expression *ternary = create_with_type(oper, TYPE_TERNARY);
    ternary->ternary.cond = cond;
    ternary->ternary.then = then;
    ternary->ternary.else_ = else_;
    ternary->value = evaluate_expression(NULL, ternary);
    return ternary;
}

expression *expression_fcn_call(const token *ident, expression_array *params)
{
    expression *fcn_call = create_with_type(ident, TYPE_FCN_CALL);
    fcn_call->fcn_call.params = params;
    return fcn_call;
}

expression *expression_copy(const expression *expr)
{
    expression *copy = create_with_type(expr->token, expr->type);
    switch (copy->type) {
        case TYPE_UNARY:
            copy->unary.expr = expression_copy(expr->unary.expr);
            break;
        case TYPE_BINARY:
            copy->binary.lhs = expression_copy(expr->binary.lhs);
            copy->binary.rhs = expression_copy(expr->binary.rhs);
            break;
        case TYPE_TERNARY:
            copy->ternary.cond = expression_copy(expr->ternary.cond);
            copy->ternary.then = expression_copy(expr->ternary.then);
            copy->ternary.else_= expression_copy(expr->ternary.else_);
        default:
            break;
    }
    copy->value = expr->value;
    return copy;
}

const token *expression_get_lhs_token(const expression *expr)
{
    switch (expr->type) {
        case TYPE_IDENT:
        case TYPE_LITERAL:
        case TYPE_FCN_CALL:
            return expr->token;
        case TYPE_UNARY:
            return expression_get_lhs_token(expr->unary.expr);
        case TYPE_BINARY:
            return expression_get_lhs_token(expr->binary.lhs);
        default:
            return expression_get_lhs_token(expr->ternary.cond);
    }
}