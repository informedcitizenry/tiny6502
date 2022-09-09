/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef file_h
#define file_h

#include <ctype.h>

typedef struct source_file
{
    char **lines;
    size_t line_numbers;
    size_t lines_allocated;
    char *file_name;

} source_file;

typedef struct binary_file
{
    int read_success;
    size_t length;
    char *data;
} binary_file;

source_file source_file_read(const char *path);
source_file source_file_from_user_input(void);

void source_file_cleanup(source_file* file);

binary_file binary_file_read(const char *path);

#endif /* file_h */
