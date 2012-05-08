#ifndef __GEL_ARRAY_H__
#define __GEL_ARRAY_H__

typedef GValueArray GelValueArray;

#define GEL_TYPE_VALUE_ARRAY G_TYPE_VALUE_ARRAY
#define gel_value_array_new g_value_array_new
#define gel_value_array_copy g_value_array_copy
#define gel_value_array_free g_value_array_free
#define gel_value_array_get_n_values(array) ((array)->n_values)
#define gel_value_array_set_n_values(array, n) ((array)->n_values = (n))
#define gel_value_array_get_values(array) ((array)->values)
#define gel_value_array_append g_value_array_append
#define gel_value_array_remove g_value_array_remove
#define gel_value_array_sort g_value_array_sort

#endif
