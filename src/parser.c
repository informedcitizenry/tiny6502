/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "error.h"
#include "expression.h"
#include "file.h"
#include "lexer.h"
#include "macro.h"
#include "memory.h"
#include "operand.h"
#include "parser.h"
#include "statement.h"
#include "string_htable.h"
#include "token.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct op_descriptor
{
    int precedence;
    enum { ASSOC_LEFT, ASSOC_RIGHT } associativity;
} op_descriptor;

static const char *token_type_names[TOKEN_TYPE_NUM] = 
{
    "<EOF>",
    "'.'",
    "'^^'",
    "'*'",
    "'/'",
    "'%'",
    "'+'",
    "'-'",
    "'<<'",
    "'>>'",
    "'>>>'",
    "'<'",
    "'<='",
    "'>='",
    "'>'",
    "'<=>'",
    "'=='",
    "'!='",
    "'&'",
    "'^'",
    "'|'",
    "'&&'",
    "'||'",
    "'?'",
    "'='",
    "':'",
    "'('",
    "'['",
    "'{'",
    "','",
    "')'",
    "']'",
    "'}'",
    "'!'",
    "'~'",
    "FORWARD_REFERENCE",
    "BACKWARD_REFERENCE",
    "'#'",
    "",
    "-C",
    "--format",
    "--help",
    "--labels",
    "--list",
    "--quiet",
    "--version",
    "",
    "--output",
    "MACRO_SUBSTITUTION",
    "MACRO_SUBSTITUTION",
    "NEWLINE",
    "?UNRECOGNIZED",
    "NUMBER",
    "NUMBER",
    "NUMBER",
    "STRING",
    "CHAR",
    "IDENT",
    "anc",
    "ane",
    "arr",
    "asr",
    "dcp",
    "dop",
    "isb",
    "jam",
    "las",
    "lax",
    "rla",
    "rra",
    "sax",
    "sha",
    "shx",
    "shy",
    "slo",
    "stp",
    "sre",
    "tas",
    "top",
    "'S'",
    "bbr",
    "bbs",
    "bra",
    "brl",
    "cop",
    "jml",
    "jsl",
    "mvn",
    "mvp",
    "pea",
    "pei",
    "per",
    "phb",
    "phd",
    "phk",
    "phx",
    "phy",
    "plb",
    "pld",
    "plx",
    "ply",
    "rep",
    "rmb",
    "rtl",
    "sep",
    "smb",
    "stp",
    "stz",
    "tcd",
    "tcs",
    "tdc",
    "trb",
    "tsb",
    "tsc",
    "txy",
    "tyx",
    "wai",
    "wdm",
    "xba",
    "xce",
    "",
    "'a'",
    "'x'",
    "'y'",
    "adc",
    "and",
    "asl",
    "bcc",
    "bcs",
    "beq",
    "bit",
    "bmi",
    "bne",
    "bpl",
    "brk",
    "bvc",
    "bvs",
    "clc",
    "cld",
    "cli",
    "clv",
    "cmp",
    "cpx",
    "cpy",
    "dec",
    "dex",
    "dey",
    "eor",
    "inc",
    "inx",
    "iny",
    "jmp",
    "jsr",
    "lda",
    "ldx",
    "ldy",
    "lsr",
    "nop",
    "ora",
    "pha",
    "php",
    "pla",
    "plp",
    "rol",
    "ror",
    "rti",
    "rts",
    "sbc",
    "sec",
    "sed",
    "sei",
    "sta",
    "stx",
    "sty",
    "tax",
    "tay",
    "tsx",
    "txa",
    "txs",
    "tya",
    ".include",
    ".macro",
    ".m8",
    ".m16",
    ".mx8",
    ".mx16",
    ".x8",
    ".x16",
    ".align",
    ".binary",
    ".byte",
    ".word",
    ".dword",
    ".fill",
    ".long",
    ".tostring",
    ".relocate",
    ".endrelocate",
    ".dp",
    ".string",
    ".cstring",
    ".lstring",
    ".nstring",
    ".pstring",
    "MACRO_DEFINITION",
    ".end",
    ".endmacro"
};

