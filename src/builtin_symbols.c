/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "builtin_symbols.h"
#include "string_htable.h"
#include "value.h"
#include <stdlib.h>

#define BUILTIN_NUMBER  22

static const char *builtin_symbol_names[BUILTIN_NUMBER] = 
{
    "CURRENT_PASS",
    "false",
    "true",
    "MATH_E",
    "MATH_PI",
    "MATH_TAU",
    "INT8_MAX",
    "INT8_MIN",
    "INT16_MAX",
    "INT16_MIN",
    "INT24_MAX",
    "INT24_MIN",
    "INT32_MAX",
    "INT32_MIN",
    "UINT8_MAX",
    "UINT8_MIN",
    "UINT16_MAX",
    "UINT16_MIN",
    "UINT24_MAX",
    "UINT24_MIN",
    "UINT32_MAX",
    "UINT32_MIN"
};

static const value builtin_symbol_values[BUILTIN_NUMBER] = 
{
    1,
    0,
    1,
    2,
    3,
    6,
    INT8_MAX,
    INT8_MIN,
    INT16_MAX,
    INT16_MIN,
    8388607,
    -8388608,
    INT32_MAX,
    INT32_MIN,
    UINT8_MAX,
    0,
    UINT16_MAX,
    0,
    16777215,
    0,
    UINT32_MAX,
    0
};

string_htable *BUILTIN_SYMBOL_TABLE = NULL;

void builtin_init(int case_sensitive)
{
    if (!BUILTIN_SYMBOL_TABLE) {
        BUILTIN_SYMBOL_TABLE = string_htable_create_from_lists(builtin_symbol_names, (const htable_value_ptr)builtin_symbol_values, sizeof(value), BUILTIN_NUMBER, case_sensitive);
    }
}

void builtin_cleanup()
{
    if (BUILTIN_SYMBOL_TABLE) {
        string_htable_destroy(BUILTIN_SYMBOL_TABLE);
    }
    BUILTIN_SYMBOL_TABLE = NULL;
}
