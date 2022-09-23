/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "assembly_context.h"
#include "anonymous_label.h"
#include "expression.h"
#include "error.h"
#include "evaluator.h"
#include "memory.h"
#include "output.h"
#include "token.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct value_stack
{
    value stack[2];   
    size_t stack_pointer;

} value_stack;

static void stack_init(value_stack *stack)
{
    stack->stack[0] = VALUE_UNDEFINED;
    stack->stack[1] = VALUE_UNDEFINED;
    stack->stack_pointer = 1;
}

static value stack_peek(const value_stack *stack)
{
    return stack->stack[1];
}

static value stack_pop(value_stack *stack)
{
    value v = stack_peek(stack);
    if (stack->stack_pointer < 1) {
        stack->stack_pointer++;
    }
    return v;
}

static void stack_push(value_stack *stack, value v)
{
    if (stack->stack_pointer) {
        stack->stack[stack->stack_pointer--] = v;
    }
}

static void stack_evaluate_expression(assembly_context*, const expression *, value_stack *stack);

static char *get_lhs_scope(const expression *expr, char *root_expr)
{
    size_t root_len = root_expr ? strlen(root_expr) : 0;
    size_t scope_len = root_len + TOKEN_TEXT_MAX_LEN + 2;
    char *scoped_name = tiny_calloc(scope_len + 1, sizeof(char));
    TOKEN_GET_TEXT(expr->binary.lhs->token, lhs_text);
    if (root_expr) {
        snprintf(scoped_name, scope_len + 1, "%s.%s", root_expr, lhs_text);
        tiny_free(root_expr);
    }
    else {
        strncpy(scoped_name, lhs_text, TOKEN_TEXT_MAX_LEN);
    }
    if (expr->binary.rhs->type == TOKEN_BINARY) {
        return get_lhs_scope(expr->binary.rhs, scoped_name);
    }
    return scoped_name;
}

static void eval_scoped_identifier(assembly_context *context, const expression *expr, value_stack *stack)
{
    if (!context) {
        stack_push(stack, VALUE_UNDEFINED);
        return;
    }
    char *root = get_lhs_scope(expr, NULL);
    TOKEN_GET_TEXT(expr->binary.rhs->token, target);
    size_t scope_size = strlen(root) + TOKEN_TEXT_MAX_LEN + 2;
    char *scoped_name = tiny_calloc(scope_size, sizeof(char));
    snprintf(scoped_name, scope_size, "%s.%s", root, target);
    value v = VALUE_UNDEFINED;
    if (!symbol_exists(context->sym_tab, scoped_name)) {
        if (!context->pass_needed) {
            if (!context->passes) {
                context->pass_needed = 1;
            }
            else {
                tiny_error(expr->binary.lhs->token, ERROR_MODE_RECOVER, "Unresolved symbol '%s'", scoped_name);
            }
        }
    }
    else {
        v = symbol_table_lookup(context->sym_tab, scoped_name);
    }
    stack_push(stack, v);
    tiny_free(root);
    tiny_free(scoped_name);
}

static void eval_ternary(assembly_context *context, const expression *expression, value_stack *stack)
{
    stack_evaluate_expression(context, expression->ternary.cond, stack);
    value cond = stack_peek(stack);
    if (cond != VALUE_UNDEFINED) {
        stack_pop(stack);
        stack_evaluate_expression(context, expression->ternary.then, stack);
        value then = stack_pop(stack);
        stack_evaluate_expression(context, expression->ternary.else_, stack);
        value else_ = stack_pop(stack);
        if (cond) {
            stack_push(stack, then);
        } else {
            stack_push(stack, else_);
        }
    }
}

