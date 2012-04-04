#include <glib-object.h>

#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelclosure.h>


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


GValue *gel_value_dup(const GValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(GEL_IS_VALUE(value), NULL);

    GValue *dup_value = gel_value_new_of_type(GEL_VALUE_TYPE(value));
    gel_value_copy(value, dup_value);
    return dup_value;
}



/**
 * gel_value_copy:
 * @src_value: a valid #GValue
 * @dest_value: a #GValue
 *
 * Tries to copy @src_value to @dest_value.
 * If @dest_value is empty, #g_value_init and #g_value_copy are used.
 * If not, #g_value_transform is used.
 *
 * Returns: #TRUE on success , #FALSE otherwise
 */
gboolean gel_value_copy(const GValue *src_value, GValue *dest_value)
{
    g_return_val_if_fail(src_value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);
    g_return_val_if_fail(GEL_IS_VALUE(src_value), FALSE);

    gboolean result = TRUE;
    GType src_type = GEL_VALUE_TYPE(src_value);

    if(!GEL_IS_VALUE(dest_value))
        g_value_init(dest_value, src_type);

    GType dest_type = GEL_VALUE_TYPE(dest_value);
    if(src_type == dest_type)
        switch(src_type)
        {
            case G_TYPE_BOOLEAN:
                gel_value_set_boolean(dest_value,
                    gel_value_get_boolean(src_value));
                break;
            case G_TYPE_LONG:
                gel_value_set_long(dest_value,
                    gel_value_get_long(src_value));
                break;
            case G_TYPE_POINTER:
                gel_value_set_pointer(dest_value,
                    gel_value_peek_pointer(src_value));
                break;
            case G_TYPE_DOUBLE:
                gel_value_set_double(dest_value,
                    gel_value_get_double(src_value));
                break;
            default:
                g_value_copy(src_value, dest_value);
        }
    else
    if(!(result = g_value_transform(src_value, dest_value)))
        g_warning("Cannot assign from %s to %s",
            g_type_name(src_type), g_type_name(dest_type));

    return result;
}


void gel_value_free(GValue *value)
{
    g_return_if_fail(value != NULL);

    if(GEL_IS_VALUE(value))
        g_value_unset(value);
    g_free(value);
}


/**
 * gel_value_list_free:
 * @value_list: a #GList of #GValue
 *
 * Frees a #GList of #GValue.
 * This function is needed to free lists used by #gel_context_eval_params
 */
void gel_value_list_free(GList *value_list)
{
    g_list_foreach(value_list, (GFunc)gel_value_free, NULL);
    g_list_free(value_list);
}


/**
 * gel_value_to_string:
 * @value: #GValue to get its representation
 *
 * Provides a debug friendly representation of @value
 *
 * Returns: a newly allocated string
 */
