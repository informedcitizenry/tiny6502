/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef options_h
#define options_h

#include "file.h"

typedef struct options
{

    source_file defines;
    const char *input;
    const char *output;
    const char *label;
    const char *list;
    const char *format;
    enum {
            CPU_UNSPECIFIED,
            CPU_6502,
            CPU_6502I,
            CPU_65C02,
            CPU_65816
    } cpu;
    int case_sensitive;
    const char **argv;
    int argc;
    
} options;

void options_cleanup(options *opt);

#endif /* options_h */
