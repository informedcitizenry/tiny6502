/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "assembly_context.h"
#include "error.h"
#include "expression.h"
#include "evaluator.h"
#include "memory.h"
#include "operand.h"
#include "output.h"
#include "pseudo_op.h"
#include "string_htable.h"
#include "symbol_table.h"
#include "token.h"
#include "value.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>

static void gen_values(assembly_context *context, const operand *operand, int size)
{
    expression **values = (expression**)operand->expression_list.expressions->data;
    size_t count = operand->expression_list.expressions->count;
    for(size_t i = 0; i < count; i++) {
        value v;
        if (values[i]->value != VALUE_UNDEFINED) {
            v = values[i]->value;
        } else {
            v = evaluate_expression(context, values[i]);
        }
        if (value_size(v) > size) {
            if (context->pass_needed || v == VALUE_UNDEFINED) {
                output_fill(context->output, size);
                continue;
            }
            tiny_error(values[i]->token, ERROR_MODE_RECOVER, "Illegal quantity %lld", v);
            return;
         }
         output_add(context->output, v, size);
    }
}

static size_t gen_string(assembly_context *context, const token *str_token, int no_high_bit)
{
    size_t len = 0;
    size_t str_start = str_token->src.start + 1; /* skip the quote */
    const char *string = str_token->src.ref + str_start;
    while (*string != '"') {
        long c = evaluate_char_literal(string, &string);
        len++;
        if (c <= UINT8_MAX) {
            if (no_high_bit && c > INT8_MAX) {
                tiny_error(str_token, ERROR_MODE_RECOVER, "One or more string bytes invalid for directive");
                return 0;
            }
            output_add(context->output, c, 1);
        } else {
            if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdbff)) {
                tiny_error(str_token, ERROR_MODE_RECOVER, "Illegal quantity (codepoint is not valid)");
                return 0;
            }
            if (c >= 0x800) {
                if (c >= 0x10000) {
                    len++;
                    output_add(context->output, (((c >> 18) & 0x07) | 0xF0), 1);
                    output_add(context->output, (((c >> 12) & 0x3F) | 0x80), 1);
                } else {
                    output_add(context->output, (((c >> 12) & 0x0F) | 0xE0), 1);
                }
                len++;
                output_add(context->output, (((c >> 6) & 0x3F) | 0x80), 1);
                output_add(context->output, (((c     ) & 0x3F) | 0x80), 1);
            } else {
                len++;
                output_add(context->output, (((c >> 6) & 0x1F) | 0xC0), 1);
            }
            len++;
            output_add(context->output, ((c & 0x3F) | 0x80), 1);
        }
    }
    return len;
}

static void gen_strings(assembly_context *context, token_type directive, const operand *operand)
{
    const expression **strings = (const expression**)operand->expression_list.expressions->data;
    size_t count = operand->expression_list.expressions->count;
    size_t output_bytes = 0;
    int no_high_bit = directive == TOKEN_LSTRING || directive == TOKEN_NSTRING;
    for(size_t i = 0; i < count; i++) {
        if (directive == TOKEN_PSTRING && i == 0) {
            output_add(context->output, 0, 1);
        }
        const token *str_token = strings[i]->token;
        if (strings[i]->type == TYPE_LITERAL &&
        (str_token->type == TOKEN_STRINGLITERAL)) {
            output_bytes += gen_string(context, str_token, no_high_bit);
            continue;
        }
        value v;
        if (strings[i]->value != VALUE_UNDEFINED) {
            v = strings[i]->value;
        } else {
            v = evaluate_expression(context, strings[i]);
        }
        if (v == VALUE_UNDEFINED) {
            output_fill(context->output, 1);
            continue;
        }
        if (no_high_bit && v > INT8_MAX) {
            tiny_error(str_token, ERROR_MODE_RECOVER, "One or more values invalid for directive");
            return;
        }
        size_t val_size = value_size(v);
        output_bytes += val_size;
        if (val_size > 4) {
            tiny_error(strings[i]->token, ERROR_MODE_RECOVER, "Illegal quantity");
            return;
        }
        output_add(context->output, v, (int)val_size);
    }
    switch (directive) {
        case TOKEN_CSTRING:
            output_add(context->output, 0, 1);
            break;
        case TOKEN_LSTRING:
            for(size_t i = context->start_pc; i < context->start_pc + output_bytes; i++) {
                if (directive == TOKEN_LSTRING) {
                    context->output->buffer[i] <<= 1;
                }
            }
            context->output->buffer[context->start_pc + output_bytes - 1] |= 1;
            break;
        case TOKEN_NSTRING:
            context->output->buffer[context->start_pc + output_bytes - 1] |= 0x80;
            break;
        case TOKEN_PSTRING:
            if (output_bytes > 255) {
                tiny_error(strings[0]->token, ERROR_MODE_RECOVER, "String length too long for \".pstring\"");
                return;
            }
            context->output->buffer[context->start_pc] = (char)output_bytes & 0xff;
            break;
        default: break; /* shut up the warning about enumeration values not handled */
    }
}