gchar* gel_value_to_string(const GValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(GEL_IS_VALUE(value), NULL);

    if(GEL_VALUE_HOLDS(value, G_TYPE_STRING))
        return g_value_dup_string(value);

    gchar *result = NULL;
    GValue string_value = {0};

    g_value_init(&string_value, G_TYPE_STRING);
    if(g_value_transform(value, &string_value))
    {
        result = g_value_dup_string(&string_value);
        g_value_unset(&string_value);
    }
    else
    if(GEL_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY))
    {
        const GValueArray *array = (GValueArray*)gel_value_get_boxed(value);
        GString *buffer = g_string_new("(");
        const guint n_values = array->n_values;
        if(n_values > 0)
        {
            guint last = n_values - 1;
            const GValue *array_values = array->values;
            guint i;
            for(i = 0; i <= last; i++)
            {
                gchar *s = gel_value_to_string(array_values + i);
                g_string_append(buffer, s);
                if(i != last)
                    g_string_append_c(buffer, ' ');
                g_free(s);
            }
        }
        g_string_append_c(buffer, ')');
        result =  g_string_free(buffer, FALSE);
    }
    else
    if(GEL_VALUE_HOLDS(value, G_TYPE_HASH_TABLE))
    {
        GHashTable *hash = (GHashTable*)gel_value_get_boxed(value);
        GString *buffer = g_string_new("(");
        guint n_values = g_hash_table_size(hash);
        if(n_values > 0)
        {
            GHashTableIter iter;
            GValue *key;
            GValue *value;
            guint last = n_values - 1;
            guint i = 0;

            g_hash_table_iter_init(&iter, hash);
            while(g_hash_table_iter_next(&iter, (void*)&key, (void*)&value))
            {
                gchar *ks = gel_value_to_string(key);
                gchar *vs = gel_value_to_string(value);
                g_string_append_printf(buffer, "(%s %s)", ks, vs);
                if(i != last)
                    g_string_append_c(buffer, ' ');
                g_free(ks);
                g_free(vs);
                i++;
            }
        }
        g_string_append_c(buffer, ')');
        result =  g_string_free(buffer, FALSE);
    }
    else
    if(GEL_VALUE_HOLDS(value, GEL_TYPE_SYMBOL))
    {
        const GelSymbol *symbol = (GelSymbol*)gel_value_get_boxed(value);
        const GValue *symbol_value = gel_symbol_get_value(symbol);
        if(symbol_value != NULL)
            result = gel_value_to_string(symbol_value);
        else
            result = g_strdup(gel_symbol_get_name(symbol));
    }
    else
    if(GEL_VALUE_HOLDS(value, G_TYPE_CLOSURE))
    {
        const GClosure *closure = (GClosure*)gel_value_get_boxed(value);
        const gchar *name = gel_closure_get_name(closure);
        if(name != NULL)
            result = g_strdup(name);
    }

    if(result == NULL)
    {
        GString *repr_string = g_string_new("<");
        g_string_append_printf(repr_string, "%s", GEL_VALUE_TYPE_NAME(value));

        if(GEL_VALUE_HOLDS(value, G_TYPE_GTYPE))
            g_string_append_printf(
                repr_string, " %s", g_type_name(g_value_get_gtype(value)));
        else
        if(g_value_fits_pointer(value))
            g_string_append_printf(
                repr_string, " %p", gel_value_peek_pointer(value));

        g_string_append_c(repr_string, '>');
        result = g_string_free(repr_string, FALSE);
    }

    return result;
}



/**
 * gel_value_to_boolean:
 * @value: #GValue to evaluate as #gboolean
 *
 * Evaluates @value as #gboolean
 *
 * An invalid value is #FALSE
 * A string is #FALSE if it is empty.
 * A number is #FALSE if it is zero.
 * An array is #FALSE if it does not have elements.
 * A pointer, object or boxed is #FALSE if it is #NULL.
 *
 * Returns: #FALSE if @value is empty or zero, #TRUE otherwise
 */
gboolean gel_value_to_boolean(const GValue *value)
{
    g_return_val_if_fail(value != NULL, FALSE);

    if(!GEL_IS_VALUE(value))
        return FALSE;

    gboolean result = TRUE;

    GType type = GEL_VALUE_TYPE(value);
    switch(type)
    {
        case G_TYPE_STRING:
            result = (
                gel_value_get_string(value) != NULL
                && g_strcmp0(gel_value_get_string(value), "") != 0);
            break;
        case G_TYPE_BOOLEAN:
            result = (gel_value_get_boolean(value) != FALSE);
            break;
        case G_TYPE_LONG:
            result = (gel_value_get_long(value) != 0);
            break;
        case G_TYPE_DOUBLE:
            result = (gel_value_get_double(value) != 0.0);
            break;
        default:
            if(type == G_TYPE_ARRAY)
            {
                GValueArray *array = (GValueArray*)gel_value_get_boxed(value);
                result = (array != NULL && array->n_values != 0);
                break;
            }
            if(g_value_fits_pointer(value))
            {
                result = (gel_value_peek_pointer(value) != NULL);
                break;
            }
            g_warning("Could not convert %s to gboolean", g_type_name(type));
    }
    return result;
}


