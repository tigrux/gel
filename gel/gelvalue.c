#include <glib-object.h>

#include <gelvalue.h>
#include <gelvalueprivate.h>

#define DEFINE_ARITHMETIC(op) \
gboolean gel_values_##op(const GValue *v1, const GValue *v2, GValue *value) \
{ \
    return gel_values_arithmetic(v1, v2, value, gel_values_simple_##op); \
}


#define DEFINE_LOGIC(op) \
gboolean gel_values_##op(const GValue *v1, const GValue *v2) \
{ \
    return gel_values_logic(v1, v2, gel_values_simple_##op); \
}


/**
 * SECTION:gelvalue
 * @short_description: Collection of utilities to work with values.
 * @title: GelValue
 * @include: gel.h
 *
 * Collection of utilities to work with #GValue structures.
 *
 * It includes functions to create, copy, duplicate, transform and apply
 * arithmetic and logic operations on #GValue structures.
 */


/**
 * gel_value_new:
 *
 * Creates a new uninitialized dynamic #GValue
 *
 * Returns: a new uninitialized #GValue
 */
GValue* gel_value_new(void)
{
    return g_slice_new0(GValue);
}


/**
 * gel_value_new_of_type:
 * @type: a #GType
 *
 * Creates a new #GValue of type @type
 *
 * Returns: a new #GValue of the type specified by @type
 */
GValue* gel_value_new_of_type(GType type)
{
    g_return_val_if_fail(type != G_TYPE_INVALID, NULL);
    return g_value_init(gel_value_new(), type);
}


/**
 * gel_value_new_from_closure:
 * @value_closure: an instance of #GClosure
 *
 * Creates a new #GValue of type #GClosure holding @value_closure
 *
 * Returns: a new #GValue of type #GClosure holding @value_closure
 */
GValue* gel_value_new_from_closure(GClosure *value_closure)
{
    g_return_val_if_fail(value_closure != NULL, NULL);

    GValue *value = gel_value_new_of_type(G_TYPE_CLOSURE);
    g_value_set_boxed(value, value_closure);
    return value;
}


/**
 * gel_value_new_closure_from_marshal:
 * @marshal: a #GClosureMarshal function
 * @object: an instance of #GObject
 *
 * Creates a new #GClosure with @object as closure's marshal_data and
 * @marshal as the closure's marshal.
 *
 * Returns: a new #GValue of type #GClosure holding the created closure
 */
GValue* gel_value_new_closure_from_marshal(GClosureMarshal marshal,
                                           GObject *object)
{
    g_return_val_if_fail(object != NULL, NULL);
    g_return_val_if_fail(G_IS_OBJECT(object), NULL);

    GClosure *closure = g_closure_new_object(sizeof(GClosure), object);
    g_closure_set_marshal(closure, marshal);
    return gel_value_new_from_closure(closure);
}


/**
 * gel_value_new_from_boolean:
 * @value_boolean: a #gboolean
 *
 * Creates a new #GValue of type #gboolean set from @value_boolean
 *
 * Returns: a new #GValue of type #gboolean set from @value_boolean
 */
GValue* gel_value_new_from_boolean(gboolean value_boolean)
{
    GValue *value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    g_value_set_boolean(value, value_boolean);
    return value;
}


/**
 * gel_value_new_from_pointer:
 * @value_pointer: a #gpointer
 *
 * Creates a new #GValue of type #gpointer set from by @value_pointer
 *
 * Returns: a new #GValue of type #gpointer set from by @value_pointer
 */
GValue* gel_value_new_from_pointer(gpointer value_pointer)
{
    GValue *value = gel_value_new_of_type(G_TYPE_POINTER);
    g_value_set_pointer(value, value_pointer);
    return value;
}


/**
 * gel_value_dup:
 * @value: a #GValue
 *
 * Creates a new #GValue that is a copy of @value
 *
 * Returns: a #GValue that is a copy of @value
 */
GValue *gel_value_dup(const GValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(G_IS_VALUE(value), NULL);

    GValue *dup_value = gel_value_new_of_type(G_VALUE_TYPE(value));
    g_value_copy(value, dup_value);
    return dup_value;
}


/**
 * gel_value_copy:
 * @src_value: a #GValue
 * @dest_value: a #GValue
 *
 * If #dest_value has been set, then tries to transform @src_value to @dest_value.
 * If @dest_value is unset, then it is initialized to @src_values's type and then copied.
 *
 * Returns: #TRUE if @src_value was succesfully copied to @dest_value, #FALSE otherwise.
 */
gboolean gel_value_copy(const GValue *src_value, GValue *dest_value)
{
    g_return_val_if_fail(src_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);
    g_return_val_if_fail(G_IS_VALUE(src_value), FALSE);

    gboolean result = TRUE;

    if(G_IS_VALUE(dest_value))
    {
        if(!g_value_transform(src_value, dest_value))
        {
            g_warning("Cannot assign from %s to %s",
                G_VALUE_TYPE_NAME(src_value), G_VALUE_TYPE_NAME(dest_value));
            result = FALSE;
        }
    }
    else
    {
        g_value_init(dest_value, G_VALUE_TYPE(src_value));
        g_value_copy(src_value, dest_value);
    }
    return result;
}


/**
 * gel_value_free:
 * @value: a #GValue allocated by any of the gel_value_new_* functions.
 *
 * Frees a dynamically created value, unsetting it if needed.
 */
void gel_value_free(GValue *value)
{
    g_return_if_fail(value != NULL);

    if(G_IS_VALUE(value))
        g_value_unset(value);
    g_slice_free(GValue, value);
}


/**
 * gel_value_list_free:
 * @list: a #GList of #GValue
 *
 * Frees @list of #GValue
 * This function is needed to free lists used by #gel_context_eval_params
 */
void gel_value_list_free(GList *list)
{
    const register GList *iter;
    for(iter = list; iter != NULL; iter = iter->next)
    {
        GValue *iter_value = (GValue*)iter->data;
        gel_value_free(iter_value);
    }
    g_list_free(list);
}


static
gchar* gel_value_array_to_string(const GValueArray *array)
{
    GString *buffer = g_string_new("[");
    const guint n_values = array->n_values;
    if(n_values > 0)
    {
        const guint last = n_values - 1;
        const GValue *const array_values = array->values;
        register guint i;
        for(i = 0; i <= last; i++)
        {
            gchar *s = gel_value_to_string(array_values + i);
            g_string_append_printf(buffer, "%s%s", s, i != last ? " " : "");
            g_free(s);
        }
    }
    g_string_append_c(buffer, ']');

    return g_string_free(buffer, FALSE);
}


static
void gel_value_array_to_string_transform(const GValue *src_value,
                                         GValue *dest_value)
{
    const GValueArray *array = (GValueArray*)g_value_get_boxed(src_value);
    g_value_take_string(dest_value, gel_value_array_to_string(array));
}


/**
 * gel_value_to_string:
 * @value: a #GValue
 *
 * Try to create a string representation of @value.
 * Use it only for debug, not for serialization.
 *
 * Returns: a new allocated string.
 */
gchar* gel_value_to_string(const GValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(G_IS_VALUE(value), NULL);

    static volatile gsize only_once = 0;
    if(g_once_init_enter(&only_once))
    {
        g_value_register_transform_func(
            G_TYPE_VALUE_ARRAY, G_TYPE_STRING,
            gel_value_array_to_string_transform);
        g_once_init_leave (&only_once, 1);
    }

    gchar *result = NULL;
    GValue string_value = {0};

    g_value_init(&string_value, G_TYPE_STRING);
    if(g_value_transform(value, &string_value))
    {
        result = g_value_dup_string(&string_value);
        g_value_unset(&string_value);
    } 
    else
    {
        GString *repr_string = g_string_new("<");
        g_string_append_printf(repr_string, "%s", G_VALUE_TYPE_NAME(value));
        if(G_VALUE_HOLDS_GTYPE(value))
            g_string_append_printf(
                repr_string, " %s", g_type_name(g_value_get_gtype(value)));
        else
        if(g_value_fits_pointer(value))
            g_string_append_printf(
                repr_string, " %p", g_value_peek_pointer(value));
        g_string_append_c(repr_string, '>');
        result = g_string_free(repr_string, FALSE);
    }
    return result;
}


/**
 * gel_value_to_boolean:
 * @value: a #GValue
 *
 * Check the type and content of @value. Anything different to zero is
 * considered #TRUE.
 * Zero means 0 for integers, 0.0 for doubles, "" for strings,
   NULL for pointers, and empty for arrays.
 *
 * Returns: #FALSE if the value holds a zero, #TRUE otherwise.
 */
gboolean gel_value_to_boolean(const GValue *value)
{
    g_return_val_if_fail(value != NULL, FALSE);

    gboolean result;
    GValue bool_value = {0};
    g_value_init(&bool_value, G_TYPE_BOOLEAN);
    if(g_value_transform(value, &bool_value))
    {
        result = g_value_get_boolean(&bool_value);
        g_value_unset(&bool_value);
    }
    else
    {
        if(G_VALUE_HOLDS_STRING(value))
            result = (
                g_value_get_string(value) != NULL
                && g_strcmp0(g_value_get_string(value), "") != 0);
        else
        if(G_VALUE_HOLDS_LONG(value))
            result = (g_value_get_long(value) != 0);
        else
        if(G_VALUE_HOLDS_DOUBLE(value))
            result = (g_value_get_double(value) != 0.0);
        else
        if(G_VALUE_HOLDS(value, G_TYPE_ARRAY))
        {
            GValueArray *array = (GValueArray*)g_value_get_boxed(value);
            if(array == NULL || array->n_values == 0)
                result = FALSE;
            else
                result = TRUE;
        }
        else
        if(g_value_fits_pointer(value))
            result = (g_value_peek_pointer(value) != NULL);
        else
            result = TRUE;
    }

    return result;
}


static
GType gel_value_simple_type(const GValue *value)
{
    g_return_val_if_fail(value != NULL, G_TYPE_INVALID);

    GType type = G_VALUE_TYPE(value);
    switch(type)
    {
        case G_TYPE_UINT:
        case G_TYPE_ULONG:
        case G_TYPE_INT:
        case G_TYPE_LONG:
            return G_TYPE_LONG;
        case G_TYPE_FLOAT:
        case G_TYPE_DOUBLE:
            return G_TYPE_DOUBLE;
        case G_TYPE_STRING:
            return G_TYPE_STRING;
        case G_TYPE_BOOLEAN:
            return G_TYPE_BOOLEAN;
        default:
            if(G_VALUE_HOLDS_ENUM(value) || G_VALUE_HOLDS_FLAGS(value))
                return G_TYPE_LONG;
            if(G_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY))
                return G_TYPE_VALUE_ARRAY;
            if(g_value_fits_pointer(value))
                return G_TYPE_POINTER;
            g_warning("Type %s could not be simplified", g_type_name(type));
            return type;
    }
}


static
GType gel_values_simple_type(const GValue *v1, const GValue *v2)
{
    GType l_type = gel_value_simple_type(v1);
    GType r_type = gel_value_simple_type(v2);

    if(l_type == r_type)
        return l_type;

    if(l_type == G_TYPE_INVALID || r_type == G_TYPE_INVALID)
        return G_TYPE_INVALID;

    if(l_type == G_TYPE_DOUBLE && r_type == G_TYPE_LONG)
        return G_TYPE_DOUBLE;

    if(r_type == G_TYPE_DOUBLE && l_type == G_TYPE_LONG)
        return G_TYPE_DOUBLE;

    return G_TYPE_INVALID;
}


static
gboolean gel_values_simple_add(const GValue *v1, const GValue *v2, 
                               GValue *dest_value)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    GType type = G_VALUE_TYPE(dest_value);
    switch(type)
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(v1)
                + g_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                g_value_get_double(v1)
                + g_value_get_double(v2));
            return TRUE;
        case G_TYPE_STRING:
            g_value_take_string(dest_value,
                g_strconcat(
                    g_value_get_string(v1),
                    g_value_get_string(v2),
                NULL));
            return TRUE;
        default:
            if(type == G_TYPE_VALUE_ARRAY)
            {
                GValueArray *a1 = (GValueArray*)g_value_get_boxed(v1);
                GValueArray *a2 = (GValueArray*)g_value_get_boxed(v2);
                const guint n1_values = a1->n_values;
                const guint n2_values = a2->n_values;

                GValueArray *array = g_value_array_new(n1_values + n2_values);
                register guint i;

                const GValue *const a1_values = a1->values;
                for(i = 0; i < n1_values; i++)
                    g_value_array_append(array, a1_values + i);

                const GValue *const a2_values = a2->values;
                for(i = 0; i < n2_values; i++)
                    g_value_array_append(array, a2_values + i);
                g_value_take_boxed(dest_value, array);
                return TRUE;
            }
    }
    return FALSE;
}


