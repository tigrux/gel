#include <glib-object.h>

#include <gelvalue.h>

#define DEFINE_ARITHMETIC(op) \
gboolean gel_values_##op(const GValue *l_value, const GValue *r_value, GValue *value) \
{ \
    return gel_values_arithmetic(l_value, r_value, value, gel_values_simple_##op); \
}


#define DEFINE_LOGIC(op) \
gboolean gel_values_##op(const GValue *l_value, const GValue *r_value) \
{ \
    return gel_values_logic(l_value, r_value, gel_values_simple_##op); \
}


GValue* gel_value_new(void)
{
    return g_slice_new0(GValue);
}


GValue* gel_value_new_of_type(GType type)
{
    g_return_val_if_fail(type != G_TYPE_INVALID, NULL);
    return g_value_init(gel_value_new(), type);
}


GValue* gel_value_new_from_closure(GClosure *closure)
{
    g_return_val_if_fail(closure != NULL, NULL);

    GValue *value = gel_value_new_of_type(G_TYPE_CLOSURE);
    g_value_set_boxed(value, closure);
    return value;
}


GValue* gel_value_new_closure_from_marshall(GClosureMarshal value_marshal,
                                            GObject *obj)
{
    g_return_val_if_fail(obj != NULL, NULL);
    g_return_val_if_fail(G_IS_OBJECT(obj), NULL);

    GClosure *closure = g_closure_new_object(sizeof(GClosure), obj);
    g_closure_set_marshal(closure, value_marshal);
    return gel_value_new_from_closure(closure);
}


GValue* gel_value_new_from_boolean(gboolean value_boolean)
{
    GValue *value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    g_value_set_boolean(value, value_boolean);
    return value;
}


GValue* gel_value_new_from_pointer(gpointer value_pointer)
{
    GValue *value = gel_value_new_of_type(G_TYPE_POINTER);
    g_value_set_pointer(value, value_pointer);
    return value;
}


GValue *gel_value_dup(const GValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(G_IS_VALUE(value), NULL);

    GValue *dup_value = gel_value_new_of_type(G_VALUE_TYPE(value));
    g_value_copy(value, dup_value);
    return dup_value;
}


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


void gel_value_free(GValue *value)
{
    g_return_if_fail(value != NULL);

    if(G_IS_VALUE(value))
        g_value_unset(value);
    g_slice_free(GValue, value);
}


gchar* gel_value_to_string(const GValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(G_IS_VALUE(value), NULL);

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


gboolean gel_value_to_boolean(const GValue *value)
{
    g_return_val_if_fail(value != NULL, FALSE);

    gboolean result;
    GValue bool_value = {0};
    g_value_init(&bool_value, G_TYPE_BOOLEAN);
    if(g_value_transform(value, &bool_value))
        result = g_value_get_boolean(&bool_value);
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
        if(g_value_fits_pointer(value))
            result = (g_value_peek_pointer(value) != NULL);
        else
            result = FALSE;
    }
    g_value_unset(&bool_value);

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
GType gel_values_simple_type(const GValue *l_value, const GValue *r_value)
{
    GType l_type = gel_value_simple_type(l_value);
    GType r_type = gel_value_simple_type(r_value);

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
gboolean gel_values_simple_add(const GValue *l_value, const GValue *r_value,
                               GValue *dest_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    GType type = G_VALUE_TYPE(dest_value);
    switch(type)
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(l_value)
                + g_value_get_long(r_value));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                g_value_get_double(l_value)
                + g_value_get_double(r_value));
            return TRUE;
        case G_TYPE_STRING:
            g_value_take_string(dest_value,
                g_strconcat(
                    g_value_get_string(l_value),
                    g_value_get_string(r_value),
                NULL));
            return TRUE;
        default:
            if(type == G_TYPE_VALUE_ARRAY)
            {
                GValueArray *l_array = (GValueArray*)g_value_get_boxed(l_value);
                GValueArray *r_array = (GValueArray*)g_value_get_boxed(r_value);
                const guint l_n_values = l_array->n_values;
                const guint r_n_values = r_array->n_values;

                GValueArray *array = g_value_array_new(l_n_values + r_n_values);
                register guint i;

                const GValue *const l_array_values = l_array->values;
                for(i = 0; i < l_n_values; i++)
                    g_value_array_append(array, l_array_values + i);

                const GValue *const r_array_values = r_array->values;
                for(i = 0; i < r_n_values; i++)
                    g_value_array_append(array, r_array_values + i);
                g_value_take_boxed(dest_value, array);
                return TRUE;
            }
    }
    return FALSE;
}


static
gboolean gel_values_simple_sub(const GValue *l_value, const GValue *r_value,
                               GValue *dest_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(l_value)
                - g_value_get_long(r_value));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                g_value_get_double(l_value)
                - g_value_get_double(r_value));
            return TRUE;
    }
    return FALSE;
}


static
gboolean gel_values_simple_mul(const GValue *l_value, const GValue *r_value,
                               GValue *dest_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(l_value)
                * g_value_get_long(r_value));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                g_value_get_double(l_value)
                * g_value_get_double(r_value));
            return TRUE;
    }
    return FALSE;
}


static
gboolean gel_values_simple_div(const GValue *l_value, const GValue *r_value,
                              GValue *dest_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(l_value)
                / g_value_get_long(r_value));
            return TRUE;
    }
    return FALSE;
}