static
GType gel_value_simple_type(const GValue *value)
{
    g_return_val_if_fail(value != NULL, G_TYPE_INVALID);

    GType type = GEL_VALUE_TYPE(value);
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
            if(GEL_VALUE_HOLDS(value, G_TYPE_ENUM)
               || GEL_VALUE_HOLDS(value, G_TYPE_FLAGS))
                return G_TYPE_LONG;
            if(GEL_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY))
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

    GType type = GEL_VALUE_TYPE(dest_value);
    switch(type)
    {
        case G_TYPE_LONG:
            gel_value_set_long(dest_value,
                gel_value_get_long(v1)
                + gel_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                gel_value_get_double(v1)
                + gel_value_get_double(v2));
            return TRUE;
        case G_TYPE_STRING:
            g_value_take_string(dest_value,
                g_strconcat(
                    gel_value_get_string(v1),
                    gel_value_get_string(v2),
                NULL));
            return TRUE;
        default:
            if(type == G_TYPE_VALUE_ARRAY)
            {
                GValueArray *a1 = (GValueArray*)gel_value_get_boxed(v1);
                GValueArray *a2 = (GValueArray*)gel_value_get_boxed(v2);
                guint n1_values = a1->n_values;
                guint n2_values = a2->n_values;

                GValueArray *array = g_value_array_new(n1_values + n2_values);

                const GValue *a1_values = a1->values;
                guint i;
                for(i = 0; i < n1_values; i++)
                    g_value_array_append(array, a1_values + i);

                const GValue *a2_values = a2->values;
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

    switch(GEL_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            gel_value_set_long(dest_value,
                gel_value_get_long(v1)
                - gel_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                gel_value_get_double(v1)
                - gel_value_get_double(v2));
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

    switch(GEL_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            gel_value_set_long(dest_value,
                gel_value_get_long(v1)
                * gel_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                gel_value_get_double(v1)
                * gel_value_get_double(v2));
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

    switch(GEL_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            gel_value_set_long(dest_value,
                gel_value_get_long(v1)
                / gel_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                gel_value_get_double(v1)
                / gel_value_get_double(v2));
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

    switch(GEL_VALUE_TYPE(dest_value))
    {
        case G_TYPE_LONG:
            gel_value_set_long(dest_value,
                gel_value_get_long(v1)
                % gel_value_get_long(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                (long)gel_value_get_double(v1)
                % (long)gel_value_get_double(v2));
            return TRUE;
    }
    return FALSE;
}


static
GType gel_values_simple_transform(const GValue *v1, const GValue *v2,
                                  GValue *vv1, GValue *vv2,
                                  const GValue **vvv1, const GValue **vvv2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    GType simple_type = gel_values_simple_type(v1, v2);
    g_return_val_if_fail(simple_type != G_TYPE_INVALID, FALSE);

    if(GEL_VALUE_TYPE(v1) == simple_type && GEL_VALUE_TYPE(v2) == simple_type)
    {
        *vvv1 = v1;
        *vvv2 = v2;
        return simple_type;
    }


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

    *vvv1 = vv1;
    *vvv2 = vv2;

    return result;
}


static
gboolean gel_values_can_cmp(const GValue *v1, const GValue *v2)
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

    if(gel_values_can_cmp(v1, v2))
        return gel_values_cmp(v1, v2) > 0;
    return FALSE;
}


static
gboolean gel_values_simple_ge(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_cmp(v1, v2))
        return gel_values_cmp(v1, v2) >= 0;
    return FALSE;
}


static
gboolean gel_values_simple_eq(const GValue *v1, const GValue *v2)
{
    if(gel_values_can_cmp(v1, v2))
        return gel_values_cmp(v1, v2) == 0;
    return FALSE;
}


static
gboolean gel_values_simple_le(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_cmp(v1, v2))
        return gel_values_cmp(v1, v2) <= 0;
    return FALSE;
}


static
gboolean gel_values_simple_lt(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_cmp(v1, v2))
        return gel_values_cmp(v1, v2) < 0;
    return FALSE;
}


static
gboolean gel_values_simple_ne(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    if(gel_values_can_cmp(v1, v2))
        return gel_values_cmp(v1, v2) != 0;
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

    GValue tmp1 = {0};
    GValue tmp2 = {0};
    const GValue *vv1 = NULL;
    const GValue *vv2 = NULL;

    GType dest_type =
        gel_values_simple_transform(v1, v2, &tmp1, &tmp2, &vv1, &vv2);
    g_return_val_if_fail(dest_type != G_TYPE_INVALID, FALSE);

    g_value_init(dest_value, dest_type);
    gboolean result = values_function(vv1, vv2, dest_value);

    if(GEL_IS_VALUE(&tmp1))
        g_value_unset(&tmp1);
    if(GEL_IS_VALUE(&tmp2))
        g_value_unset(&tmp2);
    return result;
}


/**
 * gel_values_cmp:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 *
 * Compares @v1 and @v2
 *
 * Returns: 0 for v1 == v2, 1 for v1 > v2, -1 otherwise
 */
gint gel_values_cmp(const GValue *v1, const GValue *v2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    GValue tmp1 = {0};
    GValue tmp2 = {0};
    const GValue *vv1 = NULL;
    const GValue *vv2 = NULL;

    GType simple_type =
        gel_values_simple_transform(v1, v2, &tmp1, &tmp2, &vv1, &vv2);
    g_return_val_if_fail(simple_type != G_TYPE_INVALID, FALSE);

    gboolean result = -1;
    switch(simple_type)
    {
        case G_TYPE_LONG:
        {
            glong i1 = gel_value_get_long(vv1);
            glong i2 = gel_value_get_long(vv2);
            result = i1 > i2 ? 1 : i1 < i2 ? -1 : 0;
            break;
        }
        case G_TYPE_DOUBLE:
        {
            gdouble d1 = gel_value_get_double(vv1);
            gdouble d2 = gel_value_get_double(vv2);
            result = d1 > d2 ? 1 : d1 < d2 ? -1 : 0;
            break;
        }
        case G_TYPE_BOOLEAN:
        {
            gint b1 = gel_value_get_boolean(vv1);
            gint b2 = gel_value_get_boolean(vv2);
            result = b1 > b2 ? 1 : b1 < b2 ? -1 : 0;
            break;
        }
        case G_TYPE_STRING:
        {
            result = g_strcmp0(
                gel_value_get_string(vv1), gel_value_get_string(vv2));
            break;
        }
        case G_TYPE_POINTER:
        {
            
            gpointer p1 = gel_value_peek_pointer(vv1);
            gpointer p2 = gel_value_peek_pointer(vv2);
            result = p1 > p2 ? 1 : p1 < p2 ? -1 : 0;
            break;
        }
        default:
            if(simple_type == G_TYPE_VALUE_ARRAY)
            {
                GValueArray *a1 = (GValueArray*)gel_value_get_boxed(vv1);
                GValueArray *a2 = (GValueArray*)gel_value_get_boxed(vv2);
                guint a1_n = a1->n_values;
                guint a2_n = a2->n_values;
                guint n_values = MIN(a1_n, a2_n);

                GValue *a1_values = a1->values;
                GValue *a2_values = a2->values;
                guint i;
                for(i = 0; i < n_values; i++)
                {
                    result = gel_values_cmp(a1_values + i, a2_values + i);
                    if(result != 0)
                        break;
                }
                if(i == n_values)
                    result = a1_n > a2_n ? 1 : a1_n < a2_n ? -1 : 0;
            }
    }

    if(GEL_IS_VALUE(&tmp1))
        g_value_unset(&tmp1);
    if(GEL_IS_VALUE(&tmp2))
        g_value_unset(&tmp2);
    return result;
}


static
gboolean gel_values_logic(const GValue *v1, const GValue *v2,
                          GelValuesLogic values_function)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    GValue tmp1 = {0};
    GValue tmp2 = {0};
    const GValue *vv1 = NULL;
    const GValue *vv2 = NULL;

    GType dest_type =
        gel_values_simple_transform(v1, v2, &tmp1, &tmp2, &vv1, &vv2);
    g_return_val_if_fail(dest_type != G_TYPE_INVALID, FALSE);

    gboolean result = values_function(vv1, vv2);

    if(GEL_IS_VALUE(&tmp1))
        g_value_unset(&tmp1);
    if(GEL_IS_VALUE(&tmp2))
        g_value_unset(&tmp2);
    return result;
}


#define DEFINE_LOGIC(op) \
gboolean gel_values_##op(const GValue *v1, const GValue *v2) \
{ \
    return gel_values_logic(v1, v2, gel_values_simple_##op); \
}

DEFINE_LOGIC(gt)
DEFINE_LOGIC(ge)
DEFINE_LOGIC(eq)
DEFINE_LOGIC(le)
DEFINE_LOGIC(lt)
DEFINE_LOGIC(ne)


#define DEFINE_ARITHMETIC(op) \
gboolean gel_values_##op(const GValue *v1, const GValue *v2, GValue *value) \
{ \
    return gel_values_arithmetic(v1, v2, value, gel_values_simple_##op); \
}

DEFINE_ARITHMETIC(add)
DEFINE_ARITHMETIC(sub)
DEFINE_ARITHMETIC(mul)
DEFINE_ARITHMETIC(div)
DEFINE_ARITHMETIC(mod)

