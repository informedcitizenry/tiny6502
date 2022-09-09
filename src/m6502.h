/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef m6502_h
#define m6502_h

typedef struct assembly_context assembly_context;
typedef struct operand operand;
typedef struct token token;

#define M6502I_WORDS 21
#define W65816_WORDS 37
#define W65C02_WORDS 15

extern const char *m6502i_mnemonics[M6502I_WORDS];
extern const int m6502i_types[M6502I_WORDS];

extern const char *w65816_mnemonics[W65816_WORDS];
extern const int w65816_types[W65816_WORDS];

extern const char *w65c02_mnemonics[W65C02_WORDS];
extern const int w65c02_types[W65C02_WORDS];

void m6502_gen(assembly_context *context, const token *mnemonic_token, const operand *operand, char *disassembly);
void set_instruction_set(assembly_context *context);


#endif /* m6502_h */
