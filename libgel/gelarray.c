#include <gelarray.h>

#if GEL_ARRAY_USE_GARRAY

#include <gelvalue.h>


GelArray* gel_array_new(guint n_prealloced)
{
    return g_array_sized_new(FALSE, TRUE, sizeof(GValue), n_prealloced);
}


GelArray* gel_array_copy(GelArray *array)
{
    GValue *values = gel_array_get_values(array);
    guint n_values = gel_array_get_n_values(array);

    GelArray *copy = gel_array_new(n_values);
    GValue *copy_values = gel_array_get_values(copy);

    for(guint i = 0; i < n_values; i++)
    {
        GValue zero = G_VALUE_INIT;
        g_array_append_val(copy, zero);
        gel_value_copy(values + i, copy_values + i);
    }

    return copy;
}


void gel_array_free(GelArray *array)
{
    g_array_unref(array);
}


guint gel_array_get_n_values(const GelArray *array)
{
    return array->len;
}


GelArray* gel_array_set_n_values(GelArray *array, guint n)
{
    return g_array_set_size(array, n);
}


GValue* gel_array_get_values(const GelArray *array)
{
    return (GValue *)array->data;
}


GelArray* gel_array_append(GelArray *array, const GValue *value)
{
    guint n_values = gel_array_get_n_values(array);
    GValue zero = G_VALUE_INIT;
    g_array_append_val(array, zero);

    GValue *values = gel_array_get_values(array);
    gel_value_copy(value, values + n_values);

    return array;
}


GelArray* gel_array_remove(GelArray *array, guint index)
{
    GValue *values = gel_array_get_values(array);
    g_value_unset(values + index);

    return g_array_remove_index(array, index);
}


GelArray* gel_array_sort(GelArray *array, GCompareFunc compare_func)
{
    g_array_sort(array, compare_func);

    return array;
}


#endif

