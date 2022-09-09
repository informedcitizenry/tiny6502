/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "string_view.h"
#include <string.h>

string_view string_view_from_string(char *ref) {
    size_t str_len = strlen(ref);
    string_view sv = {
        .ref = ref,
        .is_dynamic = 0,
        .start = 0,
        .end = str_len
    };
    return sv;
}
