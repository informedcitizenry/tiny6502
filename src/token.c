/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "token.h"
#include <stdlib.h>
#include <string.h>

int token_is_of_type(const token *token, const token_type *types, size_t lut_size)
{
    lut_size /= sizeof(token_type);
    token_type type = token->type;
    for(size_t i = 0; i < lut_size; i++) {
        if (types[i] == type) {
            return 1;
        }
    }
    return 0;
}

size_t token_copy_text_to_buffer(const token *token, char *buffer, size_t buffer_len)
{
    if (token->type == TOKEN_EOF || token->src.ref == NULL){
        strncpy(buffer, "<EOF>", 5);
        buffer[5] = '\0';
        return 5;
    }
    if (token->src.ref){
        size_t len = token->src.end - token->src.start;
        if (len >= TOKEN_TEXT_MAX_LEN){
            len = buffer_len - 1;
        }
        strncpy(buffer, token->src.ref + token->src.start, len);
        buffer[len] = '\0';
        return len;
    }
    return 0;
}