static op_descriptor BINARY_OPERATORS[] = 
{
    {  0, ASSOC_LEFT }, /* EOF */
    { 15, ASSOC_RIGHT }, /* . */
    { 14, ASSOC_RIGHT }, /* ^^ */
    { 13, ASSOC_LEFT }, /* * */
    { 13, ASSOC_LEFT }, /* / */
    { 13, ASSOC_LEFT }, /* % */
    { 12, ASSOC_LEFT }, /* + */
    { 12, ASSOC_LEFT }, /* - */
    { 11, ASSOC_LEFT }, /* << */
    { 11, ASSOC_LEFT }, /* >> */
    { 11, ASSOC_LEFT }, /* >>> */
    { 10, ASSOC_LEFT }, /* <= */
    { 10, ASSOC_LEFT }, /* < */
    { 10, ASSOC_LEFT }, /* >= */
    { 10, ASSOC_LEFT }, /* > */
    { 10, ASSOC_LEFT }, /* <=> */
    {  9, ASSOC_LEFT }, /* == */
    {  9, ASSOC_LEFT }, /* != */
    {  8, ASSOC_LEFT }, /* & */
    {  7, ASSOC_LEFT }, /* ^ */
    {  6, ASSOC_LEFT }, /* | */
    {  5, ASSOC_LEFT }, /* && */
    {  4, ASSOC_LEFT }, /* || */
    {  3, ASSOC_LEFT }, /* ? */
    {  2, ASSOC_LEFT }, /* : */
    {  1, ASSOC_RIGHT } /* = */
};

static const token_type pseudo_op_no_operand[] =
{
    TOKEN_ENDRELOCATE,
    TOKEN_M8,
    TOKEN_M16,
    TOKEN_MX8,
    TOKEN_MX16,
    TOKEN_PROFF,
    TOKEN_PRON,
    TOKEN_X8,
    TOKEN_X16
};

static const token_type byte_extractors[] =
{
    TOKEN_AMPERSAND, TOKEN_CARET, TOKEN_LANGLE, TOKEN_RANGLE
};

/* all the token types that are expressions or indicate the (potential)
   start of an expression */
const token_type expression_types[] =
{
    TOKEN_CHARLITERAL,
    TOKEN_STRINGLITERAL,
    TOKEN_HEXLITERAL,
    TOKEN_DECLITERAL,
    TOKEN_BINLITERAL,
    TOKEN_IDENT,
    TOKEN_PLUS,
    TOKEN_HYPHEN,
    TOKEN_MULTIPLUS,
    TOKEN_MULTIHYPHEN,
    TOKEN_LPAREN,
    TOKEN_BANG,
    TOKEN_LANGLE,
    TOKEN_RANGLE,
    TOKEN_TILDE
};

typedef struct parser
{
    size_t start_position;
    size_t position;
    size_t statements;
    int errors;
    int expect_assignment;
    dynamic_array *token_buffer;
    token *current_token;
    string_htable *macro_defs;
    lexer *lexer;
} parser;

static expression *parse_expr(parser *parser);
static void eat(parser *parser);
static int is_eos(parser* parser);

#define EXPECT_OR_FREE(p, t, o) \
if (!expect(p, t)) { tiny_free(o); return NULL; }

static void error(parser *parser, const token *token, const char *fmt, ...)
{
    parser->errors = 1;
    va_list ap;
    va_start(ap, fmt);
    tiny_verror(token, ERROR_MODE_RECOVER, fmt, ap);
    va_end(ap);
    while (!is_eos(parser)) {
        eat(parser);
    }
}

static int match(parser *parser, token_type type)
{
    return parser->current_token->type == type;
}

static void eat(parser *parser)
{
    if (parser->position < parser->token_buffer->count) {
        parser->current_token = parser->token_buffer->data[parser->position];
    } else {
        token *token = next_token(parser->lexer);
        dynamic_array_add(parser->token_buffer, token);
        parser->current_token = token;
    }
    parser->position++;
}

static token *peek(parser *parser)
{
    token *current = parser->current_token;
    size_t pos = parser->position, peek_pos = pos;
    if (parser->token_buffer->count <= pos + 1) {
        eat(parser);
        peek_pos = parser->position - 1;
        parser->current_token = current;
        parser->position = pos;
    }
    return parser->token_buffer->data[peek_pos];
}

