/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef parser_h
#define parser_h

typedef struct statement statement;
typedef struct dynamic_array expression_array;
typedef struct lexer lexer;
typedef struct parser parser;

parser *parser_create(lexer*,int);
expression *assign_expression(parser*,statement*);
statement *parse_assignment(parser*);
statement *parse_statement(parser*);
void parser_destroy(parser*);

#endif /* parser_h */
