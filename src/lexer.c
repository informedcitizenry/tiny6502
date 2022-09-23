/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "file.h"
#include "lexer.h"
#include "memory.h"
#include "string_htable.h"
#include "token.h"
#include <ctype.h>
#include <string.h>

#define TOKEN_RESERVED_NUM      TOKEN_TYPE_NUM - TOKEN_NON_RESERVED_NUM - 1

static const char *token_reserved_names[TOKEN_RESERVED_NUM] = {
    "a",
    "x",
    "y",
    "adc",
    "and",
    "asl",
    "bcc",
    "bcs",
    "beq",
    "bit",
    "bmi",
    "bne",
    "bpl",
    "brk",
    "bvc",
    "bvs",
    "clc",
    "cld",
    "cli",
    "clv",
    "cmp",
    "cpx",
    "cpy",
    "dec",
    "dex",
    "dey",
    "eor",
    "inc",
    "inx",
    "iny",
    "jmp",
    "jsr",
    "lda",
    "ldx",
    "ldy",
    "lsr",
    "nop",
    "ora",
    "pha",
    "php",
    "pla",
    "plp",
    "rol",
    "ror",
    "rti",
    "rts",
    "sbc",
    "sec",
    "sed",
    "sei",
    "sta",
    "stx",
    "sty",
    "tax",
    "tay",
    "tsx",
    "txa",
    "txs",
    "tya",
    ".include",
    ".macro",
    ".m8",
    ".m16",
    ".mx8",
    ".mx16",
    ".x8",
    ".x16",
    ".align",
    ".binary",
    ".byte",
    ".word",
    ".dword",
    ".fill",
    ".long",
    ".stringify",
    ".relocate",
    ".endrelocate",
    ".dp",
    ".pron",
    ".proff",
    ".string",
    ".cstring",
    ".lstring",
    ".nstring",
    ".pstring",
    "MACRO_DEFINITION",
    ".end",
    ".endmacro",
};

static const token_type token_reserved_types[TOKEN_RESERVED_NUM] = {
    TOKEN_A,
    TOKEN_X,
    TOKEN_Y,
    TOKEN_ADC,
    TOKEN_AND,
    TOKEN_ASL,
    TOKEN_BCC,
    TOKEN_BCS,
    TOKEN_BEQ,
    TOKEN_BIT,
    TOKEN_BMI,
    TOKEN_BNE,
    TOKEN_BPL,
    TOKEN_BRK,
    TOKEN_BVC,
    TOKEN_BVS,
    TOKEN_CLC,
    TOKEN_CLD,
    TOKEN_CLI,
    TOKEN_CLV,
    TOKEN_CMP,
    TOKEN_CPX,
    TOKEN_CPY,
    TOKEN_DEC,
    TOKEN_DEX,
    TOKEN_DEY,
    TOKEN_EOR,
    TOKEN_INC,
    TOKEN_INX,
    TOKEN_INY,
    TOKEN_JMP,
    TOKEN_JSR,
    TOKEN_LDA,
    TOKEN_LDX,
    TOKEN_LDY,
    TOKEN_LSR,
    TOKEN_NOP,
    TOKEN_ORA,
    TOKEN_PHA,
    TOKEN_PHP,
    TOKEN_PLA,
    TOKEN_PLP,
    TOKEN_ROL,
    TOKEN_ROR,
    TOKEN_RTI,
    TOKEN_RTS,
    TOKEN_SBC,
    TOKEN_SEC,
    TOKEN_SED,
    TOKEN_SEI,
    TOKEN_STA,
    TOKEN_STX,
    TOKEN_STY,
    TOKEN_TAX,
    TOKEN_TAY,
    TOKEN_TSX,
    TOKEN_TXA,
    TOKEN_TXS,
    TOKEN_TYA,
    TOKEN_INCLUDE,
    TOKEN_MACRO,
    TOKEN_M8,
    TOKEN_M16,
    TOKEN_MX8,
    TOKEN_MX16,
    TOKEN_X8,
    TOKEN_X16,
    TOKEN_ALIGN,
    TOKEN_BINARY,
    TOKEN_BYTE,
    TOKEN_WORD,
    TOKEN_DWORD,
    TOKEN_FILL,
    TOKEN_LONG,
    TOKEN_STRINGIFY,
    TOKEN_RELOCATE,
    TOKEN_ENDRELOCATE,
    TOKEN_DP,
    TOKEN_PRON,
    TOKEN_PROFF,
    TOKEN_STRING,
    TOKEN_CSTRING,
    TOKEN_LSTRING,
    TOKEN_NSTRING,
    TOKEN_PSTRING,
    TOKEN_MACRO_NAME,
    TOKEN_EOF,
    TOKEN_ENDMACRO,
};

