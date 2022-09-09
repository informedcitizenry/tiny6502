/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "assembly_context.h"
#include "error.h"
#include "evaluator.h"
#include "expression.h"
#include "m6502.h"
#include "output.h"
#include "operand.h"
#include "statement.h"
#include "token.h"
#include "value.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODE_HAS_FLAG(m, f) ((m & f) != 0)


typedef enum addressing_mode {
    
    ADDR_MODE_IMPLIED   = 0x00000000000000,
    ADDR_MODE_ZP        = 0x00000000000001,
    ADDR_MODE_ABS_FLAG  = 0x00000000000010,
    ADDR_MODE_LNG_FLAG  = 0x00000000000100,
    ADDR_MODE_S         = 0x00000000001000,
    ADDR_MODE_X         = 0x00000000010000,
    ADDR_MODE_Y         = 0x00000000100000,
    ADDR_MODE_IND_FLAG  = 0x00000001000000,
    ADDR_MODE_DIR_FLAG  = 0x00000010000000,
    ADDR_MODE_IMM_FLAG  = 0x00000100000000,
    ADDR_MODE_REL_FLAG  = 0x00001000000000,
    ADDR_MODE_TWO_FLAG  = 0x00010000000000,
    ADDR_MODE_ACCUM     = 0x00100000000000,
    ADDR_MODE_BIT_FLAG  = 0x01000000000000,
    ADDR_MODE_ILLEGAL   = 0x10000000000000,
    ADDR_MODE_ABSOLUTE  = ADDR_MODE_ABS_FLAG | ADDR_MODE_ZP,
    ADDR_MODE_LONG      = ADDR_MODE_ABSOLUTE | ADDR_MODE_LNG_FLAG,
    ADDR_MODE_ZP_S      = ADDR_MODE_S | ADDR_MODE_ZP,
    ADDR_MODE_ZP_X      = ADDR_MODE_X | ADDR_MODE_ZP,
    ADDR_MODE_ZP_Y      = ADDR_MODE_Y | ADDR_MODE_ZP,
    ADDR_MODE_ABS_X     = ADDR_MODE_X | ADDR_MODE_ABSOLUTE,
    ADDR_MODE_ABS_Y     = ADDR_MODE_Y | ADDR_MODE_ABSOLUTE,
    ADDR_MODE_LONG_X    = ADDR_MODE_X | ADDR_MODE_LONG,
    ADDR_MODE_IND_ZP    = ADDR_MODE_IND_FLAG | ADDR_MODE_ZP,
    ADDR_MODE_IND_ZP_S  = ADDR_MODE_IND_FLAG | ADDR_MODE_ZP_S,
    ADDR_MODE_IND_ZP_X  = ADDR_MODE_IND_FLAG | ADDR_MODE_ZP_X,
    ADDR_MODE_IND_ZP_Y  = ADDR_MODE_IND_FLAG | ADDR_MODE_ZP_Y,
    ADDR_MODE_INDIRECT  = ADDR_MODE_IND_FLAG | ADDR_MODE_ABSOLUTE,
    ADDR_MODE_IND_ABS_X = ADDR_MODE_X | ADDR_MODE_INDIRECT,
    ADDR_MODE_DIRECT    = ADDR_MODE_DIR_FLAG | ADDR_MODE_ZP,
    ADDR_MODE_DIR_ZP_Y  = ADDR_MODE_DIR_FLAG | ADDR_MODE_ZP_Y,
    ADDR_MODE_IMMEDIATE = ADDR_MODE_IMM_FLAG | ADDR_MODE_ZP,
    ADDR_MODE_IMM_ABS   = ADDR_MODE_IMM_FLAG | ADDR_MODE_ABSOLUTE,
    ADDR_MODE_RELATIVE  = ADDR_MODE_REL_FLAG | ADDR_MODE_ZP,
    ADDR_MODE_REL_ABS   = ADDR_MODE_REL_FLAG | ADDR_MODE_ABSOLUTE,
    ADDR_MODE_TWO_OPER  = ADDR_MODE_TWO_FLAG | ADDR_MODE_ABSOLUTE,
    ADDR_MODE_BIT_ZP    = ADDR_MODE_BIT_FLAG | ADDR_MODE_ZP,
    ADDR_MODE_BIT_OFS   = ADDR_MODE_BIT_FLAG | ADDR_MODE_TWO_FLAG | ADDR_MODE_REL_FLAG
    
} addressing_mode;

enum {
    MODES_IMP,
    MODES_ZIP,
    MODES_IMM,
    MODES_IMM_ABS,
    MODES_ZPS,
    MODES_ZPX,
    MODES_ZPY,
    MODES_ABS,
    MODES_ABSX,
    MODES_ABSY,
    MODES_LONG,
    MODES_LONG_X,
    MODES_IND_ZP,
    MODES_INDS,
    MODES_INDX,
    MODES_INDY,
    MODES_IND_ABS,
    MODES_IND_ABS_X,
    MODES_DIR,
    MODES_DIR_Y,
    MODES_ACC,
    MODES_REL,
    MODES_REL_ABS,
    MODES_TWO_OPS,
    MODES_BIT,
    MODES_BIT_OFFS,
    MODES_ALL,
    BAD =-1
};

const char *m6502i_mnemonics[M6502I_WORDS] =
{
    "anc",
    "ane",
    "arr",
    "asr",
    "dcp",
    "dop",
    "isb",
    "jam",
    "las",
    "lax",
    "rla",
    "rra",
    "sax",
    "sha",
    "shx",
    "shy",
    "slo",
    "sre",
    "stp",
    "tas",
    "top"
};

const int m6502i_types[M6502I_WORDS] =
{
    TOKEN_ANC,
    TOKEN_ANE,
    TOKEN_ARR,
    TOKEN_ASR,
    TOKEN_DCP,
    TOKEN_DOP,
    TOKEN_ISB,
    TOKEN_JAM,
    TOKEN_LAS,
    TOKEN_LAX,
    TOKEN_RLA,
    TOKEN_RRA,
    TOKEN_SAX,
    TOKEN_SHA,
    TOKEN_SHX,
    TOKEN_SHY,
    TOKEN_SLO,
    TOKEN_SRE,
    TOKEN_STP_I,
    TOKEN_TAS,
    TOKEN_TOP
};

