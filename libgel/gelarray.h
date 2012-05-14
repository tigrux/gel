#ifndef __GEL_ARRAY_H__
#define __GEL_ARRAY_H__

#include <glib-object.h>

#if GLIB_CHECK_VERSION(2,32,0)
#define GEL_ARRAY_USE_GARRAY 1
#else
#define GEL_ARRAY_USE_GARRAY 0
#endif

#if GEL_ARRAY_USE_GARRAY

typedef GArray GelArray;
#define GEL_TYPE_ARRAY G_TYPE_ARRAY
GelArray* gel_array_new(guint n_prealloced);
GelArray* gel_array_copy(GelArray *self);
void gel_array_free(GelArray *self);
guint gel_array_get_n_values(const GelArray *self);
GelArray* gel_array_set_n_values(GelArray *self, guint new_size);
GValue* gel_array_get_values(const GelArray *self);
GelArray* gel_array_append(GelArray *self, const GValue *value);
GelArray* gel_array_remove(GelArray *self, guint index);
GelArray* gel_array_sort(GelArray *self, GCompareFunc compare_func);

#else

typedef GValueArray GelArray;
#define GEL_TYPE_ARRAY G_TYPE_VALUE_ARRAY
#define gel_array_new g_value_array_new
#define gel_array_copy g_value_array_copy
#define gel_array_free g_value_array_free
#define gel_array_get_n_values(array) ((array)->n_values)
#define gel_array_set_n_values(array, new_size) ((array)->n_values = (new_size))
#define gel_array_get_values(array) ((array)->values)
#define gel_array_append g_value_array_append
#define gel_array_remove g_value_array_remove
#define gel_array_sort g_value_array_sort

#endif

typedef struct _GelArrayIter GelArrayIter;
struct _GelArrayIter
{
    GelArray *array;
    guint index;
};


void gel_array_iter_init(GelArrayIter *iter, GelArray *array);
GValue* gel_array_iter_next_value(GelArrayIter *iter);

#endif

