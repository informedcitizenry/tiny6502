/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef expression_h
#define expression_h

#include "value.h"

typedef struct token token;
typedef struct dynamic_array expression_array;

typedef struct expression
{
    enum {
        TYPE_LITERAL,
        TYPE_IDENT,
        TYPE_UNARY,
        TYPE_BINARY,
        TYPE_TERNARY,
        TYPE_FCN_CALL
    } type;
    const token *token;
    value value;
    union {
        /* unary :: operator | expr */
        struct {
            struct expression *expr;
        } unary;
        
        /*binary :: expr operator expr */
        struct {
            struct expression *lhs;
            struct expression *rhs;
        } binary;
        
        /*ternary :: expr '?' expr ':' expr */
        struct {
            struct expression *cond;
            struct expression *then;
            struct expression *else_;
        } ternary;

        /*fcn_call :: IDENT '(' (expr (',' expr)*)? ')' */
        struct {
            expression_array *params;
        } fcn_call;

    };
} expression;

void expression_destroy(expression *expression);

expression *expression_literal_ident(const token *token, int is_ident);
expression *expression_unary(const token *oper, expression *expr);
expression *expression_binary(const token *oper, expression *lhs, expression *rhs);
expression *expression_copy(const expression *expr);
expression *expression_ternary(const token *oper, expression *cond, expression *then, expression *else_);
expression *expression_fcn_call(const token *ident, expression_array *params);

const token *expression_get_lhs_token(const expression *expr);
#endif /* expression_h */
