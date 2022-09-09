/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef executor_h
#define executor_h

typedef struct assembly_context assembly_context;
typedef struct statement statement;

void statement_execute(assembly_context *context, const statement *statement);

#endif /* executor_h */
