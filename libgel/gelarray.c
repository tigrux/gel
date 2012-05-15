#include <gelarray.h>

#if GEL_ARRAY_USE_GARRAY

#include <gelvalue.h>
#include <gelvalueprivate.h>


/**
 * SECTION:gelarray
 * @short_description: Class used to keep an array of #GValue
 * @title: GelArray
 * @include: gel.h
 *
 * Class used to keep an array of #GValue.
 * It acts as a wrapper for #GValueArray or #GArray,
 * depending of the installed version of glib.
 */

/**
 * gel_array_new:
 * @n_prealloced: number of prealloced values, the initial size is still 0
 *
 * Creates a #GelArray with @n_prealloced values prealloced
 *
 * Returns: A new #GelArray
 */
GelArray* gel_array_new(guint n_prealloced)
{
    GelArray *self =
        g_array_sized_new(FALSE, TRUE, sizeof(GValue), n_prealloced);
    g_array_set_clear_func(self, (GDestroyNotify)g_value_unset);
    return self;
}

/**
 * gel_array_copy:
 * @self: the #GelArray to copy
 *
 * All the values in @array are copied into a new #GelArray 
 *
 * Returns: a copy of the #GelArray @self
 */
GelArray* gel_array_copy(GelArray *self)
{
    GValue *values = gel_array_get_values(self);
    guint n_values = gel_array_get_n_values(self);

    GelArray *copy = gel_array_new(n_values);
    GValue *copy_values = gel_array_get_values(copy);

    for(guint i = 0; i < n_values; i++)
    {
        GValue zero = {0};
        g_array_append_val(copy, zero);
        gel_value_copy(values + i, copy_values + i);
    }

    return copy;
}


/**
 * gel_array_free:
 * @self: the #GelArray to free
 *
 * All the values in @array unset and the array is freed
 *
 */
void gel_array_free(GelArray *self)
{
    g_array_unref(self);
}


/**
 * gel_array_get_n_values:
 * @self: the #GelArray to query
 *
 * Retrieves the numbers of values in @self
 *
 * Returns: the number of values in @self
 */
guint gel_array_get_n_values(const GelArray *self)
{
    return self->len;
}


/**
 * gel_array_set_n_values:
 * @self: the #GelArray to resize
 * @new_size: the new size of the array
 *
 * Sets the size of the array. This function should be used carefully.
 * This function is used when writing directly to the values of the array.
 *
 * Returns: the array @self
 */
GelArray* gel_array_set_n_values(GelArray *self, guint new_size)
{
    return g_array_set_size(self, new_size);
}


/**
 * gel_array_get_values:
 * @self: the #GelArray to query
 *
 * Retrieves a pointer to the first #GValue of the #GelArray @self
 *
 * Returns: a pointer to the first #GValue of the #GelArray @self
 */
GValue* gel_array_get_values(const GelArray *self)
{
    return (GValue *)self->data;
}


/**
 * gel_array_append:
 * @self: the #GelArray to append a #GValue
 * @value: a #GValue to append to @self
 *
 * Appends a copy of the #GValue @value to the #GelArray @self
 *
 * Returns: the array @self
 */
GelArray* gel_array_append(GelArray *self, const GValue *value)
{
    guint n_values = gel_array_get_n_values(self);
    GValue zero = {0};
    g_array_append_val(self, zero);

    GValue *values = gel_array_get_values(self);
    gel_value_copy(value, values + n_values);

    return self;
}


/**
 * gel_array_remove:
 * @self: the #GelArray to remove a #GValue by index
 * @index: the index of the value to remove
 *
 * Removes from @self the value at the position given by @index
 *
 * Returns: the array @self
 */
GelArray* gel_array_remove(GelArray *self, guint index)
{
    return g_array_remove_index(self, index);
}


/**
 * gel_array_sort:
 * @self: the #GelArray to sort
 * @compare_func: a function to compare the values
 *
 * Sorts the values of @self using @compare_func to compare them
 *
 * Returns: the array @self
 */
GelArray* gel_array_sort(GelArray *self, GCompareFunc compare_func)
{
    g_array_sort(self, compare_func);

    return self;
}

#endif

/**
 * GelArrayIter:
 * @array: a #GelArray to iterate
 * @index: the current index
 * @value: the #GValue at the current position
 * 
 * A #GelArrayIter represents an iterator that can be used to iterate over the elements of a #GelArray.
   #GelArrayIter structures are typically allocated on the stack and then initialized with #gel_array_iterator.
 */

/**
 * gel_array_iterator:
 * @self: a #GelArray to associate to an #GelArrayIter
 * @iter: a #GelArrayIter to initialize
 *
 * Initializes the #GelArrayIter @iter and associates it to @self
 */
void gel_array_iterator(GelArray *self, GelArrayIter *iter)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(iter != NULL);

    iter->array = self;
    iter->index = 0;
    iter->value = NULL;
}


/**
 * gel_array_iter_next:
 * @iter: a #GelArrayIter to advance
 *
 * Advances @iter to the next position in the associated array
 *
 * Returns: #TRUE if there is a next position available, #FALSE otherwise
 */
gboolean gel_array_iter_next(GelArrayIter *iter)
{
    g_return_val_if_fail(iter != NULL, FALSE);

    if(iter->index < gel_array_get_n_values(iter->array))
    {
        iter->value = gel_array_get_values(iter->array) + iter->index;
        iter->index++;
    }
    else
        iter->value = NULL;

    return iter->value != NULL;
}


/**
 * gel_array_iter_get:
 * @iter: a #GelArrayIter to get a #GValue
 *
 * Retrieves the #GValue in the current position of @iter
 *
 * Returns: a #GValue if the iter has not reached the end, #NULL otherwise
 */
GValue* gel_array_iter_get(GelArrayIter *iter)
{
    g_return_val_if_fail(iter != NULL, FALSE);
    g_return_val_if_fail(iter->value != NULL, FALSE);

    return iter->value;
}