static int expect(parser *parser, token_type type)
{
    if (match(parser, type)) {
        eat(parser);
        return 1;
    }
    token *current = parser->token_buffer->data[parser->position - 1];
    TOKEN_GET_TEXT(current, token_text);
    const char *expected = token_type_names[type];
    if (type == TOKEN_NEWLINE) {
        error(parser, current, "Expected statement terminator but found '%s'", token_text);
    } else if (current->type == TOKEN_EOF) {
        error(parser, current, "Expected token %s but found end of source", expected);
    } else if (token_text[0] == '\n') {
        error (parser, current, "Expected token %s but found newline character", expected);
    } else {
        error(parser, current, "Expected token %s but found '%s'", expected, token_text);
    }
    return 0;
}

static int is_eos(parser *parser)
{
    return match(parser, TOKEN_COLON) || match(parser, TOKEN_EOF) || match(parser, TOKEN_NEWLINE);
}

static void eos(parser *parser)
{
    if (is_eos(parser)) {
        eat(parser);
        while (match(parser, TOKEN_NEWLINE)) {
            eat(parser);
        }
        return;
    }
    expect(parser, TOKEN_NEWLINE);
}

static expression_array *parse_expr_list(parser *parser)
{
    expression_array *exprs;
    DYNAMIC_ARRAY_CREATE(exprs, struct expression*);
    for(;;) {
        expression *expr = parse_expr(parser);
        if (!expr) {
            break;
        }
        dynamic_array_add(exprs, expr);
        if (!match(parser, TOKEN_COMMA)) {
            break;
        }
        eat(parser);
    }
    return exprs;
}

static expression *plus_hyphen(parser *parser)
{
    token *oper = parser->current_token;
    eat(parser);
    if (!token_is_of_type(parser->current_token, expression_types, sizeof expression_types)) {
        return expression_literal_ident(oper, 1);
    }
    return expression_unary(oper, parse_expr(parser));
}

static void expr_dtor(void *expr_ptr)
{
    expression_destroy((expression*)expr_ptr);
}

static expression *parse_ident(parser *parser)
{
    token *ident = parser->current_token;
    eat(parser);
    if (match(parser, TOKEN_LPAREN)) {
        eat(parser);
        expression_array *params = parse_expr_list(parser);
        if (!expect(parser, TOKEN_RPAREN)) {
            if (params) {
                params->dtor = expr_dtor;
                dynamic_array_cleanup_and_destroy(params);
            }
            return NULL;
        }
        return expression_fcn_call(ident, params);
    }
    return expression_literal_ident(ident, 1);
}

static expression *factor(parser *parser)
{
    token *current = parser->current_token;
    switch (current->type) {
        case TOKEN_PLUS:
        case TOKEN_HYPHEN:
            return plus_hyphen(parser);
        case TOKEN_MULTIPLUS:
        case TOKEN_MULTIHYPHEN:
        case TOKEN_ASTERISK:
            eat(parser);
            return expression_literal_ident(current, 1);
        case TOKEN_IDENT:
            return parse_ident(parser);
        case TOKEN_BANG:
        case TOKEN_TILDE:
            eat(parser);
            return expression_unary(current, parse_expr(parser));
        case TOKEN_LPAREN: {
            eat(parser);
            expression *inner = parse_expr(parser);
            EXPECT_OR_FREE(parser, TOKEN_RPAREN, inner);
            return inner;
        }
        case TOKEN_STRINGLITERAL:
        case TOKEN_CHARLITERAL:
            eat(parser);
            return expression_literal_ident(current, 0);
        case TOKEN_HEXLITERAL:
        case TOKEN_DECLITERAL:
        case TOKEN_BINLITERAL: {
            expression *expr = expression_literal_ident(current, 0);
            eat(parser);
            return expr;
        }
        default:
            error(parser, parser->current_token, "Expression expected");
            return NULL;
    }
}

static expression *ternary_expr(parser *parser, expression *cond)
{
    token *oper = parser->current_token;
    eat(parser);
    expression *then = parse_expr(parser);
    EXPECT_OR_FREE(parser, TOKEN_COLON, then);
    return expression_ternary(oper, cond, then, parse_expr(parser));
}