static const token_type previous_expression_types[] = 
{
    TOKEN_ASTERISK,
    TOKEN_BINLITERAL,
    TOKEN_CHARLITERAL,
    TOKEN_DECLITERAL,
    TOKEN_HEXLITERAL,
    TOKEN_IDENT,
    TOKEN_RCURLY,
    TOKEN_RPAREN,
    TOKEN_RSQUARE,
    TOKEN_STRINGLITERAL
};

typedef struct position_t
{
    
    long position;
    int line_number;
    int line_position;
    
} position;

typedef struct source_file_state
{
    source_file source;
    position curr_position;
    int end_of_file;

} source_file_state;

typedef struct file_stack
{
    int top;
    size_t stack_capacity;
    source_file_state *file_states;

} file_stack;

typedef struct lexer
{
    position start_position;
    position curr_position;
    int end_of_file;
    token *previous_token;
    char *curr_line;
    size_t buffer_len;
    size_t buffer_capacity;
    string_htable *reserved_words;
    source_file source;
    file_stack files;
    dynamic_array *include_files;
    
} lexer;

static void set_newline(lexer *lexer)
{
    if (++lexer->curr_position.line_number >= lexer->source.line_numbers) {
        if (!lexer->files.top) {
            return;
        }
        /* restore previous file state */
        source_file_state *state = lexer->files.file_states + --lexer->files.top;
        lexer->curr_position = state->curr_position;
        lexer->source = state->source;
        if (lexer->curr_position.line_number < lexer->source.line_numbers) {
            lexer->curr_line = lexer->source.lines[lexer->curr_position.line_number];
            lexer->buffer_len = strlen(lexer->curr_line);
            lexer->end_of_file = state->end_of_file;
        }
        return;
    }
    lexer->curr_line = lexer->source.lines[lexer->curr_position.line_number];
    lexer->buffer_len = strlen(lexer->curr_line);
    lexer->curr_position.position = 0;
    lexer->curr_position.line_position = 1;
}

static char get_char(lexer *lexer)
{
    if (lexer->end_of_file){
        return TOKEN_EOF;
    }
    char c = TOKEN_EOF;
    lexer->curr_position.position++;
    if (lexer->curr_position.position >= lexer->buffer_len) {
        set_newline(lexer);
    }
    if (lexer->curr_position.position < lexer->buffer_len) {
        c = lexer->curr_line[lexer->curr_position.position];
    }
    lexer->curr_position.line_position = (int)lexer->curr_position.position + 1;
    lexer->end_of_file = c == '\0';
    return c;
}

static char peek_char(lexer *lexer)
{
    if (lexer->end_of_file) {
        return TOKEN_EOF;
    }
    char p = TOKEN_EOF;
    size_t next = lexer->curr_position.position + 1;
    if (next < lexer->buffer_len) {
        p = lexer->curr_line[next];
    }
    return p;
}

static void cleanup_include_files(void *include_ptr)
{
    source_file *incl = (source_file*)include_ptr;
    source_file_cleanup(incl);
    tiny_free(incl);
}