const char *w65816_mnemonics[W65816_WORDS] =
{
    "s",
    "bra",
    "brl",
    "cop",
    "jml",
    "jsl",
    "mvn",
    "mvp",
    "pea",
    "pei",
    "per",
    "phb",
    "phd",
    "phk",
    "phx",
    "phy",
    "plb",
    "pld",
    "plx",
    "ply",
    "rep",
    "rtl",
    "sep",
    "stp",
    "stz",
    "tcd",
    "tcs",
    "tdc",
    "trb",
    "tsb",
    "tsc",
    "txy",
    "tyx",
    "wai",
    "wdm",
    "xba",
    "xce"
};

const int w65816_types[W65816_WORDS] =
{
    TOKEN_S,
    TOKEN_BRA,
    TOKEN_BRL,
    TOKEN_COP,
    TOKEN_JML,
    TOKEN_JSL,
    TOKEN_MVN,
    TOKEN_MVP,
    TOKEN_PEA,
    TOKEN_PEI,
    TOKEN_PER,
    TOKEN_PHB,
    TOKEN_PHD,
    TOKEN_PHK,
    TOKEN_PHX,
    TOKEN_PHY,
    TOKEN_PLB,
    TOKEN_PLD,
    TOKEN_PLX,
    TOKEN_PLY,
    TOKEN_REP,
    TOKEN_RTL,
    TOKEN_SEP,
    TOKEN_STP,
    TOKEN_STZ,
    TOKEN_TCD,
    TOKEN_TCS,
    TOKEN_TDC,
    TOKEN_TRB,
    TOKEN_TSB,
    TOKEN_TSC,
    TOKEN_TXY,
    TOKEN_TYX,
    TOKEN_WAI,
    TOKEN_WDM,
    TOKEN_XBA,
    TOKEN_XCE
};

const char *w65c02_mnemonics[W65C02_WORDS] =
{
    "bbr",
    "bbs",
    "bra",
    "brl",
    "phx",
    "phy",
    "plx",
    "ply",
    "rmb",
    "smb",
    "stp",
    "stz",
    "trb",
    "tsb",
    "wai"
};

const int w65c02_types[W65C02_WORDS] =
{
    TOKEN_BBR,
    TOKEN_BBS,
    TOKEN_BRA,
    TOKEN_BRL,
    TOKEN_PHX,
    TOKEN_PHY,
    TOKEN_PLX,
    TOKEN_PLY,
    TOKEN_RMB,
    TOKEN_SMB,
    TOKEN_STP,
    TOKEN_STZ,
    TOKEN_TRB,
    TOKEN_TSB,
    TOKEN_WAI
};

static token_type acc_mnemonics[] = 
{
    TOKEN_ADC,
    TOKEN_AND,
    TOKEN_CMP,
    TOKEN_EOR,
    TOKEN_LDA,
    TOKEN_ORA,
    TOKEN_SBC
};

static token_type ix_mnemonics[] = 
{
    TOKEN_CPX,
    TOKEN_CPY,
    TOKEN_LDX,
    TOKEN_LDY
};

static token_type jmp_mnemonics[] =
{
    TOKEN_JML,
    TOKEN_JMP,
    TOKEN_JSL,
    TOKEN_JSR
};

static unsigned long modes_map[] = {
    ADDR_MODE_IMPLIED,
    ADDR_MODE_ZP,
    ADDR_MODE_IMMEDIATE,
    ADDR_MODE_IMM_ABS,
    ADDR_MODE_ZP_S,
    ADDR_MODE_ZP_X,
    ADDR_MODE_ZP_Y,
    ADDR_MODE_ABSOLUTE,
    ADDR_MODE_ABS_X,
    ADDR_MODE_ABS_Y,
    ADDR_MODE_LONG,
    ADDR_MODE_LONG_X,
    ADDR_MODE_IND_ZP,
    ADDR_MODE_IND_ZP_S,
    ADDR_MODE_IND_ZP_X,
    ADDR_MODE_IND_ZP_Y,
    ADDR_MODE_INDIRECT,
    ADDR_MODE_IND_ABS_X,
    ADDR_MODE_DIRECT,
    ADDR_MODE_DIR_ZP_Y,
    ADDR_MODE_ACCUM,
    ADDR_MODE_RELATIVE,
    ADDR_MODE_REL_ABS,
    ADDR_MODE_TWO_OPER,
    ADDR_MODE_BIT_ZP,
    ADDR_MODE_BIT_OFS
};