static void gen_binary_file(assembly_context *context, const operand *operand)
{
    expression **operands = (expression**)operand->expression_list.expressions->data;
    const token *file_token = operands[0]->token;
    if (operands[0]->type != TYPE_LITERAL || file_token->type != TOKEN_STRINGLITERAL) {
        tiny_error(file_token, ERROR_MODE_RECOVER, "Directive \".binary\" requires a file name");
        return;
    }
    value binary_file_count = -1;
    value binary_file_displ = 0;
    if (operand->expression_list.expressions->count > 1) {
        binary_file_displ = evaluate_expression(context, operands[1]);
        if (binary_file_displ == VALUE_UNDEFINED) return;
        if (binary_file_displ < 0 || binary_file_displ > UINT16_MAX) {
            tiny_error(operands[1]->token, ERROR_MODE_RECOVER, "Illegal quantity");
            return;
        }
        if (binary_file_displ + context->output->pc > UINT16_MAX) {
            tiny_error(operands[1]->token, ERROR_MODE_RECOVER, "File displacement outside of range");
            return;
        }
        if (operand->expression_list.expressions->count > 2) {
            if (operand->expression_list.expressions->count > 3) {
                tiny_error(operands[2]->token, ERROR_MODE_RECOVER, "Unexpected expression");
                return;
            }
            binary_file_count = evaluate_expression(context, operands[2]);
            if (binary_file_count == VALUE_UNDEFINED) return;
            if (binary_file_count < 0 || binary_file_count > UINT16_MAX) {
                tiny_error(operands[2]->token, ERROR_MODE_RECOVER, "Illegal quantity");
                return;
            }
            if (binary_file_displ + binary_file_count + context->output->pc > UINT16_MAX) {
                tiny_error(operands[2]->token, ERROR_MODE_RECOVER, "Combination of file displacement and size outside of range");
                return;
            }
        }
    }
    char file_name_text[TOKEN_TEXT_MAX_LEN] = {}, *c = file_name_text;
    const char *file_name_src = file_token->src.ref + file_token->src.start + 1;
    while (*file_name_src != '"' && c != file_name_text + TOKEN_TEXT_MAX_LEN) {
        *c++ = *file_name_src++;
    }
    binary_file bf;
    htable_entry *bin_entry = string_htable_find_bucket(context->binary_files, file_name_text);
    if (!bin_entry) {
        bf = binary_file_read(file_name_text);
        if (!bf.read_success) {
            tiny_error(file_token, ERROR_MODE_RECOVER, "File not found");
            return;
        }
        if (!string_htable_add(context->binary_files, file_name_text, (htable_value_ptr)&bf)) {
            tiny_error(file_token, ERROR_MODE_RECOVER, "Could not open file");
            return;
        }
    } else {
        bf = *(binary_file*)bin_entry->value;
    }
    if (!bf.read_success) {
        return;
    }
    if (binary_file_count < 0) {
        if (binary_file_displ > bf.length) {
            tiny_error(operands[2]->token, ERROR_MODE_RECOVER, "Specified file offset greater than file size");
            return;
        }
        binary_file_count = bf.length - binary_file_displ;
    } else if (binary_file_count + binary_file_displ > bf.length) {
        tiny_error(operands[1]->token, ERROR_MODE_RECOVER, "Specified file count and offset greater than file size");
        return;
    }
    output_add_values(context->output, bf.data + binary_file_displ, binary_file_count);
}

static void gen_fill(assembly_context *context, token_type directive, const operand *operand)
{
    expression **operands = (expression**)operand->expression_list.expressions->data;
    value amount = evaluate_expression(context, operands[0]);
    if (amount < INT16_MIN || amount > UINT16_MAX) {
        if (context->pass_needed) {
            output_fill(context->output, 1);
            return;
        }
        tiny_error(operands[0]->token, ERROR_MODE_RECOVER, "Illegal quantity");
        return;
    }
    if (directive == TOKEN_ALIGN) {
        int align = 0;
        while ((context->output->logical_pc + align) % (int)amount != 0) {
            align++;
        }
        amount = align;
    }
    if (operand->expression_list.expressions->count > 1) {
        if (operand->expression_list.expressions->count > 2) {
            tiny_error(operands[2]->token, ERROR_MODE_RECOVER, "Unexpected expression");
            return;
        }
        value fill = evaluate_expression(context, operands[1]);
        if (fill == VALUE_UNDEFINED) {
            output_fill(context->output, (int)amount);
            return;
        }
        output_fill_value(context->output, (int)amount, fill);
    } else {
        output_fill(context->output, (int)amount);
    }
}

