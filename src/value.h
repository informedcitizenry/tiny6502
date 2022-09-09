/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef value_h
#define value_h

#include <ctype.h>
#include <stdint.h>

typedef int64_t value;

#define INT24_MIN   (-8388608)
#define INT24_MAX   8388607
#define UINT24_MAX  16777215

extern const value VALUE_UNDEFINED;

size_t value_size(value val);

#endif /* value_h */