static
gboolean gel_values_simple_mod(const GValue *l_value, const GValue *r_value,
                               GValue *dest_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    switch(G_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            g_value_set_long(dest_value,
                g_value_get_long(l_value)
                % g_value_get_long(r_value));
            return TRUE;
        case G_TYPE_DOUBLE:
            g_value_set_double(dest_value,
                (long)g_value_get_double(l_value)
                % (long)g_value_get_double(r_value));
            return TRUE;
    }
    return FALSE;
}


static
gboolean gel_values_arithmetic(const GValue *l_value, const GValue *r_value,
                               GValue *dest_value,
                               GelValuesArithmetic values_function)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    GType dest_type = gel_values_simple_type(l_value, r_value);
    g_return_val_if_fail(dest_type != G_TYPE_INVALID, FALSE);

    if(G_IS_VALUE(dest_value))
        g_value_unset(dest_value);
    g_value_init(dest_value, dest_type);

    GValue ll_value = {0};
    g_value_init(&ll_value, dest_type);

    GValue rr_value = {0};
    g_value_init(&rr_value, dest_type);

    gboolean result;
    if(g_value_transform(l_value, &ll_value)
       && g_value_transform(r_value, &rr_value))
        result = values_function(&ll_value, &rr_value, dest_value);
    else
        result = FALSE;
    g_value_unset(&ll_value);
    g_value_unset(&rr_value);
    return result;
}


gboolean gel_values_can_compare(const GValue *l_value, const GValue *r_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    switch(gel_values_simple_type(l_value, r_value))
    {
        case G_TYPE_LONG:
        case G_TYPE_DOUBLE:
        case G_TYPE_STRING:
        case G_TYPE_POINTER:
        case G_TYPE_BOOLEAN:
            return TRUE;
        default:
            g_warning("Values cannot be compared");
            return FALSE;
    }
}


gint gel_values_compare(const GValue *l_value, const GValue *r_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    switch(gel_values_simple_type(l_value, r_value))
    {
        case G_TYPE_LONG:
        {
            glong l_long = g_value_get_long(l_value);
            glong r_long = g_value_get_long(r_value);
            return l_long > r_long ? 1 : l_long < r_long ? -1 : 0;
        }
        case G_TYPE_DOUBLE:
        {
            glong l_double = g_value_get_double(l_value);
            glong r_double = g_value_get_double(r_value);
            return l_double > r_double ? 1 : l_double < r_double ? -1 : 0;
        }
        case G_TYPE_BOOLEAN:
        {
            glong l_bool = g_value_get_boolean(l_value);
            glong r_bool = g_value_get_boolean(r_value);
            return l_bool > r_bool ? 1 : l_bool < r_bool ? -1 : 0;
        }
        case G_TYPE_STRING:
        {
            return g_strcmp0(
                g_value_get_string(l_value), g_value_get_string(r_value));
        }
        case G_TYPE_POINTER:
        {
            gpointer l_pointer = g_value_peek_pointer(l_value);
            gpointer r_pointer = g_value_peek_pointer(r_value);
            return l_pointer > r_pointer ? 1 : l_pointer < r_pointer ? -1 : 0;
        }
        default:
            return -1;
    }
}


static
gboolean gel_values_simple_gt(const GValue *l_value, const GValue *r_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    if(gel_values_can_compare(l_value, r_value))
        return gel_values_compare(l_value, r_value) > 0;
    return FALSE;
}


static
gboolean gel_values_simple_ge(const GValue *l_value, const GValue *r_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    if(gel_values_can_compare(l_value, r_value))
        return gel_values_compare(l_value, r_value) >= 0;
    return FALSE;
}


static
gboolean gel_values_simple_eq(const GValue *l_value, const GValue *r_value)
{
    if(gel_values_can_compare(l_value, r_value))
        return gel_values_compare(l_value, r_value) == 0;
    return FALSE;
}


static
gboolean gel_values_simple_le(const GValue *l_value, const GValue *r_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    if(gel_values_can_compare(l_value, r_value))
        return gel_values_compare(l_value, r_value) <= 0;
    return FALSE;
}


static
gboolean gel_values_simple_lt(const GValue *l_value, const GValue *r_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    if(gel_values_can_compare(l_value, r_value))
        return gel_values_compare(l_value, r_value) < 0;
    return FALSE;
}


static
gboolean gel_values_simple_ne(const GValue *l_value, const GValue *r_value)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    if(gel_values_can_compare(l_value, r_value))
        return gel_values_compare(l_value, r_value) != 0;
    return FALSE;
}


static
gboolean gel_values_logic(const GValue *l_value, const GValue *r_value,
                          GelValuesLogic values_function)
{
    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    g_return_val_if_fail(l_value != NULL, FALSE);
    g_return_val_if_fail(r_value != NULL, FALSE);

    GType simple_type = gel_values_simple_type(l_value, r_value);
    g_return_val_if_fail(simple_type != G_TYPE_INVALID, FALSE);

    GValue ll_value = {0};
    g_value_init(&ll_value, simple_type);

    GValue rr_value = {0};
    g_value_init(&rr_value, simple_type);

    gboolean result;
    if(g_value_transform(l_value, &ll_value)
       && g_value_transform(r_value, &rr_value))
        result = values_function(&ll_value, &rr_value);
    else
        result = FALSE;
    g_value_unset(&ll_value);
    g_value_unset(&rr_value);
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