static expression *binary_expr(parser *parser, int precedence)
{
    expression *lhs = factor(parser);
    if (!lhs) {
        return NULL;
    }
    for(;;) {
        if (parser->current_token->type < TOKEN_DOT || parser->current_token->type > TOKEN_EQUAL) {
            break;
        }
        token *op_token = parser->current_token;
        const op_descriptor *op_desc = BINARY_OPERATORS + op_token->type;
        if (op_desc->precedence < precedence) {
            break;
        }
        if (match(parser, TOKEN_QUERY)) {
            return ternary_expr(parser, lhs);
        }
        int next_prec = op_desc->associativity == ASSOC_LEFT ? op_desc->precedence + 1 : op_desc->precedence;
        eat(parser);
        expression *rhs = binary_expr(parser, next_prec);
        if (!rhs) {
            tiny_free(lhs);
            return NULL;
        }
        lhs = expression_binary(op_token, lhs, rhs);
    }
    return lhs;
}

static expression *parse_expr(parser *parser)
{
    expression *expr;
    if (token_is_of_type(parser->current_token, byte_extractors, sizeof byte_extractors)) {
        token *extractor = parser->current_token;
        eat(parser);
        expr = expression_unary(extractor, parse_expr(parser));
    } else {
        expr = binary_expr(parser, 0);
        if (!expr) {
            return expr;
        }
        if (expr->type == TYPE_BINARY && expr->token->type == TOKEN_EQUAL && !parser->expect_assignment) {
            error(parser, expr->token, "Assignment illegal in expression");
            tiny_free(expr);
            return NULL;
        }
        if (parser->expect_assignment && (expr->type != TYPE_BINARY || expr->token->type != TOKEN_EQUAL)) {
            error(parser, expr->token, "Assignment expression expected");
            tiny_free(expr);
            return NULL;
        }
    }
    return expr;
}

