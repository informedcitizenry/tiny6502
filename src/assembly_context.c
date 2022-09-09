/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "assembly_context.h"
#include "anonymous_label.h"
#include "error.h"
#include "memory.h"
#include "output.h"
#include "string_htable.h"
#include "token.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const size_t LINE_LEN = 200UL;

static void first_line_output(char line[LINE_LEN], const char *disasm, const char *src_line, int start_pc, int start_with_pc)
{
    int line_end;
    if (disasm) {
        if (start_with_pc) {
            line_end = snprintf(line + 1, LINE_LEN - 1, "%-22.4x%-17s%s", start_pc, disasm, src_line);
        } else {
            line_end = snprintf(line + 1, LINE_LEN - 1, "%-39s%s", disasm, src_line);
        }
    } else if (src_line) {
        if (start_with_pc) {
            line_end = snprintf(line + 1, LINE_LEN - 1, "%-39.4x%s", start_pc, src_line);
        } else {
            line_end = snprintf(line + 1, LINE_LEN - 1, "%s", src_line);
        }
    } else {
        line_end = snprintf(line + 1, LINE_LEN - 1, "%-32.4x\n", start_pc);
    }
    if (line_end >= LINE_LEN) {
        line[LINE_LEN - 2] = '\n';
        line[LINE_LEN - 1] = '\0';
    }
    if (line_end < LINE_LEN - 2 && line[line_end] != '\n') {
        line[line_end + 1] = '\n';
        line[line_end + 2] = '\0';
    }
}

static void copy_line_to_disassembly(assembly_context *ctx, const char *line)
{
    size_t line_len = strlen(line);
    if (ctx->disassembly_length + line_len >= ctx->disassembly_capacity / 2) {
        ctx->disassembly_capacity *= 2;
        ctx->disassembly = tiny_realloc(ctx->disassembly, ctx->disassembly_capacity);
    }
    strncpy(ctx->disassembly + ctx->disassembly_length, line, line_len);
    ctx->disassembly_length += line_len;
}

static void binary_file_dtor(htable_value_ptr binary_file_ptr)
{
    binary_file *bin = (binary_file*)binary_file_ptr;
    tiny_free(bin->data);
    tiny_free(bin);
}

void assembly_context_add_disasm_opt_pc(assembly_context *ctx, const char *disasm, const char *src_line, char preamble, int start_with_pc)
{
    if (ctx->print_off) {
        return;
    }
    char line[LINE_LEN] = {};
    int logical_start = ctx->logical_start_pc;
    int start_pc = ctx->start_pc;
    if (!ctx->pass_needed && ctx->options.list) {
        line[0] = preamble;
        if (logical_start == ctx->output->logical_pc || start_pc < ctx->output->start || start_pc >= ctx->output->end) {
            first_line_output(line, disasm, src_line, logical_start, start_with_pc);
            copy_line_to_disassembly(ctx, line);
            if (start_pc < ctx->output->start || start_pc >= ctx->output->end) {
                return;
            }
        }
        while (start_pc < ctx->output->pc) {
            size_t bytes = ctx->output->pc - start_pc;
            
            line[0] = preamble;
            if (bytes > 8) bytes = 8;
            if (logical_start == ctx->logical_start_pc) {
                if (disasm && bytes > 4) bytes = 4;
                first_line_output(line, disasm, src_line, logical_start, start_with_pc);
            } else {
                snprintf(line + 1, LINE_LEN - 1, "%-32.4x\n", logical_start);
            }
            char *line_bytes = line+8;
            const char *output_bytes = ctx->output->buffer + start_pc;
            while (bytes-- > 0) {
                char disasm_bytes[3] = {};
                snprintf(disasm_bytes, 3, "%02x", ((int)*output_bytes) & 0xff);
                memcpy(line_bytes, disasm_bytes, 2);
                line_bytes += 3;
                output_bytes++;
            }
            copy_line_to_disassembly(ctx, line);
            start_pc += 8;
            logical_start += 8;
        }
    }
}

void assembly_context_add_disasm(assembly_context *ctx, const char *disasm, const char *src_line, char preamble)
{
    assembly_context_add_disasm_opt_pc(ctx, disasm, src_line, preamble, 1);
}

