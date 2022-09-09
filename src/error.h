/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef error_h
#define error_h

#include <stdarg.h>

#define ERROR_TEXT     "\33[31m"
#define HIGHLIGHT_TEXT "\33[92m"
#define WARNING_TEXT   "\33[95m"
#define DEFAULT_TEXT   "\33[0m"

typedef struct token token;

typedef enum error_mode {
    ERROR_MODE_RECOVER, ERROR_MODE_PANIC
} error_mode;

void tiny_error(const token *token, error_mode mode, const char *error_msg, ...);
void tiny_verror(const token *token, error_mode mode, const char *error_msg, va_list ap);
int tiny_error_count(void);

void tiny_warn(const token *token, const char *msg, ...);
void tiny_vwarn(const token *token, const char *msg, va_list ap);
int tiny_warn_count(void);

void tiny_reset_errors_warnings(void);

#endif /* error_h */
