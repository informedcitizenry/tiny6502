/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef output_h
#define output_h

#include "value.h"

typedef struct output output;
typedef void(*overflow_handler_callback)(const struct output*,const void*);

typedef struct overflow_handler
{
    overflow_handler_callback callback;
    const void *data;
} overflow_handler;

typedef struct output
{
    
    char buffer[0x10000];
    overflow_handler pc_overflow_handler;
    int pc;
    int logical_pc;
    int start;
    int end;
    
} output;

void output_reset(output *output);

void output_set_overflow_handler(output *output, overflow_handler_callback callback, void *user_data);

void output_add(output *output, value value, int size);
void output_fill(output *output, int amount);
void output_fill_value(output *output, int amount, value value);
void output_add_values(output *output, const char *values, size_t size);

#endif /* output_h */
