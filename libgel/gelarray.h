#ifndef __GEL_ARRAY_H__
#define __GEL_ARRAY_H__

#include <config.h>

#include <glib-object.h>

#ifndef GEL_ARRAY_USE_GARRAY
#define GEL_ARRAY_USE_GARRAY 0
#endif

#if GEL_ARRAY_USE_GARRAY

typedef GArray GelArray;
#define GEL_TYPE_ARRAY G_TYPE_ARRAY
GelArray* gel_array_new(guint n_prealloced);
GelArray* gel_array_copy(GelArray *array);
void gel_array_free(GelArray *array);
guint gel_array_get_n_values(const GelArray *array);
GelArray* gel_array_set_n_values(GelArray *array, guint n);
GValue* gel_array_get_values(const GelArray *array);
GelArray* gel_array_append(GelArray *array, const GValue *value);
GelArray* gel_array_remove(GelArray *array, guint index);
GelArray* gel_array_sort(GelArray *array, GCompareFunc compare_func);

#else

typedef GValueArray GelArray;
#define GEL_TYPE_ARRAY G_TYPE_VALUE_ARRAY
#define gel_array_new g_value_array_new
#define gel_array_copy g_value_array_copy
#define gel_array_free g_value_array_free
#define gel_array_get_n_values(array) ((array)->n_values)
#define gel_array_set_n_values(array, n) ((array)->n_values = (n))
#define gel_array_get_values(array) ((array)->values)
#define gel_array_append g_value_array_append
#define gel_array_remove g_value_array_remove
#define gel_array_sort g_value_array_sort

#endif

#endif