void assembly_context_reset(assembly_context *ctx)
{
    ctx->pass_needed = 0;
    ctx->logical_start_pc =
    ctx->start_pc = 0;
    ctx->local_label = NULL;
    ctx->m16 = ctx->x16 = 0;
    ctx->page = 0;
    ctx->print_off = 0;
    output_reset(ctx->output);
    anonymous_label_collection_reset(ctx->anonymous_labels_new);
    ctx->disassembly_length = 0;
}

void assembly_context_to_disk(assembly_context *ctx)
{
    int pc = ctx->output->start;
    int end = ctx->output->end;
    if (end > pc) {
        FILE *fp = fopen(ctx->options.output, "w");
        if (!fp) {
            tiny_error(NULL, ERROR_MODE_PANIC, "Unable to output to file '%s'.\n", ctx->options.output);
        }
        int bytes = end - pc;
        printf("\nStart address: $%.4X\nEnd address:   $%.4X\nBytes written: %d\n", ctx->output->start, ctx->output->end, bytes);
        size_t format_len = strlen(ctx->options.format);
        if (strncmp(ctx->options.format, "cbm", format_len) == 0) {
            fputc(pc, fp); fputc(pc/256, fp);
        }
        else if (strncmp(ctx->options.format, "flat", format_len)) {
            fclose(fp);
            tiny_error(NULL, ERROR_MODE_PANIC, "Unknown output file format '%s'.\n", ctx->options.format);
        }
        fwrite(ctx->output->buffer + pc, sizeof(char), bytes, fp);
        fclose(fp);
 
        if (ctx->disassembly && ctx->options.list) {
            ctx->disassembly[ctx->disassembly_length] = '\0';
            fp = fopen(ctx->options.list, "w");
            if (!fp) {
                tiny_warn(NULL, "Could not write disassembly to file '%s'.\n", ctx->options.list);
            } else {
                time_t now;
                time(&now);
                char dt[20] = {};
                strftime(dt, 20, "%F %T", gmtime(&now));
                fprintf(fp, ";; Disassembly of file '%s'\n"
                            ";; Disassembled %s (UTC)\n;; With args:",
                            ctx->options.input,
                            dt);
                for(int i = 1; i < ctx->options.argc; i++) {
                    fprintf(fp, " %s", ctx->options.argv[i]);
                }
                fprintf(fp, "\n\n%s", ctx->disassembly);
                fclose(fp); 
            }
        }
        if (symbol_table_entry_count(ctx->sym_tab) && ctx->options.label) {
            fp = fopen(ctx->options.label, "w");
            
            if (!fp) {
                tiny_warn(NULL, "Warning: Could not report labels to file '%s'.\n", ctx->options.label);
                return;
            }
            char *buffer;
            char *symbol_report = symbol_table_report(ctx->sym_tab, &buffer);
            fputs(symbol_report, fp);
            fclose(fp);
            tiny_free(symbol_report);
        }
    }
}

void assembly_context_destroy(assembly_context *ctx)
{
    symbol_table_destroy(ctx->sym_tab);
    anonymous_label_collection_destroy(ctx->anonymous_labels_new);
    source_file_cleanup(&ctx->source);
    source_file_cleanup(&ctx->options.defines);
    string_htable_destroy(ctx->binary_files);
    tiny_free(ctx->disassembly);
    tiny_free(ctx->output);
    tiny_free(ctx);
}

assembly_context *assembly_context_create(options options)
{
    assembly_context *ctx = tiny_malloc(sizeof(assembly_context));
    ctx->output = tiny_malloc(sizeof(output));
    output_reset(ctx->output);
    ctx->options = options;
    ctx->sym_tab = symbol_table_create(options.case_sensitive);
    ctx->anonymous_labels_new = anonymous_label_make_collection();
    ctx->passes = 0;
    ctx->disassembly_capacity = 4096;
    ctx->disassembly = tiny_calloc(4096, sizeof(char));
    assembly_context_reset(ctx);
    ctx->anonymous_labels_new->add_mode = 1;
    ctx->binary_files = string_htable_create(sizeof(binary_file));
    ctx->binary_files->dtor = binary_file_dtor;
    return ctx;
}
