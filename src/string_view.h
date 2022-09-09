/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef string_view_h
#define string_view_h

typedef struct string_view
{
    
    char *ref;
    int is_dynamic;
    unsigned long start;
    unsigned long end;
    
} string_view;

string_view string_view_from_string(char *ref);

#endif /* string_view_h */