static void gen_tostring(assembly_context *context, const operand *operand)
{
    expression **values = (expression **)operand->expression_list.expressions->data;
    for(size_t i = 0; i < operand->expression_list.expressions->count; i++) {
        if (values[i]->type == TYPE_LITERAL && values[i]->token->type == TOKEN_STRINGLITERAL) {
            gen_string(context, values[i]->token, 0);
        } else {
            char formatted[64] = {};
            value val = evaluate_expression(context, values[i]);
            if (val != VALUE_UNDEFINED) {
                if (val < INT32_MIN || val > UINT32_MAX) {
                    tiny_error(values[i]->token, ERROR_MODE_RECOVER, "Illegal quantity");
                    return;
                }
                snprintf(formatted, 63, "%lld", val);
            } else {
                snprintf(formatted, 63, "\xff");  
            }
            output_add_values(context->output, formatted, strlen(formatted));
        }
    }
}

static void relocate(assembly_context *context, token_type directive, const operand *operand)
{
    if (directive == TOKEN_RELOCATE) {
        expression **exprs = (expression **)operand->expression_list.expressions->data;
        if (operand->expression_list.expressions->count > 1) {
            tiny_error(exprs[1]->token, ERROR_MODE_RECOVER, "Unexpected expression");
            return;
        }
        value logical_pc = evaluate_expression(context, exprs[0]);
        if (logical_pc < INT16_MIN || logical_pc > UINT16_MAX) {
            if (!context->pass_needed) {
                tiny_error(exprs[0]->token, ERROR_MODE_RECOVER, "Illegal quantity");
            }
            return;
        }
        context->logical_start_pc =
        context->output->logical_pc = logical_pc & 0xffff;
    } else {
        context->logical_start_pc =
        context->output->logical_pc = context->output->pc;
    }
}

static void set_page(assembly_context *context, const token *directive_token, const operand *operand)
{
    if (context->options.cpu != CPU_65816) {
        tiny_error(directive_token, ERROR_MODE_RECOVER, "Invalid pseudo-op for non-65816 CPU");
        return;
    }
    expression **exprs = (expression **)operand->expression_list.expressions->data;
    if (operand->expression_list.expressions->count > 1) {
        tiny_error(exprs[1]->token, ERROR_MODE_RECOVER, "Unexpected expression");
        return;
    }
    value page = evaluate_expression(context, exprs[0]);
    if (page < INT8_MIN || page > UINT8_MAX) {
        if (!context->pass_needed) {
            tiny_error(exprs[0]->token, ERROR_MODE_RECOVER, "Illegal quantity");
        }
        return;
    }
    context->page = (int)page & 0xff;
}

static void set_register_sizes(assembly_context *context, const token *directive_token)
{
    if (context->options.cpu != CPU_65816) {
        tiny_warn(directive_token, "Psuedo-op has no effect for non-65816 CPU");
        return;
    }
    token_type directive = directive_token->type;
    if (directive != TOKEN_X8 && directive != TOKEN_X16) {
        context->m16 = directive == TOKEN_M16 || directive == TOKEN_MX16;
    }
    if (directive != TOKEN_M8 && directive != TOKEN_M16) {
        context->x16 = directive == TOKEN_X16 || directive == TOKEN_MX16;    
    }
}

static void set_print_off_on(assembly_context *context, token_type directive, const operand *operand)
{
    context->print_off = directive == TOKEN_PROFF;
}

void pseudo_op_gen(assembly_context *context, const token *directive_token, const operand *operand)
{
    token_type directive = directive_token->type;
    switch (directive) {
        case TOKEN_M8:
        case TOKEN_M16:
        case TOKEN_MX8:
        case TOKEN_MX16:
        case TOKEN_X8:
        case TOKEN_X16: set_register_sizes(context, directive_token); break;
        case TOKEN_RELOCATE:
        case TOKEN_ENDRELOCATE: relocate(context, directive, operand); break;
        case TOKEN_BINARY: gen_binary_file(context, operand); break;
        case TOKEN_BYTE: gen_values(context, operand, 1); break;
        case TOKEN_WORD: gen_values(context, operand, 2); break;
        case TOKEN_LONG: gen_values(context, operand, 3); break;
        case TOKEN_DWORD: gen_values(context, operand, 4); break;
        case TOKEN_FILL: gen_fill(context, directive, operand); break;
        case TOKEN_STRINGIFY: gen_tostring(context, operand); break;
        case TOKEN_DP: set_page(context, directive_token, operand); break;
        case TOKEN_PRON:
        case TOKEN_PROFF: set_print_off_on(context, directive, operand); break;
        default: gen_strings(context, directive, operand);
    }
}