static
gboolean gel_values_simple_sub(const GValue *v1, const GValue *v2,
                               GValue *dest_value)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(v1)
                - g_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                g_value_get_double(v1)
                - g_value_get_double(v2));
            return TRUE;
    }
    return FALSE;
}


static
gboolean gel_values_simple_mul(const GValue *v1, const GValue *v2,
                               GValue *dest_value)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(v1)
                * g_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                g_value_get_double(v1)
                * g_value_get_double(v2));
            return TRUE;
    }
    return FALSE;
}


static
gboolean gel_values_simple_div(const GValue *v1, const GValue *v2,
                              GValue *dest_value)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(v1)
                / g_value_get_long(v2));
            return TRUE;
    }
    return FALSE;
}


static
gboolean gel_values_simple_mod(const GValue *v1, const GValue *v2,
                               GValue *dest_value)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(v1)
                % g_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                (long)g_value_get_double(v1)
                % (long)g_value_get_double(v2));
            return TRUE;
    }
    return FALSE;
}


static
GType gel_values_simple_transform(const GValue *v1, const GValue *v2,
                                  GValue *vv1, GValue *vv2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    GType simple_type = gel_values_simple_type(v1, v2);
    g_return_val_if_fail(simple_type != G_TYPE_INVALID, FALSE);

    g_value_init(vv1, simple_type);
    g_value_init(vv2, simple_type);

    GType result = G_TYPE_INVALID;
    if(g_value_transform(v1, vv1))
    {
        if(g_value_transform(v2, vv2))
            result = simple_type;
        else
        {
            g_value_unset(vv1);
            g_value_unset(vv2);
        }
    }
    else
        g_value_unset(vv1);

    return result;
}


