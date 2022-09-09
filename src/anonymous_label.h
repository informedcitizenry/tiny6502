/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#ifndef anonymous_label_h
#define anonymous_label_h

#include "value.h"

typedef struct dynamic_array dynamic_array;

typedef struct anonymous_label
{
    value value;
    int back_set;
    struct anonymous_label **back;
    struct anonymous_label **forward;

} anonymous_label;

typedef struct anonymous_label_collection
{
    dynamic_array *back;
    dynamic_array *forward;
    dynamic_array *all;
    size_t backward_index;
    size_t forward_index;
    size_t count;
    size_t capacity;
    int add_mode;
    anonymous_label *labels;
    
} anonymous_label_collection;

anonymous_label_collection *anonymous_label_make_collection(void);

void anonymous_label_collection_destroy(anonymous_label_collection *collection);
void anonymous_label_collection_reset(anonymous_label_collection *collection);

void anonymous_label_add(anonymous_label_collection *collection);
void anonymous_label_add_forward(anonymous_label_collection *collection, value value);
void anonymous_label_add_backward(anonymous_label_collection *collection, value value);
value anonymous_label_get_forward(anonymous_label_collection *collection, size_t count);
value anonymous_label_get_backward(anonymous_label_collection *collection, size_t count);
value anonymous_label_get_by_name(anonymous_label_collection *collection, const char *name);
void anonymous_label_update_current(anonymous_label_collection *collection, size_t at_index, value value);
value anonymous_label_get_current(anonymous_label_collection *collection, size_t at_index);

#endif /* anonymous_label_h */
