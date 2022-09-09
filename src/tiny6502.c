/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "assembly_context.h"
#include "builtin_symbols.h"
#include "memory.h"
#include "error.h"
#include "evaluator.h"
#include "executor.h"
#include "expression.h"
#include "lexer.h"
#include "m6502.h"
#include "options_parser.h"
#include "parser.h"
#include "file.h"
#include "statement.h"
#include "string_htable.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>

const char PRODUCT_NAME[] = "tiny6502 cross-assembler";
const char VERSION[] = "0.1";
const char COPYRIGHT[] = "(c) 2022 informedcitizenry";
const char LEGAL[] = "tiny6502 comes with ABSOLUTELY NO WARRANTY. "
                     "This is free software, and you are welcome to redistribute it under certain "
                     "conditions as defined in the LICENSE.";
                     
const int MAX_PASSES = 4;

dynamic_array *first_pass(assembly_context *context, parser *parser)
{
    dynamic_array *stats;
    DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(stats, statement*, 100);

    printf("%s %s %s\n%s\n", PRODUCT_NAME, VERSION, COPYRIGHT, LEGAL);
    for(;;) {
        statement *stat = parse_statement(parser);
        if (!stat) {
            break;
        }
        dynamic_array_add(stats, stat);
        if (!tiny_error_count()) {
            statement_execute(context, stat);
        }
    }
    context->passes++;
    return stats;
}

static void add_reserved_words(assembly_context *ctx, lexer *lexer)
{
    if (ctx->options.cpu != CPU_6502) {
        switch (ctx->options.cpu) {
            case CPU_6502I:
                lexer_add_reserved_words(lexer, M6502I_WORDS, m6502i_mnemonics, m6502i_types);
                break;
            case CPU_65C02:
                lexer_add_reserved_words(lexer, W65C02_WORDS, w65c02_mnemonics, w65c02_types);
                break;
            default:
                lexer_add_reserved_words(lexer, W65816_WORDS, w65816_mnemonics, w65816_types);
        }
    }
}

static void statement_dtor(void *stat_ptr)
{
    statement_destroy((statement*)stat_ptr);
}

int main(int argc, const char * argv[])
{
    tiny_reset_errors_warnings();
    options opts = options_parse(argc, argv);
    assembly_context *ctx = assembly_context_create(opts);
    
    if (ctx->options.input) {
        ctx->source = source_file_read(ctx->options.input);
        if (!ctx->source.lines || !ctx->source.file_name) {
            tiny_error(NULL, ERROR_MODE_PANIC, "Unable to read file %s.", ctx->options.input);
        }
    } else {
        ctx->source = source_file_from_user_input();
    }

    lexer *lexer = lexer_create(&ctx->source, ctx->options.case_sensitive),
            *defines_lexer = NULL;
            
    add_reserved_words(ctx, lexer);
    builtin_init(ctx->options.case_sensitive);
    
    parser *parser = parser_create(lexer, ctx->options.case_sensitive),
            *defines_parser = NULL;


    if (ctx->options.defines.line_numbers) {
        /* parse '--defines' options as constants */
        defines_lexer = lexer_create(&ctx->options.defines, ctx->options.case_sensitive);
        add_reserved_words(ctx, defines_lexer);
        defines_parser = parser_create(defines_lexer, ctx->options.case_sensitive);
        statement *assign_stat;
        for (;;) {
            assign_stat = parse_assignment(defines_parser);
            if (!assign_stat) {
                break;
            }
            expression *assign_expr = assign_expression(defines_parser, assign_stat);
            if (assign_expr) {
                value v = evaluate_expression(ctx, assign_expr);
                expression_destroy(assign_expr);
                if (v == VALUE_UNDEFINED) {
                    tiny_error(NULL, ERROR_MODE_PANIC, "Option --define argument must be a constant expression");
                }
            }
            statement_destroy(assign_stat);
        }
        if (tiny_error_count()) {
            tiny_error(NULL, ERROR_MODE_PANIC, "One or more arguments for option '--define' is invalid");
        }
    }
    dynamic_array *stat_array = first_pass(ctx, parser);
    stat_array->dtor = statement_dtor;
    if (!tiny_error_count()) {
        statement **stats = (statement**)stat_array->data;
        while (ctx->pass_needed && ctx->passes <= MAX_PASSES && !tiny_error_count()) {
            /* run multiple passes as needed */
            ctx->passes++;
            assembly_context_reset(ctx);
            value curr_pass = ctx->passes + 1;
            string_htable_update(BUILTIN_SYMBOL_TABLE, "CURRENT_PASS", (const htable_value_ptr)&curr_pass);
            for(int i = 0; i < stat_array->count; i++) {
                statement_execute(ctx, stats[i]);
            }
        }
    }

    if (tiny_warn_count()) {
        printf("%d warnings.\n", tiny_warn_count());
    }
    if (tiny_error_count()) {
        printf("%d errors.\n", tiny_error_count());
    } else if (ctx->passes <= MAX_PASSES) {
        printf("---------------------------------\n%d passes\n", ctx->passes);
        assembly_context_to_disk(ctx);
    }
    if (ctx->passes > MAX_PASSES) {
        perror("Too many passes.");
    }
    /* final cleanup */
    dynamic_array_cleanup_and_destroy(stat_array);
    parser_destroy(defines_parser);
    parser_destroy(parser);
    lexer_destroy(defines_lexer);
    lexer_destroy(lexer);
    builtin_cleanup();
    assembly_context_destroy(ctx);
    
    return EXIT_SUCCESS;
}
