/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "value.h"

const value VALUE_UNDEFINED = INT64_MIN;

size_t value_size(value val)
{
    if (val < INT32_MIN || val > UINT32_MAX) return sizeof(value);
    if (val < INT24_MIN || val > UINT24_MAX) return 4;
    if (val < INT16_MIN || val > UINT16_MAX) return 3;
    if (val < INT8_MIN  || val > UINT8_MAX)  return 2;
    return 1;
}
