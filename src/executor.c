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
#include "error.h"
#include "executor.h"
#include "expression.h"
#include "evaluator.h"
#include "m6502.h"
#include "operand.h"
#include "output.h"
#include "pseudo_op.h"
#include "statement.h"
#include "token.h"
#include "value.h"
#include <stdio.h>
#include <string.h>

static void assemble(assembly_context *context, const statement *statement)
{
    char disassembly[16];
    m6502_gen(context, statement->instruction, statement->operand, disassembly);
    assembly_context_add_disasm(context, disassembly, statement->instruction->src.ref, '.');
}

static void pseudo_op(assembly_context *context, const statement *statement)
{
    pseudo_op_gen(context, statement->instruction, statement->operand);
    if (context->start_pc < context->output->pc) {
        /* only disassemble pseudo-ops that output*/
        assembly_context_add_disasm(context, NULL, statement->instruction->src.ref, '>');
    }
}

typedef struct overflow_context
{

    const assembly_context *asm_context;
    const statement *statement;

} overflow_context;

static void pc_overflow_handler(const output *output, const void *overflow_ctx_ptr)
{
    const overflow_context *overflow_ctx = (const overflow_context*)overflow_ctx_ptr;
    if (!overflow_ctx->asm_context->pass_needed) {
        tiny_error(overflow_ctx->statement->instruction, ERROR_MODE_RECOVER, 
            "Program counter overflow");
    }
}

static void disassemble_label(assembly_context *context, const statement *statement, value label_value)
{
    if (label_value == VALUE_UNDEFINED) {
        return;
    }
    if (statement->label->type == TOKEN_IDENT &&
        statement->instruction &&
        statement->instruction->type == TOKEN_EQUAL) {
        char disasm[16] = {};
        snprintf(disasm, 16, "$%x", (int)label_value);
        assembly_context_add_disasm_opt_pc(context, disasm, statement->label->src.ref, '=', 0);
        return;
    }
    assembly_context_add_disasm(context, NULL, statement->label->src.ref, '.');
}

static void create_or_update_label(assembly_context *context, const statement *statement)
{
    if (!context->passes && (!statement->label ||
        (statement->label->type != TOKEN_PLUS && statement->label->type != TOKEN_HYPHEN))) {
        anonymous_label_add(context->anonymous_labels_new);
    }
    if (!statement->label) {
            return;
    }
    value label_val = context->output->logical_pc;
    if (statement->instruction && statement->instruction->type == TOKEN_EQUAL) {
        label_val = evaluate_expression(context, statement->operand->single_expression.expr);
        if (statement->label->type == TOKEN_ASTERISK) {
            if (label_val < INT16_MIN || label_val > UINT16_MAX) {
                if (!context->pass_needed && label_val != VALUE_UNDEFINED) {
                    tiny_error(statement->operand->single_expression.expr->token, ERROR_MODE_RECOVER, "Illegal quantity");
                }
                return;
            }
            context->output->logical_pc = context->output->pc = (int)(label_val & 0xffff);
            context->logical_start_pc =
            context->start_pc = context->output->pc;
            disassemble_label(context, statement, label_val);
            return;
        }
        if (statement->label->type != TOKEN_IDENT) {
            tiny_error(statement->instruction, ERROR_MODE_RECOVER, "Invalid operation");
        }
        disassemble_label(context, statement, label_val);
    }
    if (statement->label->type == TOKEN_IDENT) {
        char label_name[TOKEN_TEXT_MAX_LEN * 2 + 2] = {};
        const char *label_start = statement->label->src.ref + statement->label->src.start;
        if (*label_start == '_') {
            TOKEN_GET_TEXT(statement->label, label_text);
            if (context->local_label) {
                TOKEN_GET_TEXT(context->local_label, local_label);
                snprintf(label_name, TOKEN_TEXT_MAX_LEN, "%s.%s", local_label, label_text);
            }
            else {
                strncpy(label_name, label_text, TOKEN_TEXT_MAX_LEN);
            }
        } else {
            token_copy_text_to_buffer(statement->label, label_name, TOKEN_TEXT_MAX_LEN);
            context->local_label = statement->label;
        }
        if (context->passes && symbol_table_lookup(context->sym_tab, label_name) != label_val) {
            context->pass_needed = 1;
            symbol_table_update(context->sym_tab, label_name, label_val);
        } else if (!context->passes) {
            if (!symbol_exists(context->sym_tab, label_name)) {
                symbol_table_define(context->sym_tab, label_name, label_val);
            }
            else {
                tiny_error(statement->label, ERROR_MODE_RECOVER, "Symbol '%s' already exists", label_name);
            }
        }
        goto add_to_disassembly;
    }
    
    if (context->passes > 0) {
        context->pass_needed |= label_val != anonymous_label_get_current(context->anonymous_labels_new, statement->index);
        anonymous_label_update_current(context->anonymous_labels_new, statement->index, label_val);
    }
    if (statement->label->type == TOKEN_PLUS) {
        anonymous_label_add_forward(context->anonymous_labels_new, label_val);
    } else {
        anonymous_label_add_backward(context->anonymous_labels_new, label_val);
    }
add_to_disassembly:
    if (!statement->instruction) {
        disassemble_label(context, statement, label_val);
    }
}

void statement_execute(assembly_context *context, const statement *statement)
{
    context->logical_start_pc = context->output->logical_pc;
    context->start_pc = context->output->pc;
    create_or_update_label(context, statement);
    if (statement->instruction &&
        ((statement->instruction->type >= TOKEN_ANC && statement->instruction->type <= TOKEN_TOP) ||
         (statement->instruction->type >= TOKEN_BBR && statement->instruction->type <= TOKEN_XCE) ||
        statement->instruction->type >= TOKEN_ADC)) {
        /* set up program counter overflow handler */
        overflow_context overflow_ctx = {
            .asm_context = context,
            .statement = statement
        };
        output_set_overflow_handler(context->output, pc_overflow_handler, &overflow_ctx);
        if (statement->instruction->type <= TOKEN_TYA) {
            assemble(context, statement);
        } else {
            pseudo_op(context, statement);
        }
    }
}