static int map_6502[][MODES_ALL] = 
{/*        IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
/* bra */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* brl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cop */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jml */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jsl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* mvn */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* mvp */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pea */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pei */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* per */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phd */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phk */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phx */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phy */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pld */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plx */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ply */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rep */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rmb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rtl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sep */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* smb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stp */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stz */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tcd */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tcs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tdc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* trb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txy */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tyx */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* wai */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* wdm */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* xba */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* xce */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* adc */{ BAD, 0x65, 0x69,  BAD,  BAD, 0x75,  BAD, 0x6d, 0x7d, 0x79,  BAD,  BAD,  BAD,  BAD, 0x61, 0x71,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* and */{ BAD, 0x25, 0x29,  BAD,  BAD, 0x35,  BAD, 0x2d, 0x3d, 0x39,  BAD,  BAD,  BAD,  BAD, 0x21, 0x31,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* asl */{0x0a, 0x06,  BAD,  BAD,  BAD, 0x16,  BAD, 0x0e, 0x1e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x0a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bcc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x90,  BAD,  BAD,  BAD,  BAD},
/* bcs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xb0,  BAD,  BAD,  BAD,  BAD},
/* beq */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xf0,  BAD,  BAD,  BAD,  BAD},
/* bit */{ BAD, 0x24,  BAD,  BAD,  BAD,  BAD,  BAD, 0x2c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bmi */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x30,  BAD,  BAD,  BAD,  BAD},
/* bne */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xd0,  BAD,  BAD,  BAD,  BAD},
/* bpl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x10,  BAD,  BAD,  BAD,  BAD},
/* brk */{0x00,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bvc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x50,  BAD,  BAD,  BAD,  BAD},
/* bvs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x70,  BAD,  BAD,  BAD,  BAD},
/* clc */{0x18,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cld */{0xd8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cli */{0x58,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* clv */{0xb8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cmp */{ BAD, 0xc5, 0xc9,  BAD,  BAD, 0xd5,  BAD, 0xcd, 0xdd, 0xd9,  BAD,  BAD,  BAD,  BAD, 0xc1, 0xd1,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cpx */{ BAD, 0xe4, 0xe0,  BAD,  BAD,  BAD,  BAD, 0xec,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cpy */{ BAD, 0xc4, 0xc0,  BAD,  BAD,  BAD,  BAD, 0xcc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dec */{ BAD, 0xc6,  BAD,  BAD,  BAD, 0xd6,  BAD, 0xce, 0xde,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dex */{0xca,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dey */{0x88,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* eor */{ BAD, 0x45, 0x49,  BAD,  BAD, 0x55,  BAD, 0x4d, 0x5d, 0x59,  BAD,  BAD,  BAD,  BAD, 0x41, 0x51,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* inc */{ BAD, 0xe6,  BAD,  BAD,  BAD, 0xf6,  BAD, 0xee, 0xfe,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* inx */{0xe8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* iny */{0xc8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jmp */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x4c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  0x6c, BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jsr */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x20,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* lda */{ BAD, 0xa5, 0xa9,  BAD,  BAD, 0xb5,  BAD, 0xad, 0xbd, 0xb9,  BAD,  BAD,  BAD,  BAD, 0xa1, 0xb1,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ldx */{ BAD, 0xa6, 0xa2,  BAD,  BAD,  BAD, 0xb6, 0xae,  BAD, 0xbe,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ldy */{ BAD, 0xa4, 0xa0,  BAD,  BAD, 0xb4,  BAD, 0xac, 0xbc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* lsr */{0x4a, 0x46,  BAD,  BAD,  BAD, 0x56,  BAD, 0x4e, 0x5e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x4a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* nop */{0xea,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ora */{ BAD, 0x05, 0x09,  BAD,  BAD, 0x15,  BAD, 0x0d, 0x1d, 0x19,  BAD,  BAD,  BAD,  BAD, 0x01, 0x11,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pha */{0x48,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* php */{0x08,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pla */{0x68,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plp */{0x28,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rol */{0x2a, 0x26,  BAD,  BAD,  BAD, 0x36,  BAD, 0x2e, 0x3e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x2a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ror */{0x6a, 0x66,  BAD,  BAD,  BAD, 0x76,  BAD, 0x6e, 0x7e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x6a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rti */{0x40,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rts */{0x60,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sbc */{ BAD, 0xe5, 0xe9,  BAD,  BAD, 0xf5,  BAD, 0xed, 0xfd, 0xf9,  BAD,  BAD,  BAD,  BAD, 0xe1, 0xf1,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sec */{0x38,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sed */{0xf8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sei */{0x78,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sta */{ BAD, 0x85,  BAD,  BAD,  BAD, 0x95,  BAD, 0x8d, 0x9d, 0x99,  BAD,  BAD,  BAD,  BAD, 0x81, 0x91,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stx */{ BAD, 0x86,  BAD,  BAD,  BAD,  BAD, 0x96, 0x8e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sty */{ BAD, 0x84,  BAD,  BAD,  BAD, 0x94,  BAD, 0x8c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tax */{0xaa,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tay */{0xa8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsx */{0xba,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txa */{0x8a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txs */{0x9a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tya */{0x98,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/*         IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
};

static int map_65c02[][MODES_ALL] = 
{/*        IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
/* bra */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x80,  BAD,  BAD,  BAD,  BAD},
/* brl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cop */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jml */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jsl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* mvn */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* mvp */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pea */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pei */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* per */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phd */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phk */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phx */{0xda,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phy */{0x5a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pld */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plx */{0xfa,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ply */{0x7a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rep */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rmb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rtl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sep */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* smb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stp */{0xdb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stz */{ BAD, 0x64,  BAD,  BAD,  BAD, 0x74,  BAD, 0x9c, 0x9e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tcd */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tcs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tdc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* trb */{ BAD, 0x14,  BAD,  BAD,  BAD,  BAD,  BAD, 0x1c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsb */{ BAD, 0x04,  BAD,  BAD,  BAD,  BAD,  BAD, 0x0c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txy */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tyx */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* wai */{0xcb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* wdm */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* xba */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* xce */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* adc */{ BAD, 0x65, 0x69,  BAD,  BAD, 0x75,  BAD, 0x6d, 0x7d, 0x79,  BAD,  BAD, 0x72,  BAD, 0x61, 0x71,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* and */{ BAD, 0x25, 0x29,  BAD,  BAD, 0x35,  BAD, 0x2d, 0x3d, 0x39,  BAD,  BAD, 0x32,  BAD, 0x21, 0x31,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* asl */{0x0a, 0x06,  BAD,  BAD,  BAD, 0x16,  BAD, 0x0e, 0x1e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x0a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bcc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x90,  BAD,  BAD,  BAD,  BAD},
/* bcs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xb0,  BAD,  BAD,  BAD,  BAD},
/* beq */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xf0,  BAD,  BAD,  BAD,  BAD},
/* bit */{ BAD, 0x24, 0x89,  BAD,  BAD, 0x34,  BAD, 0x2c, 0x3c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bmi */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x30,  BAD,  BAD,  BAD,  BAD},
/* bne */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xd0,  BAD,  BAD,  BAD,  BAD},
/* bpl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x10,  BAD,  BAD,  BAD,  BAD},
/* brk */{0x00,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bvc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x50,  BAD,  BAD,  BAD,  BAD},
/* bvs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x70,  BAD,  BAD,  BAD,  BAD},
/* clc */{0x18,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cld */{0xd8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cli */{0x58,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* clv */{0xb8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cmp */{ BAD, 0xc5, 0xc9,  BAD,  BAD, 0xd5,  BAD, 0xcd, 0xdd, 0xd9,  BAD,  BAD, 0xd2,  BAD, 0xc1, 0xd1,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cpx */{ BAD, 0xe4, 0xe0,  BAD,  BAD,  BAD,  BAD, 0xec,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cpy */{ BAD, 0xc4, 0xc0,  BAD,  BAD,  BAD,  BAD, 0xcc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dec */{0x3a, 0xc6,  BAD,  BAD,  BAD, 0xd6,  BAD, 0xce, 0xde,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x3a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dex */{0xca,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dey */{0x88,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* eor */{ BAD, 0x45, 0x49,  BAD,  BAD, 0x55,  BAD, 0x4d, 0x5d, 0x59,  BAD,  BAD, 0x52,  BAD, 0x41, 0x51,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* inc */{0x1a, 0xe6,  BAD,  BAD,  BAD, 0xf6,  BAD, 0xee, 0xfe,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x1a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* inx */{0xe8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* iny */{0xc8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jmp */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x4c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x6c, 0x7c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jsr */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x20,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* lda */{ BAD, 0xa5, 0xa9,  BAD,  BAD, 0xb5,  BAD, 0xad, 0xbd, 0xb9,  BAD,  BAD, 0xb2,  BAD, 0xa1, 0xb1,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ldx */{ BAD, 0xa6, 0xa2,  BAD,  BAD,  BAD, 0xb6, 0xae,  BAD, 0xbe,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ldy */{ BAD, 0xa4, 0xa0,  BAD,  BAD, 0xb4,  BAD, 0xac, 0xbc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* lsr */{0x4a, 0x46,  BAD,  BAD,  BAD, 0x56,  BAD, 0x4e, 0x5e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x4a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* nop */{0xea,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ora */{ BAD, 0x05, 0x09,  BAD,  BAD, 0x15,  BAD, 0x0d, 0x1d, 0x19,  BAD,  BAD, 0x12,  BAD, 0x01, 0x11,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pha */{0x48,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* php */{0x08,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pla */{0x68,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plp */{0x28,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rol */{0x2a, 0x26,  BAD,  BAD,  BAD, 0x36,  BAD, 0x2e, 0x3e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x2a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ror */{0x6a, 0x66,  BAD,  BAD,  BAD, 0x76,  BAD, 0x6e, 0x7e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x6a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rti */{0x40,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rts */{0x60,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sbc */{ BAD, 0xe5, 0xe9,  BAD,  BAD, 0xf5,  BAD, 0xed, 0xfd, 0xf9,  BAD,  BAD, 0xf2,  BAD, 0xe1, 0xf1,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sec */{0x38,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sed */{0xf8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sei */{0x78,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sta */{ BAD, 0x85,  BAD,  BAD,  BAD, 0x95,  BAD, 0x8d, 0x9d, 0x99,  BAD,  BAD, 0x92,  BAD, 0x81, 0x91,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stx */{ BAD, 0x86,  BAD,  BAD,  BAD,  BAD, 0x96, 0x8e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sty */{ BAD, 0x84,  BAD,  BAD,  BAD, 0x94,  BAD, 0x8c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tax */{0xaa,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tay */{0xa8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsx */{0xba,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txa */{0x8a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txs */{0x9a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tya */{0x98,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/*         IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
};

static int map_65816[][MODES_ALL] = 
{/*        IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
/* bra */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x80,  BAD,  BAD,  BAD,  BAD},
/* brl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x82,  BAD,  BAD,  BAD},
/* cop */{ BAD,  BAD, 0x02,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jml */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x5c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xdc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jsl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x22,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* mvn */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x54,  BAD,  BAD},
/* mvp */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x44,  BAD,  BAD},
/* pea */{ BAD,  BAD,  BAD, 0xf4,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pei */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xd4,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* per */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x62,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phb */{0x8b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phd */{0x0b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phk */{0x4b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phx */{0xda,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* phy */{0x5a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plb */{0xab,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pld */{0x2b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plx */{0xfa,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ply */{0x7a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rep */{ BAD,  BAD, 0xc2,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rmb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rtl */{0x6b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sep */{ BAD,  BAD, 0xe2,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* smb */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stp */{0xdb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stz */{ BAD, 0x64,  BAD,  BAD,  BAD, 0x74,  BAD, 0x9c, 0x9e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tcd */{0x5b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tcs */{0x1b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tdc */{0x7b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* trb */{ BAD, 0x14,  BAD,  BAD,  BAD,  BAD,  BAD, 0x1c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsb */{ BAD, 0x04,  BAD,  BAD,  BAD,  BAD,  BAD, 0x0c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsc */{0x3b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txy */{0x9b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tyx */{0xbb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* wai */{0xcb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* wdm */{ BAD,  BAD, 0x42,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* xba */{0xeb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* xce */{0xfb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* adc */{ BAD, 0x65, 0x69, 0x69, 0x63, 0x75,  BAD, 0x6d, 0x7d, 0x79, 0x6f, 0x7f, 0x72, 0x73, 0x61, 0x71,  BAD,  BAD, 0x67, 0x77,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* and */{ BAD, 0x25, 0x29, 0x29, 0x23, 0x35,  BAD, 0x2d, 0x3d, 0x39, 0x2f, 0x3f, 0x32, 0x33, 0x21, 0x31,  BAD,  BAD, 0x27, 0x37,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* asl */{0x0a, 0x06,  BAD,  BAD,  BAD, 0x16,  BAD, 0x0e, 0x1e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x0a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bcc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x90,  BAD,  BAD,  BAD,  BAD},
/* bcs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xb0,  BAD,  BAD,  BAD,  BAD},
/* beq */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xf0,  BAD,  BAD,  BAD,  BAD},
/* bit */{ BAD, 0x24, 0x89,  BAD,  BAD, 0x34,  BAD, 0x2c, 0x3c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bmi */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x30,  BAD,  BAD,  BAD,  BAD},
/* bne */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xd0,  BAD,  BAD,  BAD,  BAD},
/* bpl */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x10,  BAD,  BAD,  BAD,  BAD},
/* brk */{0x00,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* bvc */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x50,  BAD,  BAD,  BAD,  BAD},
/* bvs */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x70,  BAD,  BAD,  BAD,  BAD},
/* clc */{0x18,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cld */{0xd8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cli */{0x58,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* clv */{0xb8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cmp */{ BAD, 0xc5, 0xc9, 0xc9, 0xc3, 0xd5,  BAD, 0xcd, 0xdd, 0xd9, 0xcf, 0xdf, 0xd2, 0xd3, 0xc1, 0xd1,  BAD,  BAD, 0xc7, 0xd7,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cpx */{ BAD, 0xe4, 0xe0, 0xe0,  BAD,  BAD,  BAD, 0xec,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* cpy */{ BAD, 0xc4, 0xc0, 0xc0,  BAD,  BAD,  BAD, 0xcc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dec */{0x3a, 0xc6,  BAD,  BAD,  BAD, 0xd6,  BAD, 0xce, 0xde,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x3a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dex */{0xca,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dey */{0x88,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* eor */{ BAD, 0x45, 0x49, 0x49, 0x43, 0x55,  BAD, 0x4d, 0x5d, 0x59, 0x4f, 0x5f, 0x52, 0x53, 0x41, 0x51,  BAD,  BAD, 0x47, 0x57,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* inc */{0x1a, 0xe6,  BAD,  BAD,  BAD, 0xf6,  BAD, 0xee, 0xfe,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x1a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* inx */{0xe8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* iny */{0xc8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jmp */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x4c,  BAD,  BAD, 0x5c,  BAD,  BAD,  BAD,  BAD,  BAD, 0x6c, 0x7c, 0xdc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jsr */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x20,  BAD,  BAD, 0x22,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xfc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* lda */{ BAD, 0xa5, 0xa9, 0xa9, 0xa3, 0xb5,  BAD, 0xad, 0xbd, 0xb9, 0xaf, 0xbf, 0xb2, 0xb3, 0xa1, 0xb1,  BAD,  BAD, 0xa7, 0xb7,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ldx */{ BAD, 0xa6, 0xa2, 0xa2,  BAD,  BAD, 0xb6, 0xae,  BAD, 0xbe,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ldy */{ BAD, 0xa4, 0xa0, 0xa0,  BAD, 0xb4,  BAD, 0xac, 0xbc,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* lsr */{0x4a, 0x46,  BAD,  BAD,  BAD, 0x56,  BAD, 0x4e, 0x5e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x4a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* nop */{0xea,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ora */{ BAD, 0x05, 0x09, 0x09, 0x03, 0x15,  BAD, 0x0d, 0x1d, 0x19, 0x0f, 0x1f, 0x12, 0x13, 0x01, 0x11,  BAD,  BAD, 0x07, 0x17,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pha */{0x48,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* php */{0x08,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* pla */{0x68,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* plp */{0x28,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rol */{0x2a, 0x26,  BAD,  BAD,  BAD, 0x36,  BAD, 0x2e, 0x3e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x2a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ror */{0x6a, 0x66,  BAD,  BAD,  BAD, 0x76,  BAD, 0x6e, 0x7e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x6a,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rti */{0x40,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rts */{0x60,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sbc */{ BAD, 0xe5, 0xe9, 0xe9, 0xe3, 0xf5,  BAD, 0xed, 0xfd, 0xf9, 0xef, 0xff, 0xf2, 0xf3, 0xe1, 0xf1,  BAD,  BAD, 0xe7, 0xf7,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sec */{0x38,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sed */{0xf8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sei */{0x78,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sta */{ BAD, 0x85,  BAD,  BAD, 0x83, 0x95,  BAD, 0x8d, 0x9d, 0x99, 0x8f, 0x9f, 0x92, 0x93, 0x81, 0x91,  BAD,  BAD, 0x87, 0x97,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stx */{ BAD, 0x86,  BAD,  BAD,  BAD,  BAD, 0x96, 0x8e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sty */{ BAD, 0x84,  BAD,  BAD,  BAD, 0x94,  BAD, 0x8c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tax */{0xaa,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tay */{0xa8,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tsx */{0xba,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txa */{0x8a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* txs */{0x9a,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tya */{0x98,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/*         IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
};

static int map_6502i[][MODES_ALL] =
{/*        IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
/* anc */{ BAD,  BAD, 0x2b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* ane */{ BAD,  BAD, 0x8b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* arr */{ BAD,  BAD, 0x6b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* asr */{ BAD,  BAD, 0x4b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dcp */{ BAD, 0xc7,  BAD,  BAD,  BAD, 0xd7,  BAD, 0xcf, 0xdf, 0xdb,  BAD,  BAD,  BAD,  BAD, 0xc3, 0xd3,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* dop */{0x80, 0x04, 0x80,  BAD,  BAD, 0x14,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* isb */{ BAD, 0xe7,  BAD,  BAD,  BAD, 0xf7,  BAD, 0xef, 0xff, 0xfb,  BAD,  BAD,  BAD,  BAD, 0xe3, 0xf3,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* jam */{0x03,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* las */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0xbb,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* lax */{ BAD, 0xa7,  BAD,  BAD,  BAD,  BAD, 0xb7, 0xaf,  BAD, 0xbf,  BAD,  BAD,  BAD,  BAD, 0xa3, 0xb3,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rla */{ BAD, 0x27,  BAD,  BAD,  BAD, 0x37,  BAD, 0x2f, 0x3f, 0x3b,  BAD,  BAD,  BAD,  BAD, 0x23, 0x33,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* rra */{ BAD, 0x67,  BAD,  BAD,  BAD, 0x77,  BAD, 0x6f, 0x7f, 0x7b,  BAD,  BAD,  BAD,  BAD, 0x63, 0x73,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sax */{ BAD, 0x87, 0xcb,  BAD,  BAD,  BAD, 0x97, 0x8f,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x83,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sha */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x9f,  BAD,  BAD,  BAD,  BAD,  BAD, 0x93,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* shx */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x9e,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* shy */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x9c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* slo */{ BAD, 0x07,  BAD,  BAD,  BAD, 0x17,  BAD, 0x0f, 0x1f, 0x1b,  BAD,  BAD,  BAD,  BAD, 0x03, 0x13,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* sre */{ BAD, 0x47,  BAD,  BAD,  BAD, 0x57,  BAD, 0x4f, 0x5f, 0x5b,  BAD,  BAD,  BAD,  BAD, 0x43, 0x53,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* stp */{0x13,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* tas */{ BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x9b,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD},
/* top */{0x0c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD, 0x0c,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD,  BAD}
/*         IMP    ZP  IMM   IMMA   ZPS   ZPX   ZPY   ABS  ABSX  ABSY  LONG LONGX INDZP  INDS  INDX  INDY   IND INDAX   DIR  DIRY   ACC   REL  RELA   TWO BITZP BITOF*/
};

static const char *disasm_formats[] = 
{
    "",
    "$%02x",
    "#$%02x",
    "#$%04x",
    "$%02x,s",
    "$%02x,x",
    "$%02x,y",
    "$%04x",
    "$%04x,x",
    "$%04x,y",
    "$%06x",
    "$%06x,x",
    "($%02x)",
    "($%02x,s),y",
    "($%02x,x)",
    "($%02x),y",
    "($%04x)",
    "($%04x,x)",
    "[$%02x]",
    "[$%02x],y",
    "a",
    "$%04x",
    "$%04x",
    "$%02x,$%02x",
    "%lld,$%02x",
    "%lld,$%02x,$%04x"
};

static size_t mode_index(addressing_mode mode)
{
    for(size_t mode_ix = 0; mode_ix < MODES_ALL; mode_ix++) {
        if (modes_map[mode_ix] == mode) {
            return mode_ix;
        }
    }
    return MODES_ALL;
}

static void disassemble(addressing_mode mode, char *disassembly, ...)
{
    memset(disassembly, 0, 13);
    const char *fmt = disasm_formats[mode_index(mode)];
    va_list ap;
    va_start(ap, disassembly);
    vsnprintf(disassembly, 12, fmt, ap);
    va_end(ap);
}

static int lookup_opcode(token_type mnemonic, int cpu, addressing_mode mode)
{
    size_t mnem_index;
    if (mnemonic <= TOKEN_TOP) {
        mnem_index = mnemonic - TOKEN_ANC;
    }
    else if (mnemonic <= TOKEN_XCE) {
        mnem_index = mnemonic - TOKEN_BRA;
    } else {
        mnem_index = mnemonic - TOKEN_ADC + (W65816_WORDS + 1);
    }
    const int *mnem_modes;
    switch (cpu) {
        case CPU_65C02: mnem_modes = map_65c02[mnem_index]; break;
        case CPU_65816: mnem_modes = map_65816[mnem_index]; break;
        default:        mnem_modes = map_6502[mnem_index]; break;
    }
    size_t mode_ix = mode_index(mode);
    int opc = mnem_modes[mode_ix];
    if (opc == BAD && mnemonic <= TOKEN_TOP) {
        return map_6502i[mnem_index][mode_ix];
    }
    return opc;
}

static int convert_to_relative(addressing_mode mode, value *val, int pc)
{
    if (MODE_HAS_FLAG(mode, ADDR_MODE_REL_FLAG)) {
        value min_val = MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) ? INT16_MIN : INT8_MIN;
        value max_val = min_val == INT16_MIN ? INT16_MAX : INT8_MAX;
        if (*val == VALUE_UNDEFINED) {
            return 1;
        }
        pc += 2;
        *val -= pc;
        if (*val < min_val || *val > max_val) {
            return 0;
        }
    }
    return 1;
}

static addressing_mode gen_implied(assembly_context *context, const token *mnemonic_token, const operand *operand, char *disassembly)
{
    addressing_mode mode = operand ? ADDR_MODE_ACCUM : ADDR_MODE_IMPLIED;
    int opc = lookup_opcode(mnemonic_token->type, context->options.cpu, mode);
    if (opc != BAD) {
        output_add(context->output, opc, 1);
        return mode;
    }
    return ADDR_MODE_ILLEGAL;
}

static addressing_mode gen_two_operand(assembly_context *context, const token *mnemonic_token, const operand *operand, char *disassembly)
{
    value op0 = evaluate_expression(context, operand->two_expression.expr0);
    value op1 = evaluate_expression(context, operand->two_expression.expr1);
    if (op0 < INT8_MIN || op0 > UINT8_MAX || op1 < INT8_MIN  || op1 > UINT8_MAX) {
        if (context->pass_needed || VALUE_UNDEFINED == op0 || VALUE_UNDEFINED == op1) {
            output_fill(context->output, 3);
            return ADDR_MODE_TWO_OPER;
        }
        const token *offending = (op0 < INT8_MIN || op0 > UINT8_MAX) ?
                        operand->two_expression.expr0->token :
                        operand->two_expression.expr1->token;
        tiny_error(offending, ERROR_MODE_RECOVER, "Illegal quantity");
        return ADDR_MODE_TWO_OPER;
    }
    int opc = lookup_opcode(mnemonic_token->type, context->options.cpu, ADDR_MODE_TWO_OPER);
    if (opc != BAD) {
        output_add(context->output, opc, 1);
        output_add(context->output, op0, 1);
        output_add(context->output, op1, 1);
        if (!context->pass_needed) disassemble(ADDR_MODE_TWO_OPER, disassembly, op0 & 0xff, op1 & 0xff);
        return ADDR_MODE_TWO_OPER;
    }
    return ADDR_MODE_ILLEGAL;
}

static addressing_mode gen_bit_operand(assembly_context *context, const token *mnemonic_token, const operand *operand, char *disassembly)
{
    addressing_mode mode = operand->form == FORM_BIT_ZP ? ADDR_MODE_BIT_ZP : ADDR_MODE_BIT_OFS;
    expression *bit;
    value zp_offs;
    if (mode == ADDR_MODE_BIT_ZP) {
        bit = operand->bit_expression.bit;
        zp_offs = evaluate_expression(context, operand->bit_expression.expr);
    } else {
        bit = operand->bit_offset_expression.bit;
        zp_offs = evaluate_expression(context, operand->bit_offset_expression.offs);
    }
    if (zp_offs < INT8_MIN || zp_offs > UINT8_MAX) {
        if (context->pass_needed || VALUE_UNDEFINED == zp_offs) {
            output_fill(context->output, mode == ADDR_MODE_BIT_ZP ? 2 : 3);
            return mode;
        }
        const token *offending = mode == ADDR_MODE_BIT_ZP ?
                    operand->bit_expression.expr->token :
                    operand->bit_offset_expression.offs->token;
        tiny_error(offending, ERROR_MODE_RECOVER, "Illegal quantity");
        return mode;
    }
    int opc = 0x07;
    if (mnemonic_token->type == TOKEN_BBR || mnemonic_token->type == TOKEN_BBS) {
        opc += 8;
    }
    if (mnemonic_token->type == TOKEN_BBS || mnemonic_token->type == TOKEN_SMB) {
        opc += 0x80;
    }
    opc += bit->value * 0x10;
    output_add(context->output, opc, 1);
    output_add(context->output, zp_offs, 1);
    if (mode == ADDR_MODE_BIT_OFS) {
        value rel = evaluate_expression(context, operand->bit_offset_expression.expr);
        if (rel < INT16_MIN || rel > UINT16_MAX) {
            if (context->pass_needed || VALUE_UNDEFINED == rel) {
                output_fill(context->output, 1);
                return mode;
            }
            tiny_error(operand->bit_offset_expression.expr->token, ERROR_MODE_RECOVER, "Relative branch too far from $%04x", context->output->logical_pc);
            return mode;
        }
        rel &= 0xffff;
        value displ = rel;
        if (!convert_to_relative(ADDR_MODE_BIT_OFS, &rel, context->output->logical_pc)) {
            tiny_error(operand->bit_offset_expression.expr->token, ERROR_MODE_RECOVER, "Relative branch too far from $%04x", context->output->logical_pc);
            return mode;
        }
        output_add(context->output, rel & 0xff, 1);
        if (!context->pass_needed) disassemble(mode, disassembly, bit->value & 7, zp_offs & 0xff, displ & 0xffff);
    } else if (!context->pass_needed) {
        disassemble(mode, disassembly, bit->value & 7, zp_offs & 0xff);
    }
    return mode;
}

static addressing_mode gen_relative(assembly_context *context, const token *mnemonic_token, const operand *operand, char *disassembly)
{
    addressing_mode mode = ADDR_MODE_RELATIVE;
    if (operand->single_expression.bitwidth) {
        switch (operand->single_expression.bitwidth->value) {
            case  8: break;
            case 16: mode = ADDR_MODE_REL_ABS; break;
            default:
                tiny_error(operand->single_expression.bitwidth->token, ERROR_MODE_RECOVER, "Invalid bitwidth modifier");
                return mode;
        }
    }
    value rel = evaluate_expression(context, operand->single_expression.expr);
    if (rel < INT16_MIN || rel > UINT16_MAX) {
        if (context->pass_needed || VALUE_UNDEFINED == rel) {
            output_fill(context->output, MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) ? 3 : 2);
        } else {
            tiny_error(operand->single_expression.expr->token, ERROR_MODE_RECOVER, "Relative branch too far from $%04x", context->output->logical_pc);
        }
        return mode;
    }
    rel &= 0xffff;
    value displ = rel;
    if (!convert_to_relative(mode, &rel, context->output->logical_pc)) {
        mode = ADDR_MODE_REL_ABS;
        convert_to_relative(mode, &rel, context->output->logical_pc);
    }
    int opc = lookup_opcode(mnemonic_token->type, context->options.cpu, mode);
    if (opc == BAD) {
        if (context->pass_needed) {
            output_fill(context->output, 2);
        }
        else {
            tiny_error(operand->single_expression.expr->token, ERROR_MODE_RECOVER, "Relative branch too far from $%04x", context->output->logical_pc);
        }
        return mode;
    }
    output_add(context->output, opc, 1);
    if (MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG)) {
        output_add(context->output, rel, 2);
    } else {
        output_add(context->output, rel & 0xff, 1);
    }
    if (!context->pass_needed) disassemble(mode, disassembly, displ);
    return mode;
}

static addressing_mode gen_single_operand(assembly_context *context, const token *mnemonic_token, const operand* oper, char *disassembly)
{
    addressing_mode mode = ADDR_MODE_ZP;
    switch (oper->form) {
        case FORM_IMMEDIATE:    mode = ADDR_MODE_IMMEDIATE; break;
        case FORM_DIRECT:       mode = ADDR_MODE_DIRECT; break;
        case FORM_DIRECT_Y:     mode = ADDR_MODE_DIR_ZP_Y; break;
        case FORM_INDEX_S:      mode = ADDR_MODE_ZP_S; break;
        case FORM_INDEX_X:      mode = ADDR_MODE_ZP_X; break;
        case FORM_INDEX_Y:      mode = ADDR_MODE_ZP_Y; break;
        case FORM_INDIRECT_S:   mode = ADDR_MODE_IND_ZP_S; break;
        case FORM_INDIRECT_X:   mode = ADDR_MODE_IND_ZP_X; break;
        case FORM_INDIRECT_Y:   mode = ADDR_MODE_IND_ZP_Y; break;
        case FORM_INDIRECT:     mode = ADDR_MODE_IND_ZP; break;
        default: break;
    }
    value oper_val = evaluate_expression(context, oper->single_expression.expr), orig_val = oper_val;
    token_type mnemonic = mnemonic_token->type;
    value page = (oper_val >> 8);
    if (context->options.cpu == CPU_65816 &&
        !MODE_HAS_FLAG(mode, ADDR_MODE_IMM_FLAG) &&
        !token_is_of_type(mnemonic_token, jmp_mnemonics, sizeof jmp_mnemonics) &&
        !context->pass_needed && 
        oper_val >= INT16_MIN && oper_val <= UINT16_MAX) {
        /* truncate values to byte if they are the same page as
           current and the mnemonic is a direct page mnemonic */
        if (page == context->page) {
            oper_val &= 0xff;
        }
    }
    if (oper_val < INT8_MIN || oper_val > UINT8_MAX) {
        if (oper_val < INT16_MIN || oper_val > UINT16_MAX) {
            if (oper_val < INT24_MIN || oper_val > UINT24_MAX) {
                if (context->pass_needed || VALUE_UNDEFINED == oper_val) {
                    /* guestimate the actual instruction size */
                    int size = context->output->logical_pc > UINT8_MAX ? 3 : 2;
                    if (mnemonic == TOKEN_JML || mnemonic == TOKEN_JML) {
                        size = 4;
                    } else if (mnemonic == TOKEN_JMP || mnemonic == TOKEN_JSR) {
                        size = 3;
                    }
                    else if (MODE_HAS_FLAG(mode, ADDR_MODE_IND_FLAG) ||
                            MODE_HAS_FLAG(mode, ADDR_MODE_DIR_FLAG) ||
                            MODE_HAS_FLAG(mode, ADDR_MODE_IMM_FLAG)) {
                        size = 2;
                    }
                    output_fill(context->output, size);
                    return mode;
                }
                tiny_error(oper->single_expression.expr->token, ERROR_MODE_RECOVER, "Illegal quantity");
                return mode;
            }
            oper_val &= 0xffffff;
            mode |= ADDR_MODE_LONG;
        } else {
            oper_val &= 0xffff;
            mode |= ADDR_MODE_ABS_FLAG;
        }
    } else {
        oper_val &= 0xff;
    }
    expression *bitwidth = oper->single_expression.bitwidth;
    if (bitwidth) {
        switch (bitwidth->value) {
            case 8:
                if (MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) || MODE_HAS_FLAG(mode, ADDR_MODE_LNG_FLAG)) {
                    tiny_error(oper->single_expression.expr->token, ERROR_MODE_RECOVER, "Illegal quantity");
                    return mode;
                }
                break;
            case 16:
                oper_val = orig_val; /* restore from page truncation */
                if (MODE_HAS_FLAG(mode, ADDR_MODE_LNG_FLAG)) {
                    tiny_error(oper->single_expression.expr->token, ERROR_MODE_RECOVER, "Illegal quantity");
                    return mode;
                }
                mode |= ADDR_MODE_ABS_FLAG;
                break;
            case 24:
                oper_val = orig_val; /* restore from page truncation */
                mode |= ADDR_MODE_LONG;
                break;
            default:
                tiny_error(bitwidth->token, ERROR_MODE_RECOVER, "Invalid bitwidth specifier");
                return mode;
        }
    }
    else if (MODE_HAS_FLAG(mode, ADDR_MODE_IMM_FLAG) && context->options.cpu == CPU_65816) {
        if (token_is_of_type(mnemonic_token, acc_mnemonics, sizeof acc_mnemonics)) {
            if (!context->m16 && (MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) || MODE_HAS_FLAG(mode, ADDR_MODE_LNG_FLAG))) {
                tiny_error(oper->single_expression.expr->token, ERROR_MODE_RECOVER,
                "Illegal quantity (8-bit immediate mode accumulator specified)");
                return mode;
            }
            if (context->m16) {
                mode |= ADDR_MODE_ABS_FLAG;
            }
        }
        else if (token_is_of_type(mnemonic_token, ix_mnemonics, sizeof ix_mnemonics)) {
            if (!context->x16 && (MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) || MODE_HAS_FLAG(mode, ADDR_MODE_LNG_FLAG))) {
                tiny_error(oper->single_expression.expr->token, ERROR_MODE_RECOVER,
                "Illegal quantity (8-bit index register mode specified)");
                return mode;
            }
            if (context->x16) {
                mode |= ADDR_MODE_ABS_FLAG;
            }
        }
    }
    int size = MODE_HAS_FLAG(mode, ADDR_MODE_LNG_FLAG) ? 3 :
               MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) ? 2 :
               1;
    int opc = lookup_opcode(mnemonic, context->options.cpu, mode);
    if (opc == BAD) {
        size = 2;
        oper_val = orig_val; /* restore from page truncation */
        if ((mnemonic == TOKEN_JMP || mnemonic == TOKEN_JML) && MODE_HAS_FLAG(mode, ADDR_MODE_DIR_FLAG) && !MODE_HAS_FLAG(mode, ADDR_MODE_LNG_FLAG)) {
            opc =lookup_opcode(mnemonic, context->options.cpu, ADDR_MODE_DIRECT);
        } else {
            if (MODE_HAS_FLAG(mode, ADDR_MODE_ZP) &&
                !MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) &&
                (!bitwidth || bitwidth->value > 8)) {
                mode |= ADDR_MODE_ABS_FLAG;
                opc = lookup_opcode(mnemonic, context->options.cpu, mode);
            }
            if (opc == BAD &&
                MODE_HAS_FLAG(mode, ADDR_MODE_ABS_FLAG) &&
                !MODE_HAS_FLAG(mode, ADDR_MODE_LNG_FLAG) &&
                (!bitwidth || bitwidth->value > 16)) {
                mode |= ADDR_MODE_LNG_FLAG;
                opc = lookup_opcode(mnemonic, context->options.cpu, mode);
                size = 3;
            }
        }
        if (opc == BAD) {
            if (context->pass_needed) {
                output_fill(context->output, size + 1);
                return mode;
            }
            return ADDR_MODE_ILLEGAL;
        }
    }
    output_add(context->output, opc, 1);
    output_add(context->output, oper_val, size);
    if ((mnemonic == TOKEN_JMP || mnemonic == TOKEN_JML) && MODE_HAS_FLAG(mode, ADDR_MODE_DIR_FLAG)) {
        snprintf(disassembly, 8, "[$%04llx]", oper_val & 0xffff);
    } else {
        disassemble(mode, disassembly, oper_val);
    }
    return mode;
}

void m6502_gen(assembly_context *context, const token *mnemonic_token, const operand *oper, char *disassembly)
{
    token_type mnemonic = mnemonic_token->type;
    addressing_mode mode;
    char disasm[13] = {};
    if (oper) {
        switch (oper->form) {
            case FORM_BIT_ZP:
            case FORM_BIT_OFFS_ZP:
                mode = gen_bit_operand(context, mnemonic_token, oper, disasm);
                break;
            case FORM_TWO_OPERANDS:
                mode = gen_two_operand(context, mnemonic_token, oper, disasm);
                break;
            case FORM_ACCUMULATOR:
                mode = gen_implied(context, mnemonic_token, oper, disasm);
                break;
            default:
                if (mnemonic == TOKEN_BCC ||
                    mnemonic == TOKEN_BCS ||
                    mnemonic == TOKEN_BEQ ||
                    mnemonic == TOKEN_BMI ||
                    mnemonic == TOKEN_BNE ||
                    mnemonic == TOKEN_BPL ||
                    mnemonic == TOKEN_BRA ||
                    mnemonic == TOKEN_BRL ||
                    mnemonic == TOKEN_BVC ||
                    mnemonic == TOKEN_BVS) {
                    mode = gen_relative(context, mnemonic_token, oper, disasm);
                }
                else {
                    mode = gen_single_operand(context, mnemonic_token, oper, disasm);
                }
        }
    }
    else {
        mode = gen_implied(context, mnemonic_token, oper, disasm);
    }
    if (mode == ADDR_MODE_ILLEGAL) {
        tiny_error(mnemonic_token, ERROR_MODE_RECOVER, "Mode not supported");
        return;
    }
    if (!context->pass_needed) {
        TOKEN_GET_TEXT(mnemonic_token, mnemonic_text);
        /* lowercase mnemonic for output */
        mnemonic_text[0] |= 0x60;
        mnemonic_text[1] |= 0x60;
        mnemonic_text[2] |= 0x60;
        if (!oper || oper->form == FORM_ACCUMULATOR) {
            char *m = mnemonic_text;
            *disassembly++ = *m++;
            *disassembly++ = *m++;
            *disassembly++ = *m;
            *disassembly = '\0';
            return;
        }
        snprintf(disassembly, 16, "%s %s", mnemonic_text, disasm);
    }
}