lexer *lexer_create(const source_file *source, int case_sensitive)
{
    lexer *lex = tiny_calloc(1, sizeof(lexer));
    lex->curr_position.position = -1;
    lex->curr_position.line_number = 0;
    lex->reserved_words = string_htable_create_from_lists(token_reserved_names, (const htable_value_ptr)token_reserved_types, sizeof(token_type), TOKEN_RESERVED_NUM, case_sensitive);
    lex->source = *source;
    lex->curr_line = lex->source.lines[0];
    lex->buffer_len = strlen(lex->curr_line);
    DYNAMIC_ARRAY_CREATE(lex->include_files, source_file);
    lex->include_files->dtor = cleanup_include_files;
    get_char(lex); /* prime the lookahead character */
    return lex;
}

dynamic_array *lexer_include_and_process(lexer *lexer, const source_file *include)
{
    const char *curr_file = lexer->source.file_name;
    size_t curr_file_len = strlen(curr_file);
    lexer_include(lexer, include);
    dynamic_array *included;
    DYNAMIC_ARRAY_CREATE(included, token);
    
    while (strncmp(curr_file, lexer->source.file_name, curr_file_len) != 0) {
        dynamic_array_add(included, next_token(lexer));
    }
    return included;
    
}

void lexer_include(lexer *lexer, const source_file *include)
{
    if (lexer->files.top == lexer->files.stack_capacity) {
        if (!lexer->files.stack_capacity) {
            lexer->files.stack_capacity = 1;
        } else {
            lexer->files.stack_capacity *= 2;
        }
        lexer->files.file_states = tiny_realloc(lexer->files.file_states, lexer->files.stack_capacity * sizeof(source_file_state));
    }
    /* save current file state*/
    source_file_state *state = lexer->files.file_states + lexer->files.top++;
    state->curr_position = lexer->curr_position;
    state->source = lexer->source;
    state->end_of_file = lexer->end_of_file;
    
    /* push new file state */
    lexer->source = *include;
    if (lexer->source.line_numbers) {
        lexer->curr_line = lexer->source.lines[0];
        lexer->buffer_len = strlen(lexer->curr_line);
        lexer->curr_position.line_number = 0;
        lexer->curr_position.position = 0;
        lexer->curr_position.line_position = 1;
        lexer->end_of_file = 0;
    }
    source_file *incl_copy = tiny_malloc(sizeof(source_file));
    *incl_copy = *include;
    dynamic_array_add(lexer->include_files, incl_copy);
}

void lexer_destroy(lexer *lexer)
{
    if (!lexer) return;
    string_htable_destroy(lexer->reserved_words);
    dynamic_array_cleanup_and_destroy(lexer->include_files);
    tiny_free(lexer->files.file_states);
    tiny_free(lexer);
}

static token_type token_type_from_token_text(lexer *lexer, const char *token_text)
{
    htable_entry *entry = string_htable_find_bucket(lexer->reserved_words, token_text);
    if (entry) {
        return (token_type)*entry->value;
    }
    return TOKEN_IDENT;
}

static char current_char(lexer *lexer){
    if (lexer->curr_position.line_number < lexer->source.line_numbers &&
        lexer->curr_position.position < lexer->buffer_len) {
        return lexer->curr_line[lexer->curr_position.position];
    }
    return TOKEN_EOF;
}

static token *create_token(token_type type, lexer *lexer)
{
    token *t = tiny_calloc(1, sizeof(token));
    t->type = type;
    t->src.ref = lexer->curr_line;
    t->src_filename = lexer->source.file_name;
    t->src_line = lexer->start_position.line_number + 1;
    t->src_line_pos = lexer->start_position.line_position;
    t->src.start = (int)lexer->start_position.position;
    t->src.end = (int)lexer->curr_position.position;
    if (lexer->files.top) {
        source_file_state *include = lexer->files.file_states + (lexer->files.top - 1);
        t->include_filename = include->source.file_name;
        t->include_line = include->curr_position.line_number;
    }
    lexer->previous_token = t;
    if (t->type == TOKEN_END) {
        t->type = TOKEN_EOF;
    }
    return t;
}

