/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef assembly_context_h
#define assembly_context_h

#include "file.h"
#include "m6502.h"
#include "options.h"
#include "symbol_table.h"
#include <ctype.h>

typedef struct output output;
typedef struct token token;
typedef struct anonymous_label_collection anonymous_label_collection;
typedef struct string_htable string_htable;

typedef struct assembly_context
{
    
    source_file source;
    int logical_start_pc;
    int start_pc;
    int passes;
    int pass_needed;
    int m16;
    int x16;
    int page;
    output *output;
    char *disassembly;
    size_t disassembly_length;
    size_t disassembly_capacity;
    int print_off;
    symbol_table *sym_tab;
    struct options options;
    anonymous_label_collection *anonymous_labels_new;
    token *local_label;
    string_htable *binary_files;
    
} assembly_context;

assembly_context *assembly_context_create(options options);
void assembly_context_reset(assembly_context *ctx);
void assembly_context_destroy(assembly_context *ctx);

void assembly_context_add_disasm_opt_pc(assembly_context *ctx, const char *disasm, const char *src_line, char preamble, int start_with_pc);
void assembly_context_add_disasm(assembly_context *ctx, const char *disasm, const char *src_line, char preamble);
void assembly_context_to_disk(assembly_context *ctx);

#endif /* assembly_context_h */