static
gboolean gel_values_can_compare(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);
    
    GType type = gel_values_simple_type(v1, v2);
    switch(type)
    {
        case G_TYPE_LONG:
        case G_TYPE_DOUBLE:
        case G_TYPE_STRING:
        case G_TYPE_POINTER:
        case G_TYPE_BOOLEAN:
            return TRUE;
        default:
            if(type == G_TYPE_VALUE_ARRAY)
                return TRUE;
            g_warning("Values cannot be compared");
            return FALSE;
    }
}


static
gboolean gel_values_simple_gt(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_compare(v1, v2))
        return gel_values_compare(v1, v2) > 0;
    return FALSE;
}


static
gboolean gel_values_simple_ge(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_compare(v1, v2))
        return gel_values_compare(v1, v2) >= 0;
    return FALSE;
}


static
gboolean gel_values_simple_eq(const GValue *v1, const GValue *v2)
{
    if(gel_values_can_compare(v1, v2))
        return gel_values_compare(v1, v2) == 0;
    return FALSE;
}


static
gboolean gel_values_simple_le(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_compare(v1, v2))
        return gel_values_compare(v1, v2) <= 0;
    return FALSE;
}


static
gboolean gel_values_simple_lt(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_compare(v1, v2))
        return gel_values_compare(v1, v2) < 0;
    return FALSE;
}


