/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "error.h"
#include "output.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void output_reset(output *output)
{
    memset(output, 0, sizeof(char) * 0x10000);
    output->start = 0xffff;
    output->end = 0;
    output->logical_pc = output->pc = 0;
}

void output_add(output *output, value value, int size)
{
    output_add_values(output, (const char*)&value, size);
}

void output_fill(output *output, int amount)
{
    if (output->pc + amount > 0x10000) {
        if (output->pc_overflow_handler.callback) {
            output->pc_overflow_handler.callback(output, output->pc_overflow_handler.data);
            return;
        }
        tiny_error(NULL, ERROR_MODE_PANIC, "Program counter overflow.");
    }
    output->logical_pc += amount;
    output->pc += amount;
}

void output_fill_value(output *output, int amount, value value)
{
    char bytes[sizeof(value) + 1] = {};
    memcpy(bytes, &value, sizeof(value));
    size_t size = value_size(value);
    while (amount) {
        if (amount < size) {
            size = amount;
        }
        output_add_values(output, bytes, size);
        amount -= size;
    }
}

void output_add_values(output *output, const char *values, size_t size)
{
    if (output->pc < output->start) {
        output->start = output->pc;
    }
    if (output->pc + size >= 0x10000) {
        if (output->pc_overflow_handler.callback) {
            output->pc_overflow_handler.callback(output, output->pc_overflow_handler.data);
            return;
        }
        tiny_error(NULL, ERROR_MODE_PANIC, "Program counter overflow.");
    }
    memcpy(output->buffer + output->pc, values, size);
    output->pc += size;
    output->logical_pc += size;
    if (output->pc > output->end) {
        output->end = output->pc;
    }
}

void output_set_overflow_handler(output *out, overflow_handler_callback callback, void *user_data)
{
    out->pc_overflow_handler.callback = callback;
    out->pc_overflow_handler.data = user_data;
}
