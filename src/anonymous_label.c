/**
* tiny6502
*
* Copyright (c) 2022 informedcitizenry <informedcitizenry@gmail.com>
*
* Licensed under the MIT license. See LICENSE for full license information.
*
*/

#include "anonymous_label.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

anonymous_label_collection *anonymous_label_make_collection(void)
{
    anonymous_label_collection *collection = tiny_malloc(sizeof(anonymous_label_collection));
    DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(collection->back, value, 97);
    DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(collection->forward, value, 97);
    DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(collection->all, value, 97);
    collection->backward_index = 0;
    collection->forward_index = 0;
    collection->add_mode = 1;
    collection->all->initialize = 1;
    return collection;
}

void anonymous_label_collection_destroy(anonymous_label_collection *collection)
{
    dynamic_array_cleanup_and_destroy(collection->back);
    dynamic_array_cleanup_and_destroy(collection->forward);
    dynamic_array_destroy(collection->all);
    tiny_free(collection);
}

void anonymous_label_add(anonymous_label_collection *collection)
{
    if (collection->add_mode) {
        dynamic_array_add(collection->all, NULL);
    }
}

void anonymous_label_add_forward(anonymous_label_collection *collection, value val)
{
    if (collection->add_mode) {
        value *this_label_val = tiny_malloc(sizeof(value));
        *this_label_val = val;
        dynamic_array_add(collection->forward, this_label_val);
        dynamic_array_add(collection->all, this_label_val);
    }
    collection->forward_index++;
}

void anonymous_label_add_backward(anonymous_label_collection *collection, value val)
{
    if (collection->add_mode) {
        value *this_label_val = tiny_malloc(sizeof(value));
        *this_label_val = val;
        dynamic_array_add(collection->back, this_label_val);
        dynamic_array_add(collection->all, this_label_val);
    }
    collection->backward_index++;
}

value anonymous_label_get_forward(anonymous_label_collection *collection, size_t count)
{
    if (!collection->add_mode) {
        size_t index = collection->forward_index + (count - 1);
        if (index < collection->forward->count) {
            return *(value*)collection->forward->data[index];
        }
    }
    return VALUE_UNDEFINED;
}

value anonymous_label_get_backward(anonymous_label_collection *collection, size_t count)
{
    if (collection->add_mode) {
        size_t index = collection->back->count - count;
        if (index < collection->back->count) {
            return *(value*)collection->back->data[index];
        }
    }
    size_t index = collection->backward_index - count;
    if (index < collection->back->count) {
        return *(value*)collection->back->data[index];
    }
    return VALUE_UNDEFINED;
}

value anonymous_label_get_by_name(anonymous_label_collection *collection,const char *name)
{
    size_t count = strlen(name);
    if (name[0] == '-') {
        return anonymous_label_get_backward(collection, count);
    }
    return anonymous_label_get_forward(collection, count);
}

void anonymous_label_update_current(anonymous_label_collection *collection, size_t at_index, value val)
{
    if (at_index < collection->all->count) {
        value *existing = (value*)collection->all->data[at_index];
        if (existing) {
            *existing = val;
        }
    }
}

value anonymous_label_get_current(anonymous_label_collection *collection, size_t at_index)
{
    if (at_index < collection->all->count) {
        value *existing = (value*)collection->all->data[at_index];
        if (existing) {
            return *existing;
        }
    }
    return VALUE_UNDEFINED;
}

void anonymous_label_collection_reset(anonymous_label_collection *collection)
{
    collection->add_mode = 0;
    collection->backward_index = 0;
    collection->forward_index = 0;
}