static
gboolean gel_values_simple_ne(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_compare(v1, v2))
        return gel_values_compare(v1, v2) != 0;
    return FALSE;
}



static
gboolean gel_values_arithmetic(const GValue *v1, const GValue *v2,
                               GValue *dest_value,
                               GelValuesArithmetic values_function)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    GValue vv1 = {0};
    GValue vv2 = {0};

    GType dest_type = gel_values_simple_transform(v1, v2, &vv1, &vv2);
    g_return_val_if_fail(dest_type != G_TYPE_INVALID, FALSE);

    g_value_init(dest_value, dest_type);
    gboolean result = values_function(&vv1, &vv2, dest_value);

    g_value_unset(&vv1);
    g_value_unset(&vv2);
    return result;
}


gint gel_values_compare(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    GValue vv1 = {0};
    GValue vv2 = {0};

    GType simple_type = gel_values_simple_transform(v1, v2, &vv1, &vv2);
    g_return_val_if_fail(simple_type != G_TYPE_INVALID, FALSE);

    gboolean result = -1;
    switch(simple_type)
    {
        case G_TYPE_LONG:
        {
            glong i1 = g_value_get_long(&vv1);
            glong i2 = g_value_get_long(&vv2);
            result = i1 > i2 ? 1 : i1 < i2 ? -1 : 0;
            break;
        }
        case G_TYPE_DOUBLE:
        {
            gdouble d1 = g_value_get_double(&vv1);
            gdouble d2 = g_value_get_double(&vv2);
            result = d1 > d2 ? 1 : d1 < d2 ? -1 : 0;
            break;
        }
        case G_TYPE_BOOLEAN:
        {
            gint b1 = g_value_get_boolean(&vv1);
            gint b2 = g_value_get_boolean(&vv2);
            result = b1 > b2 ? 1 : b1 < b2 ? -1 : 0;
            break;
        }
        case G_TYPE_STRING:
        {
            result = g_strcmp0(
                g_value_get_string(&vv1), g_value_get_string(&vv2));
            break;
        }
        case G_TYPE_POINTER:
        {
            
            gpointer p1 = g_value_peek_pointer(&vv1);
            gpointer p2 = g_value_peek_pointer(&vv2);
            result = p1 > p2 ? 1 : p1 < p2 ? -1 : 0;
            break;
        }
        default:
            if(simple_type == G_TYPE_VALUE_ARRAY)
            {
                GValueArray *a1 = (GValueArray*)g_value_get_boxed(&vv1);
                GValueArray *a2 = (GValueArray*)g_value_get_boxed(&vv2);
                guint a1_n = a1->n_values;
                guint a2_n = a2->n_values;
                guint n_values = MIN(a1_n, a2_n);

                register guint i;
                GValue *const a1_values = a1->values;
                GValue *const a2_values = a2->values;
                for(i = 0; i < n_values; i++)
                {
                    result = gel_values_compare(a1_values + i, a2_values + i);
                    if(result != 0)
                        break;
                }
                if(i == n_values)
                    result = a1_n > a2_n ? 1 : a1_n < a2_n ? -1 : 0;
            }
    }

    g_value_unset(&vv1);
    g_value_unset(&vv2);
    return result;
}


static
gboolean gel_values_logic(const GValue *v1, const GValue *v2,
                          GelValuesLogic values_function)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    GValue vv1 = {0};
    GValue vv2 = {0};

    GType simple_type = gel_values_simple_transform(v1, v2, &vv1, &vv2);
    g_return_val_if_fail(simple_type != G_TYPE_INVALID, FALSE);

    gboolean result = values_function(&vv1, &vv2);
    g_value_unset(&vv1);
    g_value_unset(&vv2);
    return result;
}


DEFINE_LOGIC(gt)
DEFINE_LOGIC(ge)
DEFINE_LOGIC(eq)
DEFINE_LOGIC(le)
DEFINE_LOGIC(lt)
DEFINE_LOGIC(ne)

DEFINE_ARITHMETIC(add)
DEFINE_ARITHMETIC(sub)
DEFINE_ARITHMETIC(mul)
DEFINE_ARITHMETIC(div)
DEFINE_ARITHMETIC(mod)