static operand *parse_operand(parser *parser, int bit)
{
    if (is_eos(parser)) {
        return NULL;
    }
    expression *bitwidth = NULL;
    int mode = FORM_ZP_ABSOLUTE;
    if (bit) {
        /* bit operand := '0' .. '7' ',' expr (',' expr)? */
        expression *bit_expr = parse_expr(parser);
        if (!bit_expr) {
            return NULL;
        }
        if (bit_expr->type != TYPE_LITERAL || bit_expr->token->type != TOKEN_DECLITERAL || bit_expr->value < 0 || bit_expr->value > 7) {
            error(parser, expression_get_lhs_token(bit_expr), "Invalid bit constant");
            tiny_free(bit_expr);
            eos(parser);
            return NULL;
        }
        EXPECT_OR_FREE(parser, TOKEN_COMMA, bit_expr);
        expression *expr0 = parse_expr(parser);
        if (is_eos(parser)) {
            /* expr ',' expr */
            return operand_bit(bit_expr, expr0);
        }
        if (!expect(parser, TOKEN_COMMA)) {
            tiny_free(bit_expr);
            tiny_free(expr0);
            return NULL;
        }
        /* expr ',' expr ',' expr */
        return operand_bit_offset(bit_expr, expr0, parse_expr(parser));
    }
    if (match(parser, TOKEN_LSQUARE)) {
        eat(parser);
        bitwidth = parse_expr(parser);
        expect(parser, TOKEN_RSQUARE);
        if (is_eos(parser) || match(parser, TOKEN_COMMA)) {
            if (is_eos(parser)) {
                return operand_single_expression(FORM_DIRECT, bitwidth, NULL);
            }
            eat(parser);
            EXPECT_OR_FREE(parser, TOKEN_Y, bitwidth);
            return operand_single_expression(FORM_DIRECT_Y, bitwidth, NULL);
        } else if (bitwidth->type != TYPE_LITERAL && bitwidth->token->type != TOKEN_DECLITERAL) {
            error(parser, expression_get_lhs_token(bitwidth), "Invalid bitwidth specifier argument");
            tiny_free(bitwidth);
            return NULL;
        }
    }
    if (match(parser, TOKEN_HASH)) {
        eat(parser);
        mode = FORM_IMMEDIATE;
    }
    else if (match(parser, TOKEN_LSQUARE)) {
        eat(parser);
        expression *expr = parse_expr(parser);
        if (!expect(parser, TOKEN_RSQUARE)) return NULL;
        if (is_eos(parser)) {
            return operand_single_expression(FORM_DIRECT, expr, bitwidth);
        }
        eat(parser);
        EXPECT_OR_FREE(parser, TOKEN_Y, expr);
        return operand_single_expression(FORM_DIRECT_Y, expr, bitwidth);
    }
    else if (match(parser, TOKEN_LPAREN)) {
        size_t mark_pos = parser->position;
        token *curr_token = parser->current_token;
        eat(parser);
        expression *expr = parse_expr(parser);
        if (!expr) return NULL;
        if (match(parser, TOKEN_COMMA)) {
            eat(parser);
            if (match(parser, TOKEN_S)) {
                eat(parser);
                EXPECT_OR_FREE(parser, TOKEN_RPAREN, expr);
                EXPECT_OR_FREE(parser, TOKEN_COMMA, expr);
                EXPECT_OR_FREE(parser, TOKEN_Y, expr);
                return operand_single_expression(FORM_INDIRECT_S, expr, bitwidth);
            }
            EXPECT_OR_FREE(parser, TOKEN_X, expr); 
            EXPECT_OR_FREE(parser, TOKEN_RPAREN, expr);
            return operand_single_expression(FORM_INDIRECT_X, expr, bitwidth);
        }
        EXPECT_OR_FREE(parser, TOKEN_RPAREN, expr);
        if (match(parser, TOKEN_COMMA)) {
            eat(parser);
            EXPECT_OR_FREE(parser, TOKEN_Y, expr);
            return operand_single_expression(FORM_INDIRECT_Y, expr, bitwidth);
        }
        if (is_eos(parser)) {
            return operand_single_expression(FORM_INDIRECT, expr, bitwidth);
        }
        parser->position = mark_pos;
        parser->current_token = curr_token;
    }
    else if (match(parser, TOKEN_A)) {
        if (bitwidth) {
            error(parser, expression_get_lhs_token(bitwidth), "Invalid use of bitwidth modifier");
        }
        eat(parser);
        return operand_single_expression(FORM_ACCUMULATOR, NULL, bitwidth);
    }
    expression *expr = parse_expr(parser);
    if (!expr) return NULL;
    if (match(parser, TOKEN_COMMA)) {
        if (mode == FORM_IMMEDIATE) {
            EXPECT_OR_FREE(parser, TOKEN_NEWLINE, expr);
        }
        eat(parser);
        int form = FORM_TWO_OPERANDS;
        switch (parser->current_token->type) {
            case TOKEN_S: form = FORM_INDEX_S; break;
            case TOKEN_X: form = FORM_INDEX_X; break;
            case TOKEN_Y: form = FORM_INDEX_Y; break;
            default: break;
        }
        if (form != FORM_TWO_OPERANDS) {
            eat(parser);
            return operand_single_expression(form, expr, bitwidth);
        }
        if (bitwidth) {
            error(parser, expression_get_lhs_token(bitwidth), "Invalid use of bitwidth modifier");
            tiny_free(bitwidth);
            return NULL;
        }
        return operand_two_expressions(expr, parse_expr(parser));
    }
    return operand_single_expression(mode, expr, bitwidth);
}

static void destroy_param(void *param_ptr)
{
    dynamic_array *param = (dynamic_array*)param_ptr;
    dynamic_array_destroy(param);
}

static statement *macro_expand(parser *parser, statement *statement)
{
    TOKEN_GET_TEXT(statement->instruction, macro_name);
    macro *m = (macro*)string_htable_get(parser->macro_defs, macro_name);
    if (!m) {
        error(parser, statement->instruction, "Unknown macro name");
        statement_destroy(statement);
        return parse_statement(parser);
    }
    dynamic_array *params = NULL;
    if (!is_eos(parser)) {
        /* for tracking where commas are not parameter delimiters, this is
           irrelevant now because we don't support function calls but will 
           be necessary if and when we do */
        int open = 0;
        DYNAMIC_ARRAY_CREATE(params, struct dynamic_array*);
        params->dtor = destroy_param;
        dynamic_array *curr_param = NULL;
        while (!is_eos(parser)) {
            token *current = parser->current_token;
            if (match(parser, TOKEN_COMMA)) {
                error(parser, current, "Unexpected token");
                goto destroy_and_parse;
            }
            if (current->type == TOKEN_LPAREN || current->type == TOKEN_LSQUARE || current->type == TOKEN_LCURLY) {
                open++;
            }
            if (current->type == TOKEN_RPAREN || current->type == TOKEN_RSQUARE || current->type == TOKEN_RCURLY) {
                open--;
            }
            if (!curr_param) {
                DYNAMIC_ARRAY_CREATE(curr_param, struct token*);
            }
            dynamic_array_add(curr_param, current);
            eat(parser);
            if (match(parser, TOKEN_COMMA)) {
                token *comma = parser->current_token;
                eat(parser);
                if (!open) {
                    if (is_eos(parser)) {
                        error(parser, comma, "Unexpected end of parameter list");
                        goto destroy_and_parse;
                    }
                    dynamic_array_add(params, curr_param);
                    curr_param = NULL;
                }
                else {
                    dynamic_array_add(curr_param, comma);
                }
            }
        }
        if (curr_param) {
            dynamic_array_add(params, curr_param);
        }
    }
    if (m->arg_names && m->arg_names->count > params->count) {
        error(parser, statement->instruction, "Macro definition requires more parameters than provided");
        goto destroy_and_parse;
    }
    dynamic_array *expanded = macro_expand_macro(statement->label, statement->instruction, params, m, parser->lexer);
    eos(parser);
    if (expanded->count) {
        parser->position--;
        dynamic_array_insert_range(parser->token_buffer, expanded, parser->position);
        eat(parser);
    }
    if (params) {
        dynamic_array_cleanup_and_destroy(params);
    }
    dynamic_array_destroy(expanded);
destroy_and_parse:
    statement_destroy(statement);
    return parse_statement(parser);
}

