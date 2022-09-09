/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "file.h"
#include "memory.h"
#include "value.h"
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void source_read_from_stream(FILE *fp, source_file *f)
{
    f->lines_allocated = 100;
    size_t line_count = 0;
    f->lines = tiny_calloc(f->lines_allocated, sizeof(char*));
    for(;;) {
        char *src_line = tiny_malloc(LINE_MAX);
        if (!fgets(src_line, LINE_MAX, fp)) {
            tiny_free(src_line);
            break;
        }
        char *cr = strchr(src_line, '\r');
        if (cr) {
            /* In Windows the line break is CRLF, on macOS it is CR. Mutate to LF;
            */
            *cr = '\n';
            if (*(++cr)) {
                *cr = '\0';
            }
        }
        f->lines[line_count] = src_line;

        if (++line_count >= f->lines_allocated) {
            f->lines_allocated *= 2;
            f->lines = tiny_realloc(f->lines, sizeof(char*)*f->lines_allocated);
        }
    }
    if (line_count) {
        char *final_line = f->lines[line_count - 1];
        if (!strchr(final_line, '\n') && !strchr(final_line, '\r')) {
            size_t last_line_len = strlen(final_line);
            if (last_line_len < LINE_MAX) {
                final_line[last_line_len] = '\n';
                if (last_line_len + 1 < LINE_MAX) {
                    final_line[last_line_len + 1] = '\0';
                }
            }
        }
    }
    f->line_numbers = line_count;
}

binary_file binary_file_read(const char *path)
{
    binary_file f = {};
    FILE *fp = fopen(path, "rb");
    if (fp) {
        f.read_success = 1;
        f.data = tiny_calloc(UINT24_MAX, sizeof(char));
        f.length = fread(f.data, sizeof(char), UINT24_MAX, fp);
        fclose(fp);
    }
    return f;
}

source_file source_file_read(const char *path)
{
    source_file f = {};
    setlocale(LC_CTYPE, "en_US.UTF-8"); /* try to read source file as UTF8 */
    FILE *fp = fopen(path, "r");
    if (fp) {
        f.file_name = strdup(path);
        source_read_from_stream(fp, &f);
        fclose(fp);
    }
    return f;
}

source_file source_file_from_user_input()
{
    source_file f = {
        .file_name = "<user_input>",
        .lines = NULL,
        .line_numbers = 0
    };
    source_read_from_stream(stdin, &f);
    return f;
}

void source_file_cleanup(source_file *file)
{
    if (file->lines) {
        for(size_t i = 0; i < file->line_numbers; i++) {
            tiny_free(file->lines[i]);
        }
        tiny_free(file->lines);
    }
    tiny_free(file->file_name);
    file->lines = NULL;
    file->line_numbers = 0;
}
