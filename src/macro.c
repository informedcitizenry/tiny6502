/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "error.h"
#include "file.h"
#include "lexer.h"
#include "macro.h"
#include "memory.h"
#include "string_htable.h"
#include "token.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>

dynamic_array *macro_expand_macro(const token *pre_expand_label, const token *expand_token, dynamic_array *params, macro *macro, lexer *lex)
{
    dynamic_array *expanded;
    DYNAMIC_ARRAY_CREATE(expanded, token);
    
    token *first_macro_token = (token*)macro->block_tokens->data[0];
    token *last_macro_token = (token*)macro->block_tokens->data[macro->block_tokens->count - 1];

    /* new_source does NOT need to be freed in this function after we are done
       since as we see below it is added to the macro, and will be freed when
       the macro will be cleaned up */
    source_file *new_source = tiny_malloc(sizeof(source_file));
    
    
    new_source->line_numbers = last_macro_token->src_line - first_macro_token->src_line + 1;
    new_source->file_name = strdup(first_macro_token->src_filename);
    new_source->lines = tiny_malloc(sizeof(char*)*new_source->line_numbers);
    
    dynamic_array_add(macro->sources, new_source);
    
    size_t line_index = first_macro_token->src_line - 1;
    char **dest_source = new_source->lines;
    
    size_t src_line_size = strlen(first_macro_token->src.ref);
    char *curr_line = tiny_malloc(sizeof(char)*LINE_MAX+src_line_size+1);
    
    memcpy(curr_line, first_macro_token->src.ref, src_line_size);
    memset(curr_line + src_line_size, '\0', LINE_MAX - src_line_size);
    dest_source[0] = curr_line;
    int substitution_offset = 0;
    
    if (pre_expand_label) {
        /* inject label at start of expansion */
        token *label = tiny_malloc(sizeof(token));
        *label = *pre_expand_label;
        
        substitution_offset = (int)(label->src.end - label->src.start);

        memmove(curr_line + label->src.end, curr_line, strlen(curr_line));
        memcpy(curr_line, label->src.ref + label->src.start, substitution_offset);
        
        label->src.ref = curr_line;
        dynamic_array_add(expanded, label);
    }
    for(size_t m_ix = 0; m_ix < macro->block_tokens->count; m_ix++) {
        token *t = macro->block_tokens->data[m_ix];
        if (t->src_line - 1 != line_index) {
            substitution_offset = 0;
            line_index = t->src_line - 1;
            src_line_size = strlen(t->src.ref);
            curr_line = tiny_malloc(sizeof(char)*LINE_MAX);
            memcpy(curr_line, t->src.ref, src_line_size);
            memset(curr_line + src_line_size, '\0', LINE_MAX - src_line_size);
            *(++dest_source) = curr_line;
        }
        if (t->type == TOKEN_MACROSUBSTITUTION ||
            t->type == TOKEN_NUMBEREDSUBSTITUTION) {
            long p_ix = -1;
            if (t->type == TOKEN_NUMBEREDSUBSTITUTION) {
                p_ix = strtol(t->src.ref + t->src.start + 1, NULL, 10);
                if (p_ix > params->count) {
                    tiny_error(t, ERROR_MODE_RECOVER, "Required parameter %d missing in macro call", p_ix);
                    continue;
                }
            } else {
                TOKEN_GET_TEXT(t, subname);
                if (string_htable_contains(macro->arg_names, subname + 1)) {
                    p_ix = *(long*)string_htable_get(macro->arg_names, subname + 1);
                } else {
                    tiny_error(t, ERROR_MODE_RECOVER, "No argument matches '%s' macro substitution", subname + 1);
                    continue;
                }
            }
            if (--p_ix < 0 || p_ix > params->count) {
                tiny_error(t, ERROR_MODE_RECOVER, "Required argument missing in macro call");
                continue;
            } else {
                /* get token bounds for substitution */
                dynamic_array *param_tokens = (dynamic_array*)params->data[p_ix];
                token *first_param_token = (token*)param_tokens->data[0];
                token *last_param_token = (token*)param_tokens->data[param_tokens->count - 1];
                
                /* calculate size of substitution */
                size_t subst_size = last_param_token->src.end - first_param_token->src.start;
                size_t t_size = t->src.end - t->src.start;
                char *trail = t->src.ref + t->src.end;
                size_t trail_size = strlen(trail);
                
                /* replace the "\<sub>" in the source with the substituted string of tokens */
                memmove(curr_line + t->src.start + subst_size + substitution_offset, trail, trail_size);
                memcpy(curr_line + t->src.start + substitution_offset, first_param_token->src.ref + first_param_token->src.start, subst_size);
                
                for(size_t p = 0; p < param_tokens->count; p++) {
                    token *repl_token = tiny_malloc(sizeof(token));
                    token *parm_token = (token*)param_tokens->data[p];
                    *repl_token = *parm_token;
                    repl_token->expanded_macro = expand_token;
                    repl_token->src.ref = curr_line;
                    repl_token->src.start = t->src.start + substitution_offset + parm_token->src.start - first_param_token->src.start;
                    repl_token->src.end = repl_token->src.start + parm_token->src.end - parm_token->src.start;
                    repl_token->src_line_pos = (int)repl_token->src.start + 1;
                    dynamic_array_add(expanded, repl_token);
                }
                curr_line[t->src.start + substitution_offset + subst_size + trail_size] = '\0';
                substitution_offset += (int)subst_size - (int)t_size;
            }
        } else if (t->type == TOKEN_INCLUDE && m_ix < macro->block_tokens->count - 1) {
            t = macro->block_tokens->data[++m_ix];
            char incname[TOKEN_TEXT_MAX_LEN] = {};
            size_t len = token_copy_text_to_buffer(t, incname, TOKEN_TEXT_MAX_LEN);
            incname[len] = '\0';
            source_file sf = source_file_read(incname + 1);
            lexer_include(lex, &sf);
            do {
                token *inc_t = next_token(lex);
                if (inc_t->type == TOKEN_NEWLINE) {
                    sf.lines--;
                }
                if (inc_t->type == TOKEN_EOF) {
                    break;
                }
                inc_t->expanded_macro = expand_token;
                dynamic_array_add(expanded, inc_t);
            } while (sf.lines);
        } else {
            token *t_copy = tiny_malloc(sizeof(token));
            *t_copy = *t;
            t_copy->expanded_macro = expand_token;
            t_copy->src.ref = curr_line;
            t_copy->src.start += substitution_offset;
            t_copy->src.end = t_copy->src.start + (t->src.end - t->src.start);
            t_copy->src_line_pos += substitution_offset;
            dynamic_array_add(expanded, t_copy);
        }
    }
    token *nl = tiny_malloc(sizeof(token));
    nl->type = TOKEN_NEWLINE;
    nl->src = ((token*)expanded->data[expanded->count - 1])->src;
    nl->src.end++;
    nl->src.start = nl->src.end - 1;
    if (nl->src.start < LINE_MAX) {
        if (nl->src.end < LINE_MAX) {
            nl->src.ref[nl->src.end] = '\0';
        }
    }
    nl->expanded_macro = expand_token;
    dynamic_array_add(expanded, nl);
    return expanded;
}

static void source_file_dtor(void *source_file_ptr)
{
    source_file_cleanup((source_file*)source_file_ptr);
    tiny_free(source_file_ptr);
}

macro *macro_create(string_htable *arg_names, dynamic_array *block_tokens)
{
    macro *m = tiny_malloc(sizeof(macro));
    m->arg_names = arg_names;
    m->block_tokens = block_tokens;
    DYNAMIC_ARRAY_CREATE(m->sources, typeof(source_file));
    m->sources->dtor = source_file_dtor;
    return m;
}

void macro_destroy(macro *macro)
{
    string_htable_destroy(macro->arg_names);
    dynamic_array_destroy(macro->block_tokens);
    dynamic_array_cleanup_and_destroy(macro->sources);
    tiny_free(macro);
}