static statement *to_end_macro(parser *parser, statement *statement)
{
    while (!match(parser, TOKEN_ENDMACRO) && !match(parser, TOKEN_EOF)) {
        eat(parser);
    }
    expect(parser, TOKEN_ENDMACRO);
    eos(parser);
    tiny_free(statement);
    return parse_statement(parser);
}

static statement *macro_def(parser *parser, statement *statement)
{
    if (!statement->label || statement->label->type != TOKEN_IDENT) {
        error(parser, statement->instruction, "\".macro\" directive requires identifier");
    }
    char macro_name[TOKEN_TEXT_MAX_LEN] = {};
    macro_name[0] = '.';
    token_copy_text_to_buffer(statement->label, macro_name + 1, TOKEN_TEXT_MAX_LEN - 1);
    if (string_htable_contains(parser->macro_defs, macro_name)) {
        error(parser, statement->label, "Redefinition of macro name '%s'", macro_name);
        return to_end_macro(parser, statement);
    } else if (lexer_is_reserved_word(parser->lexer, macro_name)) {
        error(parser, statement->label, "Macro name resolves to '%s' which is a reserved word", macro_name);
        return to_end_macro(parser, statement);
    }
    string_htable *arg_names = NULL;

    if (!is_eos(parser)) {
        /* arg_names is a hash map to indeces */
        arg_names = string_htable_create(sizeof(size_t));
        arg_names->case_sensitive = lexer_is_case_sensitive(parser->lexer);
        for(;;) {
            token *arg = parser->current_token;
            TOKEN_GET_TEXT(arg, argtext);
            if (expect(parser, TOKEN_IDENT)) {
                string_htable_add(arg_names, argtext, (const htable_value_ptr)&arg_names->count);
   
                if (!match(parser, TOKEN_COMMA)) {
                    break;
                }
                eat(parser);
            } else break;
        }
    }
    eos(parser);
    dynamic_array *macro_block;
    DYNAMIC_ARRAY_CREATE(macro_block, struct token);
    for(;;) {
        if (match(parser, TOKEN_EOF) || match(parser, TOKEN_ENDMACRO)) {
            break;
        }
        if (match(parser, TOKEN_MACRO)) {
            error(parser, parser->current_token, 0, "Macro definition cannot reside within another macro definition");
            while (!match(parser, TOKEN_ENDMACRO) && !match(parser, TOKEN_EOF)) {
                eat(parser);
            }
            if (match(parser, TOKEN_EOF)) {
                break;
            }
        } else {
            dynamic_array_add(macro_block, parser->current_token);
        }
        eat(parser);
    }
    expect(parser, TOKEN_ENDMACRO);
    size_t final_ix = macro_block->count - 1;
    size_t penult_ix = final_ix ? final_ix - 1 : final_ix;
    token *final = (token*)macro_block->data[final_ix];
    if (final->type != TOKEN_NEWLINE) {
        if (final->type != TOKEN_PLUS && final->type != TOKEN_HYPHEN && final->type != TOKEN_IDENT) {
            error(parser, final, "Unexpected token");
        }
        else {
            token *penult = (token*)macro_block->data[penult_ix];
            if (penult->type != TOKEN_NEWLINE) {
                error(parser, penult, "Unexpected token");
            }
        }
    }

    lexer_add_reserved_word(parser->lexer, macro_name);
    eos(parser);
    
    macro *m = macro_create(arg_names, macro_block);
    m->define_token = statement->label;
    string_htable_add(parser->macro_defs, macro_name, (const htable_value_ptr)m);
    
    /* string hash table adds a copy of the passed value, so only free this
     macro pointer, not its contents */
    tiny_free(m);
    statement_destroy(statement);
    return parse_statement(parser);
}