static void eval_binary(assembly_context *context, const expression *expression, value_stack *stack)
{
    token_type oper = expression->token->type;
    if (oper == TOKEN_DOT && expression->binary.rhs->token->type == TOKEN_IDENT) {
        eval_scoped_identifier(context, expression, stack);
        return;
    }
    const token *lhs_token = expression->binary.lhs->token;
    if (oper == TOKEN_EQUAL && context) {
        if (lhs_token->type != TOKEN_IDENT) {
            tiny_error(lhs_token, ERROR_MODE_RECOVER, "Invalid lvalue in assignment");
        }
        stack_evaluate_expression(context, expression->binary.rhs, stack);
        if (stack_peek(stack) != VALUE_UNDEFINED) {
            TOKEN_GET_TEXT(lhs_token, token_text);
            if (!symbol_exists(context->sym_tab, token_text)) {
                symbol_table_define(context->sym_tab, token_text, stack_peek(stack));
            } else {
                tiny_error(lhs_token, ERROR_MODE_RECOVER, "Symbol '%s' previously defined");
            }
        }
        return;
    }
    stack_evaluate_expression(context, expression->binary.lhs, stack);
    if (stack_peek(stack) != VALUE_UNDEFINED) {
        value lhs = stack_pop(stack);
        if ((oper == TOKEN_DOUBLEAMPERSAND || oper == TOKEN_DOUBLEPIPE) &&
            (lhs < 0 || lhs > 1))
        {
            tiny_warn(expression->token, "Consider using the '%c' operator instead", 
            expression->token->src.ref[expression->token->src.start]);
        }
        if ((oper == TOKEN_DOUBLEAMPERSAND && !lhs) || 
            (oper == TOKEN_DOUBLEPIPE && lhs))
        {
            stack_push(stack, lhs);
            return;
        }
        stack_evaluate_expression(context, expression->binary.rhs, stack);
        if (stack_peek(stack) == VALUE_UNDEFINED) {
            return;
        }
        value rhs = stack_pop(stack);
        if (rhs == 0 && oper == TOKEN_SOLIDUS) {
            if (!context || !context->passes) {
                tiny_error(expression_get_lhs_token(expression->binary.rhs), ERROR_MODE_RECOVER, "Divide by zero error");
            }
            stack_push(stack, VALUE_UNDEFINED);
            return;
        }
        if (oper == TOKEN_DOUBLECARET || oper == TOKEN_LSHIFT) {
            /* treat operations that can end up with potentially very large results separate */
            double magniResult;
            if (oper == TOKEN_DOUBLECARET) {
                magniResult = pow(lhs, rhs);
            }
            else {
                magniResult = lhs * pow(2, rhs);
            }
            /* check if "magnitude" operations result in overflow */
            int overflow = magniResult < INT64_MIN || magniResult > INT64_MAX || magniResult == INFINITY || magniResult == NAN;
            if (overflow && (!context || !context->pass_needed)) {
                tiny_error(expression_get_lhs_token(expression->binary.lhs), ERROR_MODE_RECOVER, "Arithmetic overflow");
                stack_push(stack, VALUE_UNDEFINED);
            }
            else {
                stack_push(stack, (value)magniResult);
            }
            return;
        }
        if (oper == TOKEN_ARSHIFT) {
            int lhsSign = lhs >= 0 ? 1 : -1;
            stack_push(stack, (lhs >> rhs) * lhsSign);
            return;
        }
        value result = VALUE_UNDEFINED;
        switch (oper) {
            case TOKEN_ASTERISK:        result = lhs * rhs; break;
            case TOKEN_SOLIDUS:         result = lhs / rhs; break;
            case TOKEN_PERCENT:         result = lhs % rhs; break;
            case TOKEN_PLUS:            result = lhs + rhs; break;
            case TOKEN_HYPHEN:          result = lhs - rhs; break;
            case TOKEN_RSHIFT:          result = lhs >> rhs; break;
            case TOKEN_LANGLE:          result = lhs < rhs; break;
            case TOKEN_LTE:             result = lhs <= rhs; break;
            case TOKEN_GTE:             result = lhs >= rhs; break;
            case TOKEN_RANGLE:          result = lhs > rhs; break;
            case TOKEN_SPACESHIP:       result = lhs > rhs ? 1 : lhs == rhs ? 0 : -1; break;
            case TOKEN_DOUBLEEQUAL:     result = lhs == rhs; break;
            case TOKEN_BANGEQUAL:       result = lhs != rhs; break;
            case TOKEN_AMPERSAND:       result = lhs & rhs; break;
            case TOKEN_CARET:           result = lhs ^ rhs; break;
            case TOKEN_PIPE:            result = lhs | rhs; break;
            case TOKEN_DOUBLEAMPERSAND: result = lhs && rhs; break;
            case TOKEN_DOUBLEPIPE:      result = lhs || rhs; break;
            default: result = rhs;
        }
        stack_push(stack, result);
    }
}