static void skip_whitespace(lexer *lexer)
{
    char c = current_char(lexer);
    while (isspace(c)){
        if (c == '\n'){
            break;
        }
        c = get_char(lexer);
    }
}

static token *check_plus_hyphen(lexer *lexer)
{
    char c = current_char(lexer);
    char n = get_char(lexer);
    if (n == c) {
        while (n == c) {
            n = get_char(lexer);
        }
        return c == '+' ?
        create_token(TOKEN_MULTIPLUS, lexer) :
        create_token(TOKEN_MULTIHYPHEN, lexer);
    }
    return c == '+' ? create_token(TOKEN_PLUS, lexer) : create_token(TOKEN_HYPHEN, lexer);
}

static token *check_bang(lexer *lexer)
{
    char n = get_char(lexer);
    if (n == '='){
        get_char(lexer);
        return create_token(TOKEN_BANGEQUAL, lexer);
    }
    return create_token(TOKEN_BANG, lexer);
}

static token *check_angles(lexer *lexer)
{
    char c = current_char(lexer);
    char n = get_char(lexer);
    if (n == c) {
        n = get_char(lexer);
        if (c == '<') {
            return create_token(TOKEN_LSHIFT, lexer);
        }
        if (n == c) {
            get_char(lexer);
            return create_token(TOKEN_ARSHIFT, lexer);
        }
        return create_token(TOKEN_RSHIFT, lexer);
    }
    if (n == '=') {
        n = get_char(lexer);
        if (c == '<' && n == '>') {
            get_char(lexer);
            return create_token(TOKEN_SPACESHIP, lexer);
        }
        return c == '<' ?
        create_token(TOKEN_LTE, lexer) :
        create_token(TOKEN_GTE, lexer);
    }
    return c == '<' ? create_token(TOKEN_LANGLE, lexer) : create_token(TOKEN_RANGLE, lexer);
}

static token *check_doubled(lexer *lexer)
{
    char c = current_char(lexer);
    char n = get_char(lexer);
    if (n == c) {
        get_char(lexer);
        switch (c) {
            case '^': return create_token(TOKEN_DOUBLECARET, lexer);
            case '&': return create_token(TOKEN_DOUBLEAMPERSAND, lexer);
            case '=': return create_token(TOKEN_DOUBLEEQUAL, lexer);
            default: return create_token(TOKEN_DOUBLEPIPE, lexer);
        }
    }
    switch (c) {
        case '^': return create_token(TOKEN_CARET, lexer);
        case '&': return create_token(TOKEN_AMPERSAND, lexer);
        case '=': return create_token(TOKEN_EQUAL, lexer);
        default: return create_token(TOKEN_PIPE, lexer);
    }
}

static int is_escape(lexer *lexer)
{
    char c = get_char(lexer);
    if (isdigit(c) && c <= '7') {
        return 1;
    }
    int is_escape;
    if (c == 'U' || c == 'u' || c == 'x'){
        int x = c == 'x';
        int max = x ? 2 : c == 'u' ? 4 : 8;
        int count = 0;
        c = get_char(lexer);
        while (isxdigit(c) && count < max){
            count++;
            c = get_char(lexer);
        }
        lexer->curr_position.position--;
        is_escape = (count == max) || x ? count : 0;
    } else {
        switch (c) {
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'v':
            case '\\':
            case '\'':
            case '"':
                is_escape = 1;
                break;
            default:
                is_escape = 0;
        }
    }
    return is_escape;
}