static statement *include(parser *parser, statement *statement)
{
    token *inc_name = parser->current_token;
    if (!match(parser, TOKEN_STRINGLITERAL)) {
        expect(parser, TOKEN_STRINGLITERAL);
        statement_destroy(statement);
        return parse_statement(parser);
    }
    char include_file[TOKEN_TEXT_MAX_LEN] = {};
    size_t len = token_copy_text_to_buffer(inc_name, include_file, TOKEN_TEXT_MAX_LEN);
    include_file[len - 1] = '\0';
    const source_file *lexer_src = lexer_get_source(parser->lexer);
    if (strncmp(include_file + 1, lexer_src->file_name, len - 1) == 0) {
        error(parser, inc_name, "Recursive inclusion of file '%s'", include_file + 1);
        statement_destroy(statement);
        return parse_statement(parser);
    }
    source_file sf = source_file_read(include_file + 1);
    if (sf.lines) {
        dynamic_array *included = lexer_include_and_process(parser->lexer, &sf);
        eat(parser);
        if (!is_eos(parser)) {
            expect(parser, TOKEN_NEWLINE);
            statement_destroy(statement);
            return parse_statement(parser);
        }
        if (statement->label) {
            dynamic_array_insert(parser->token_buffer, statement->label, parser->position++);
        }
        dynamic_array_insert_range(parser->token_buffer, included, parser->position);
        dynamic_array_destroy(included);
    }
    statement_destroy(statement);
    if (!sf.lines) {
        error(parser, inc_name, "Could not open file '%s'", include_file + 1);
    }
    return parse_statement(parser);
}

expression *assign_expression(parser *parser, statement *assign)
{
    if (assign->label) {
        if (assign->instruction && assign->instruction->type == TOKEN_EQUAL && assign->operand->form == FORM_ZP_ABSOLUTE) {
            return expression_binary(assign->instruction, 
                                     expression_literal_ident(assign->label, 1), 
                                     expression_copy(assign->operand->single_expression.expr));
        }
        if (assign->instruction) {
            error(parser, assign->instruction, "Expected assignment operator");
            return NULL;
        }
        static token equal = {
            .type = TOKEN_EQUAL
        };
        static token default_val = {
            .src = {
                .ref = "1",
                .start = 0,
                .end = 1
            },
            .type = TOKEN_DECLITERAL
        };
        return expression_binary(&equal, expression_literal_ident(assign->label, 1), expression_literal_ident(&default_val, 0));
    }
    return NULL;
}

statement *parse_assignment(parser *parser)
{
    if (match(parser, TOKEN_EOF)) {
        return NULL;
    }
    token *constant = parser->current_token;
    token *equ = NULL;
    statement *stat;
    eat(parser);
    if (!is_eos(parser)) {
        if (constant->type == TOKEN_PLUS || constant->type == TOKEN_HYPHEN) {
            error(parser, equ, "Illegal operation on anonymous label");
            return parse_statement(parser);
        }
        equ = parser->current_token;
        expect(parser, TOKEN_EQUAL);
        stat = statement_create(constant, equ);
        stat->operand = parse_operand(parser, 0);
        if (stat->operand->form != FORM_ZP_ABSOLUTE) {
            error(parser, stat->operand->single_expression.expr->token,
            "Expression expected");
            return parse_statement(parser);
        }
    } else {
        stat = statement_create(constant, equ);
    }
    eos(parser);
    stat->index = parser->statements++;
    return stat;
}