static void eval_unary(assembly_context *context, const expression *expression, value_stack *stack)
{
    stack_evaluate_expression(context, expression->unary.expr, stack);
    if (stack_peek(stack) != VALUE_UNDEFINED) {
        value val = stack_pop(stack);
        switch (expression->token->type) {
            case TOKEN_HYPHEN:      val = -val; break;
            case TOKEN_BANG:        val = !val; break;
            case TOKEN_TILDE:       val = ~val; break;
            case TOKEN_LANGLE:      val &= 0xff; break;
            case TOKEN_RANGLE:      val = (val >> 8) & 0xff;
            case TOKEN_AMPERSAND:   val &= 0xffff; break;
            default:                val = (val >> 16) & 0xff;
        }
        stack_push(stack, val);
    }
}

static void lookup_ident(assembly_context *context, const expression *expression, value_stack *stack)
{
    if (!context) {
        stack_push(stack, VALUE_UNDEFINED);
        return;
    }
    const token *token = expression->token;
    if (token->type == TOKEN_ASTERISK) {
        stack_push(stack, context->output->logical_pc);
        return;
    }
    TOKEN_GET_TEXT(token, name);
    if (name[0] == '+' || name[0] == '-') {
        if (name[0] == '+' && !context->passes) {
            context->pass_needed = 1;
            stack_push(stack, VALUE_UNDEFINED);
            return;
        }
        value v = anonymous_label_get_by_name(context->anonymous_labels_new, name);
        if (v == VALUE_UNDEFINED) {
            tiny_error(token, ERROR_MODE_RECOVER, "Unresolved anonymous label");
        }
        stack_push(stack, v);
        return;
    }
    if (symbol_exists(context->sym_tab, name)) {
        stack_push(stack, symbol_table_lookup(context->sym_tab, name));
        return;
    }
    if (context->local_label && name[0] == '_') {
        char scoped_name[TOKEN_TEXT_MAX_LEN*2+1] = {};
        TOKEN_GET_TEXT(context->local_label, local_label);
        snprintf(scoped_name, TOKEN_TEXT_MAX_LEN*2, "%s.%s", local_label, name);
        if (symbol_exists(context->sym_tab, scoped_name)) {
            stack_push(stack, symbol_table_lookup(context->sym_tab, scoped_name));
            return;
        }
    }
    if (!context->passes) {
        context->pass_needed = 1;
        stack_push(stack, VALUE_UNDEFINED);
        return;
    }
    tiny_error(token, ERROR_MODE_RECOVER, "Symbol '%s' not defined", name);
    stack_push(stack, VALUE_UNDEFINED);
}

