/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "memory.h"
#include "options_parser.h"
#include <string.h>
#include <stdio.h>

static const char *help =
 "tiny6502 cross-asssembler Version 0.2\n"
 "(c) 2022 informedcitizenry\n"
 "\n"
 "Usage: tiny6502 [Options] file...\n"
 "Options:\n"
 "--case-sensitive, -C              Specificy case-sensitivity\n"
 "--cpu=<arg>, -c <arg>             Specificy the target CPU\n"
 "--define=<arg>, -D <arg>          Define one or more symbols\n"
 "--format=<arg>, -f <arg>          The output format\n"
 "--label=<file>, -l <file>         The label listing\n"
 "--list=<file>, -L <file>          The disassembly listing\n"
 "--output=<file>, -o <fil>         The output file\n"
 "--version, -v                     Print the version number\n"
 "--help, -h, -?                    This help message";

const char *get_arg(const char *option_arg, int *i, int argc, const char * option_name, const char *short_name, const char * argv[]) {
    if (option_arg) {
        fprintf(stderr, "option %s already defined.\n", option_name);
        exit(1);
    }
    const char *arg = argv[*i];
    if (strstr(arg, option_name) != arg && strstr(arg, short_name) != arg) {
        fprintf(stderr, "unknown option %s.\n", arg);
        exit(1);
    }
    const char *eq = strchr(arg, '=');
    if (eq) {
        if (++eq) {
            return eq;
        }
        fprintf(stderr, "argument expected for option %s.\n", option_name);
        exit(1);
    }
    if (++(*i) == argc || argv[*i][0] == '-') {
        fprintf(stderr, "argument expected for option %s.\n", option_name);
        exit(1);
    }
    return argv[*i];
}

options options_parse(int argc, const char * argv[])
{
    options opt = {
        .argc = argc,
        .argv = argv
    };
    for(int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (arg[0] != '-') {
            if (opt.input) {
                fprintf(stderr, "Input file previously specified.");
                exit(1);
            }
            opt.input = arg;
        } else {
            if (strcmp(arg, "--case-sensitive") == 0 ||
                strcmp(arg, "-C") == 0) {
                if (opt.case_sensitive) {
                    fputs("option --case-sensitive already defined\n", stderr);
                    exit(1);
                }
                opt.case_sensitive = 1;
            }
            else if (strstr(arg, "--cpu") ||
                     strcmp(arg, "-c") == 0) {
                const char *cpu = get_arg(NULL, &i, argc, "--cpu", "-c", argv);
                if (opt.cpu != CPU_UNSPECIFIED) {
                    fputs("option --cpu already set\n", stderr);
                    exit(1);
                }
                if (strcmp(cpu, "6502") == 0) {
                    opt.cpu = CPU_6502;
                }
                else if (strcmp(cpu, "6502i") == 0) {
                    opt.cpu = CPU_6502I;
                }
                else if (strcmp(cpu, "65C02") == 0) {
                    opt.cpu = CPU_65C02;
                }
                else if (strcmp(cpu, "65816") == 0) {
                    opt.cpu = CPU_65816;
                } else {
                    fprintf(stderr, "Invalid cpu '%s' specified for option --cpu\n", cpu);
                    exit(1);
                }
            }
            else if (strstr(arg, "--define") ||
                     strstr(arg, "-D") == arg) {
                int first_def_truncated = arg[1] == 'D' && arg[2];
                int j = i;
                for(; j + 1 < argc && argv[j + 1][0] != '-'; j++) { }
                int num_defs = (j - i) + first_def_truncated;
                if (num_defs < 1) {
                    fprintf(stderr, "Option '--define' expects one or more arguments.");
                    exit(1);
                }
                opt.defines.file_name = NULL;
                opt.defines.line_numbers = num_defs;
                opt.defines.lines = tiny_malloc(sizeof(char*)*num_defs);
                
                if (first_def_truncated) {
                    /* user passed in -DMydefine*/
                    size_t def_len = strlen(arg + 2);
                    opt.defines.lines[0] = tiny_calloc(def_len + 2, sizeof(char));
                    strncpy(opt.defines.lines[0], arg + 2, def_len);
                    opt.defines.lines[0][def_len] = '\n';
                }
                for(j = first_def_truncated; j < num_defs; j++) {
                    const char *define = argv[++i];
                    size_t def_len = strlen(define);
                    opt.defines.lines[j] = tiny_calloc(def_len + 2, sizeof(char));
                    strncpy(opt.defines.lines[j], define, def_len);
                    opt.defines.lines[j][def_len] = '\n';
                }
            }
            else if (strstr(arg, "--format") ||
                     strcmp(arg, "-f") == 0) {
                opt.format = get_arg(opt.format, &i, argc, "--format", "-f", argv);
            }
            else if (strstr(arg, "--output") ||
                     strcmp(arg, "-o") == 0) {
                opt.output = get_arg(opt.output, &i, argc, "--output", "-o", argv);
            }
            else if (strstr(arg, "--label") ||
                     strcmp(arg, "-l") == 0) {
                opt.label = get_arg(opt.label, &i, argc, "--label", "-l", argv);
            }
            else if (strstr(arg, "--list") ||
                     strcmp(arg, "-L") == 0) {
                opt.list = get_arg(opt.list, &i, argc, "--list", "-L", argv);
            }
            else if (strcmp(arg, "--version") == 0 ||
                     strcmp(arg, "-V") == 0) {
                puts("Version 0.01");
                exit(0);
            }
            else if (strcmp(arg, "-?") == 0 || strcmp(arg, "-h") == 0 ||
                     strcmp(arg, "--help") == 0) {
                puts(help);
                exit(0);
            } else {
                fprintf(stderr, "Unknown option %s\n", arg);
                exit(1);
            }
        }
    }
    if (!opt.output) opt.output = "a.out";
    if (!opt.format) opt.format = "cbm";
    if (opt.cpu == CPU_UNSPECIFIED) opt.cpu = CPU_6502;
    return opt;
}