/*stat :: eos? **/
statement *parse_statement(parser *parser)
{
    parser->expect_assignment = 0;
    token *label = NULL;
    token *instruction = NULL;
    if (is_eos(parser)) {
        if (match(parser, TOKEN_EOF)) {
            return NULL;
        }
        eos(parser);
    }
    statement *statement;
    if (match(parser, TOKEN_IDENT) ||
        match(parser, TOKEN_HYPHEN) ||
        match(parser, TOKEN_PLUS) ||
        match(parser, TOKEN_ASTERISK)) {
        if (peek(parser)->type == TOKEN_EQUAL) {
            return parse_assignment(parser);
        }
        label = parser->current_token;
        eat(parser);
        if (match(parser, TOKEN_COLON) && label->type == TOKEN_IDENT) {
            eat(parser);
        }
        if (is_eos(parser) || label->type == TOKEN_ASTERISK) {
            if (label->type == TOKEN_ASTERISK) {
                error(parser, parser->current_token, "Symbol '*' is reserved");
            }
            statement = statement_create(label, NULL);
            goto finish;
        }
    }
    instruction = parser->current_token;
    eat(parser);
    if (instruction->type == TOKEN_ASTERISK && match(parser, TOKEN_EQUAL)) {
        tiny_error(instruction, ERROR_MODE_RECOVER, "Program counter assignment cannot be preceded by a label");
        statement = statement_create(label, NULL);
        goto finish;
    }
    statement = statement_create(label, instruction);
    if ((instruction->type >= TOKEN_ANC && instruction->type <= TOKEN_TOP) ||
        (instruction->type >= TOKEN_BBR && instruction->type <= TOKEN_XCE) ||
        instruction->type >= TOKEN_ADC) {
        if (instruction->type > TOKEN_PSTRING) {
            if (instruction->type == TOKEN_MACRO_NAME) {
                return macro_expand(parser, statement);
            }
            error(parser, instruction, "Mnemonic or directive expected");
            instruction = NULL;
            goto finish;
        }
        if (statement->instruction->type == TOKEN_INCLUDE) {
            return include(parser, statement);
        }
        if (statement->instruction->type == TOKEN_MACRO) {
            return macro_def(parser, statement);
        }
        if (statement->instruction->type <= TOKEN_TYA) {
            token_type mnemonic = statement->instruction->type;
            statement->operand = parse_operand(parser,
                mnemonic == TOKEN_BBR || mnemonic == TOKEN_BBS || mnemonic == TOKEN_RMB || mnemonic == TOKEN_SMB);
        } else {
            int no_operand = token_is_of_type(statement->instruction, pseudo_op_no_operand, sizeof pseudo_op_no_operand);
            if (!is_eos(parser)) {
                if (no_operand) {
                    error(parser, parser->current_token, "Unexpected expression");
                    goto finish;
                }
                statement->operand = operand_expression_list(parse_expr_list(parser));
            } else if (!no_operand) {
                error(parser, parser->current_token, "Expression expected");
            }
        }
        goto finish;
    }
    if (instruction->type == TOKEN_DOT && match(parser, TOKEN_IDENT)) {
        if (parser->current_token->src_line == instruction->src_line &&
            parser->current_token->src_line_pos == instruction->src_line_pos + 1) {
            instruction->src.end = parser->current_token->src.end;
            eat(parser);
            return macro_expand(parser, statement);
        }
    }
    TOKEN_GET_TEXT(instruction, instruction_text);
    error(parser, instruction, "Expected mnemonic or directive but found '%s'", instruction_text);
    instruction = NULL;
finish:
    eos(parser);
    statement->index = parser->statements++;
    return statement;
}

static void macro_destructor(htable_value_ptr macro_ptr)
{
    macro *m = (macro*)macro_ptr;
    macro_destroy(m);
}

void parser_destroy(parser *parser)
{
    if (!parser) return;
    string_htable_destroy(parser->macro_defs);
    dynamic_array_cleanup_and_destroy(parser->token_buffer);
    tiny_free(parser);
}

parser *parser_create(lexer *lexer, int case_sensitive)
{
    parser *parser = tiny_calloc(1, sizeof(struct parser));
    DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(parser->token_buffer, struct token *, 2039);
    parser->macro_defs = string_htable_create(sizeof(macro));
    parser->macro_defs->dtor = macro_destructor;
    parser->macro_defs->case_sensitive = case_sensitive;
    parser->lexer = lexer;
    eat(parser);
    return parser;
}