static token *check_string(lexer *lexer)
{
    char quote = current_char(lexer);
    char c = get_char(lexer);
    position mark = lexer->curr_position;
    int is_quote = 1;
    size_t size = 0;
    while (c != quote){
        if (c == TOKEN_EOF || c == '\n' || size == TOKEN_TEXT_MAX_LEN){
            is_quote = 0;
            break;
        }
        if (c == '\\'){
            int esc = is_escape(lexer);
            if (!esc) {
                is_quote = 0;
                break;
            }
            size += esc;
        }
        size++;
        c = get_char(lexer);
    }
    if (!is_quote){
        lexer->curr_position = mark;
        return create_token(TOKEN_UNRECOGNIZED, lexer);
    }
    get_char(lexer);
    token_type type = quote == '"' ? TOKEN_STRINGLITERAL : TOKEN_CHARLITERAL;
    return create_token(type, lexer);
}


static int is_utf8_alpha(char c)
{
    return isalpha(c) || c < -1;
}

static int is_utf8_alnum(char c)
{
    return isalnum(c) || c < -1;
}

static token *get_ident(lexer *lexer)
{
    /* just long enough to compare to keywords, ID length is much longer. */
    char id_buff[16] = {};
    int i = -1;
    char c = current_char(lexer);
    int iskw = 1;
    if (c == '.' /* from check_dot */) {
        if (!is_utf8_alpha(c = get_char(lexer))) {
            return create_token(TOKEN_DOT, lexer);
        }
        id_buff[++i] = '.';
    }
    while ((is_utf8_alnum(c) || c == '_') && i < TOKEN_TEXT_MAX_LEN){
        iskw &= ++i < 15;
        if (iskw) {
            id_buff[i] = c;
        }
        c = get_char(lexer);
    }
    token_type type = TOKEN_IDENT;
    if (iskw){
        type = token_type_from_token_text(lexer, id_buff);
    }
    return create_token(type, lexer);
}

static token *check_dot(lexer *lexer)
{
    position mark = lexer->curr_position;
    token *t = get_ident(lexer);
    if (t->type == TOKEN_IDENT) {
        lexer->curr_position = mark;
        get_char(lexer);
        t->type = TOKEN_DOT;
    }
    return t;
}

static token *check_backslash(lexer *lexer)
{
    char c = get_char(lexer), n = c;
    if ((!is_utf8_alnum(c) && c != '_') || c == '0') {
        return create_token(TOKEN_UNRECOGNIZED, lexer);
    }
    while (is_utf8_alnum(c) || c == '_') {
        c = get_char(lexer);
    }
    if (is_utf8_alpha(n) || n == '_') {
        return create_token(TOKEN_MACROSUBSTITUTION, lexer);
    }
    return create_token(TOKEN_NUMBEREDSUBSTITUTION, lexer);
}

static int char_is_binary(int c)
{
    if (c == '0' || c == '1') return 1;
    return 0;
}

static int is_numeric(lexer *lexer, int(*fcn)(int))
{
    char c = current_char(lexer);
    int is_numeric = 0;
    while (fcn(c)){
        is_numeric = 1;
        c = get_char(lexer);
        if (c == '_'){
            position mark = lexer->curr_position;
            c = get_char(lexer);
            if (!fcn(c)){
                lexer->curr_position = mark;
                break;
            }
        }
    }
    return is_numeric;
}

static token *get_number(lexer *lexer)
{
    int(*number_fcn)(int);
    char c = current_char(lexer);
    token_type type;
    switch (c) {
        case '$':
            number_fcn = isxdigit;
            type = TOKEN_HEXLITERAL;
            get_char(lexer);
            break;
        case '%':
            number_fcn = char_is_binary;
            type = TOKEN_BINLITERAL;
            get_char(lexer);
            break;
        default:
            number_fcn = isdigit;
            type = TOKEN_DECLITERAL;
            break;
    }
    if (!is_numeric(lexer, number_fcn)) {
        return c == '$' ? create_token(TOKEN_UNRECOGNIZED, lexer) :
               c == '%' ? create_token(TOKEN_PERCENT, lexer) :
        create_token(TOKEN_DECLITERAL, lexer);
    }
    return create_token(type, lexer);
    
}

