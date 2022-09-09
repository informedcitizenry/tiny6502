/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "error.h"
#include "statement.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int MAX_ENTRIES = 1000;

static int errors = 0;
static int warnings = 0;

static void output_error_type(int is_error, error_mode mode)
{
    if (is_error) {
        fprintf(stdout, ERROR_TEXT);
        if (mode == ERROR_MODE_PANIC) {
            fprintf(stdout, "fatal ");
        }
        fprintf(stdout, "error: " DEFAULT_TEXT);
    } else {
        fprintf(stdout, WARNING_TEXT "warning: " DEFAULT_TEXT);
    }
}

static void tiny_log_message(const token *token, int is_error, error_mode mode, const char *msg, va_list ap)
{
    if (errors + warnings >= MAX_ENTRIES) return;
    if (is_error) errors++; else warnings++;
    if (errors + warnings > MAX_ENTRIES) {
        fprintf(stdout, "Too many errors");
        return;
    }
    if (!token || !token->src_filename) {
        output_error_type(is_error, mode);
        vfprintf(stdout, msg, ap);
        return;
    }
    char source_line[LINE_DISPLAY_LEN * 2 + 1] = {};
    statement_get_source_line_from_token(token, source_line);
    char *c = source_line;
    while (*c && *c != '\n') {
        if (*c == '\t') {
            *c = ' ';
        }
        c++;
    }
    while (*c == '\n') {
        c++;
    }
    size_t line_pos = token->src_line_pos - 1;
    while (line_pos--) {
        *c = ' ';
        c++;
    }
    if (token->expanded_macro) {
        char macro_name[64] = {};
        size_t name_len = token_copy_text_to_buffer(token->expanded_macro, macro_name, 64);
        if (name_len > 61) {
            macro_name[60] = macro_name[61] = macro_name[62] = '.';
        }
        fprintf(stdout, "Expanded from macro '%s'(%d):\n", macro_name, token->expanded_macro->src_line+1);
    }
    if (token->include_filename) {
        fprintf(stdout, "Included from %s(%d):\n", token->include_filename, token->include_line+1);
    }
    fprintf(stdout, "%s(%d:%d): ", token->src_filename, token->src_line, token->src_line_pos);
    output_error_type(is_error, mode);
    const char *fmt = "%s.\n%s" HIGHLIGHT_TEXT "%s" DEFAULT_TEXT "\n";
    char formatted[200] = {};
    vsnprintf(formatted, 199, msg, ap);
    fprintf(stdout, fmt, formatted, source_line, "^~~");
}

void tiny_error(const token *token, error_mode mode, const char *error_msg, ...)
{
    va_list ap;
    va_start(ap, error_msg);
    tiny_verror(token, mode, error_msg, ap);
    va_end(ap);
}

void tiny_verror(const token *token, error_mode mode, const char *error_msg, va_list ap)
{
    tiny_log_message(token, 1, mode, error_msg, ap);
    if (mode == ERROR_MODE_PANIC){
        va_end(ap);
        exit(1);
    }
}

void tiny_warn(const token *token, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    tiny_vwarn(token, msg, ap);
    va_end(ap);
}

void tiny_vwarn(const token *token, const char *msg, va_list ap)
{
    tiny_log_message(token, 0, 0, msg, ap);
}

int tiny_error_count()
{
    return errors;
}

int tiny_warn_count()
{
    return warnings;
}

void tiny_reset_errors_warnings()
{
    errors = warnings = 0;
}