static void stack_evaluate_token_value(const token *token, value_stack *stack)
{
    if (token->src.end - token->src.start > 65) {
        tiny_error(token, ERROR_MODE_RECOVER, "Illegal quantity");
        stack_push(stack, VALUE_UNDEFINED);
    }
    if (token->type == TOKEN_CHARLITERAL) {
        const char *end_ptr;
        value chr = evaluate_char_literal(token->src.ref + token->src.start + 1, &end_ptr);
        if (*end_ptr != '\'' || chr > UINT8_MAX) {
            if (chr > UINT8_MAX) {
                tiny_error(token, ERROR_MODE_RECOVER, "Escape sequence out of range");
            } else {
                tiny_error(token, ERROR_MODE_RECOVER, "Invalid char literal");
            }
            chr = VALUE_UNDEFINED;
        }
        stack_push(stack, chr);
        return;
    }
    char num_buffer[66] = {};
    char *n = num_buffer;
    const char *tk = token->src.ref + token->src.start;
    const char *tk_end = token->src.ref + token->src.end;
    if (token->type == TOKEN_HEXLITERAL || token->type == TOKEN_BINLITERAL) {
        ++tk;
    }
    /* strip digit separators '_' */
    while (tk != tk_end) {
        if (*tk != '_') {
            *n++ = *tk;
        }
        tk++;
    }
    value v = VALUE_UNDEFINED;
    switch (token->type) {
        case TOKEN_HEXLITERAL:
            v = strtoll(num_buffer, NULL, 16);
            break;
        case TOKEN_DECLITERAL:
            v = strtoll(num_buffer, NULL, 10);
            break;
        case TOKEN_BINLITERAL:
            v = strtoll(num_buffer, NULL, 2);
            break;
        default:
            tiny_error(token, ERROR_MODE_RECOVER, "Expected integer literal");
            v = VALUE_UNDEFINED;
    }
    if (v != VALUE_UNDEFINED && (v < INT32_MIN || v > UINT32_MAX)) {
        tiny_error(token, ERROR_MODE_RECOVER, "Illegal quantity");
        v = VALUE_UNDEFINED;
    }
    stack_push(stack, v);
}

static void stack_evaluate_expression(assembly_context *context, const expression *expression, value_stack *stack)
{
    if (expression->value != VALUE_UNDEFINED) {
        stack_push(stack, expression->value);
        return;
    }
    switch (expression->type) {
        case TYPE_IDENT:
            lookup_ident(context, expression, stack);
            break;
        case TYPE_LITERAL:
            stack_evaluate_token_value(expression->token, stack);
            break;
        case TYPE_UNARY:
            eval_unary(context, expression, stack);
            break;
        case TYPE_BINARY:
            eval_binary(context, expression, stack);
            break;
        case TYPE_FCN_CALL: {
                TOKEN_GET_TEXT(expression->token, symbol_name);
                if (symbol_exists(context->sym_tab, symbol_name)) {
                    tiny_error(expression->token, ERROR_MODE_RECOVER, "Symbol is not a function");
                } else {
                    tiny_error(expression->token, ERROR_MODE_RECOVER, "Symbol '%s' not defined", symbol_name);
                }
            }
            break;
        default:
            eval_ternary(context, expression, stack);
    }
}

value evaluate_char_literal(const char* string, const char **str_ptr)
{
    value c = *string++;
    if (c != '\\') {
        if (str_ptr) *str_ptr = string;
        return c;
    } 
    switch (*string) {
        case '\\': string++; break;
        case '\'': string++; break;
        case '"': string++; break;
        case 'b': c = '\b'; string++; break;
        case 'f': c = '\f'; string++; break;
        case 'n': c = '\n'; string++; break;
        case 'r': c = '\r'; string++; break;
        case 't': c = '\t'; string++; break;
        case 'v': c = '\v'; string++; break;
        default: {
            if (isdigit(*string)) {
                c = strtol(string, (char**)str_ptr, 8);
            } else {
                c = strtol(++string, (char**)str_ptr, 16);
            }
            return c;
        }
    }
    *str_ptr = string;
    return c;
}

value evaluate_token_value(const token *token)
{
    value_stack stack;
    stack_init(&stack);
    stack_evaluate_token_value(token, &stack);
    return stack_pop(&stack);
}

value evaluate_expression(assembly_context *context, const expression *expression)
{
    value_stack stack;
    stack_init(&stack);
    stack_evaluate_expression(context, expression, &stack);
    return stack_pop(&stack);
}