static token *check_percent(lexer *lexer)
{
    char p = peek_char(lexer);
    if ((p != '0' && p != '1') || token_is_of_type(lexer->previous_token, previous_expression_types, sizeof previous_expression_types)) {
        get_char(lexer);
        return create_token(TOKEN_PERCENT, lexer);
    }
    return get_number(lexer);
}

static token *next_new_line(lexer *lexer)
{
    char c = get_char(lexer);
    while (c != '\n') {
        c = get_char(lexer);
    }
    return next_token(lexer);
}

static token *check_solidus(lexer *lexer)
{
    char c = get_char(lexer);
    if (c == '/') {
        return next_new_line(lexer);
    }
    if (c == '*') {
        position mark = lexer->curr_position;
        c = get_char(lexer);
        while (c != TOKEN_EOF) {
            while (c != '*' && c != TOKEN_EOF) {
                c = get_char(lexer);
            }
            if (c != TOKEN_EOF) {
                c = get_char(lexer);
                if (c == '/') {
                    get_char(lexer);
                    return next_token(lexer);
                }
            }
        }
        lexer->curr_position = mark;
    }
    return create_token(TOKEN_SOLIDUS, lexer);
}

void lexer_add_reserved_word(lexer *lexer, const char *name)
{
    token_type macro_name = TOKEN_MACRO_NAME;
    string_htable_add(lexer->reserved_words, name, (const htable_value_ptr)&macro_name);
}

void lexer_add_reserved_words(lexer *lexer, size_t word_count, const char** word_names, const int *word_values)
{
    string_htable_add_range(lexer->reserved_words, word_count, word_names, (const htable_value_ptr)word_values);
}

static token *get_newline(lexer *lexer)
{
    lexer->curr_position.position++;
    token *t = create_token(TOKEN_NEWLINE, lexer);
    set_newline(lexer);
    return t;
}

int lexer_is_reserved_word(const lexer *lexer, const char *word)
{
    return string_htable_contains(lexer->reserved_words, word);
}

int lexer_is_case_sensitive(const lexer *lexer)
{
    return lexer->reserved_words->case_sensitive;
}

const source_file *lexer_get_source(lexer *lexer)
{
    return &lexer->source;
}

token *next_token(lexer *lexer)
{
    skip_whitespace(lexer);
    char c = current_char(lexer);
    lexer->start_position = lexer->curr_position;
    token_type type = TOKEN_UNRECOGNIZED;
    switch (c){
        case '0' ... '9':
        case '$':
            return get_number(lexer);
        case '%':
            return check_percent(lexer);
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '\x80' ... '\xfe':
        case '_':
            return get_ident(lexer);
        case '.':
            return check_dot(lexer);
        case '\\':
            return check_backslash(lexer);
        case '\'':
        case '"':
            return check_string(lexer);
        case '<':
        case '>':
            return check_angles(lexer);
        case '^':
        case '&':
        case '|':
        case '=':
            return check_doubled(lexer);
        case '!':
            return check_bang(lexer);
        case ';':
            return next_new_line(lexer);
        case '/':
            return check_solidus(lexer);
        case '\n':
            return get_newline(lexer);
        case '\0': 
            return create_token(TOKEN_EOF, lexer);
        case '-':
        case '+': return check_plus_hyphen(lexer);
        case '?': type = TOKEN_QUERY;       break;
        case '*': type = TOKEN_ASTERISK;    break;
        case '(': type = TOKEN_LPAREN;      break;
        case ')': type = TOKEN_RPAREN;      break;
        case '[': type = TOKEN_LSQUARE;     break;
        case ']': type = TOKEN_RSQUARE;     break;
        case '{': type = TOKEN_LCURLY;      break;
        case '}': type = TOKEN_RCURLY;      break;
        case ',': type = TOKEN_COMMA;       break;
        case ':': type = TOKEN_COLON;       break;
        case '~': type = TOKEN_TILDE;       break;
        case '#': type = TOKEN_HASH;        break;
        default: break;
    }
    get_char(lexer); /* if we're here advance the position once */
    return create_token(type, lexer);
}
