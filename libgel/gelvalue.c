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


GValue* gel_value_new_from_boxed(GType type, void *boxed)
{
    GValue *value = gel_value_new_of_type(type);
    gel_value_take_boxed(value, boxed);

    return value;
}


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
 */
void gel_value_copy(const GValue *src_value, GValue *dest_value)
{
    g_return_if_fail(src_value != NULL);
    g_return_if_fail(dest_value != NULL);
    g_return_if_fail(GEL_IS_VALUE(src_value));

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
            case G_TYPE_INT64:
                gel_value_set_int64(dest_value,
                    gel_value_get_int64(src_value));
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
        if(!g_value_transform(src_value, dest_value))
        {
            if(GEL_IS_VALUE(dest_value))
                g_value_unset(dest_value);
            g_value_init(dest_value, src_type);
            g_value_copy(src_value, dest_value);
        }
}


void gel_value_free(GValue *value)
{
    g_return_if_fail(value != NULL);

    if(GEL_IS_VALUE(value))
        g_value_unset(value);
    g_free(value);
}


void gel_value_list_free(GList *value_list)
{
    g_list_foreach(value_list, (GFunc)gel_value_free, NULL);
    g_list_free(value_list);
}


static
gchar* gel_value_stringify(const GValue *value, gchar* (*str)(const GValue *))
{
    gchar *result = NULL;
    GValue string_value = {0};

    g_value_init(&string_value, G_TYPE_STRING);
    if(g_value_transform(value, &string_value))
    {
        result = g_value_dup_string(&string_value);
        g_value_unset(&string_value);
    }
    else
    if(GEL_VALUE_HOLDS(value, GEL_TYPE_VALUE_ARRAY))
    {
        const GelValueArray *array = gel_value_get_boxed(value);
        GString *buffer = g_string_new("(");
        const guint n_values = gel_value_array_get_n_values(array);

        if(n_values > 0)
        {
            guint last = n_values - 1;
            const GValue *array_values = gel_value_array_get_values(array);

            for(guint i = 0; i <= last; i++)
            {
                gchar *s = str(array_values + i);
                g_string_append_printf(buffer,
                    "%s%s", s, i != last ? " " : ")");
                g_free(s);
            }
        }
        else
            g_string_append_c(buffer, ')');

        result =  g_string_free(buffer, FALSE);
    }
    else
    if(GEL_VALUE_HOLDS(value, G_TYPE_HASH_TABLE))
    {
        GHashTable *hash = gel_value_get_boxed(value);
        GString *buffer = g_string_new("{");
        guint n_values = g_hash_table_size(hash);

        if(n_values > 0)
        {
            GHashTableIter iter;
            GValue *key;
            GValue *value;
            guint last = n_values - 1;
            guint i = 0;

            g_hash_table_iter_init(&iter, hash);
            while(g_hash_table_iter_next(&iter,
                    (void *)&key, (void *)&value))
            {
                gchar *ks = str(key);
                gchar *vs = str(value);
                g_string_append_printf(buffer,
                    "%s %s%s", ks, vs, i!=last ? " " : "}");
                g_free(ks);
                g_free(vs);
                i++;
            }
        }
        else
            g_string_append_c(buffer, '}');

        result =  g_string_free(buffer, FALSE);
    }
    else
    if(GEL_VALUE_HOLDS(value, GEL_TYPE_SYMBOL))
    {
        const GelSymbol *symbol = gel_value_get_boxed(value);
        const GValue *symbol_value = gel_symbol_get_value(symbol);

        if(symbol_value != NULL)
            result = str(symbol_value);
        else
            result = g_strdup(gel_symbol_get_name(symbol));
    }
    else
    if(GEL_VALUE_HOLDS(value, G_TYPE_CLOSURE))
    {
        const GClosure *closure = gel_value_get_boxed(value);
        const gchar *name = gel_closure_get_name(closure);

        if(name != NULL)
            result = g_strdup(name);
    }
    else
    if(GEL_VALUE_HOLDS(value, G_TYPE_GTYPE))
        result = g_strdup(g_type_name(gel_value_get_gtype(value)));

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
 * gel_value_repr:
 * @value: #GValue to make a string from
 *
 * Provides a representation of @value
 *
 * Returns: a newly allocated string
 */
gchar* gel_value_repr(const GValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(GEL_IS_VALUE(value), NULL);

    if(GEL_VALUE_HOLDS(value, G_TYPE_STRING))
        return g_strdup_printf("\"%s\"", gel_value_get_string(value));

    return gel_value_stringify(value, gel_value_repr);
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
    if(value == NULL)
        return g_strdup("NULL");

    if(!GEL_IS_VALUE(value))
        return g_strdup("VOID");

    if(GEL_VALUE_HOLDS(value, G_TYPE_STRING))
        return g_strdup(gel_value_get_string(value));

    return gel_value_stringify(value, gel_value_to_string);
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
        case G_TYPE_INT64:
            result = (gel_value_get_int64(value) != 0);
            break;
        case G_TYPE_DOUBLE:
            result = (gel_value_get_double(value) != 0.0);
            break;
        default:
            if(type == G_TYPE_ARRAY)
            {
                GelValueArray *array = gel_value_get_boxed(value);
                result =
                    (array != NULL && gel_value_array_get_n_values(array) != 0);
                break;
            }
            else
            if(type == G_TYPE_HASH_TABLE)
            {
                GHashTable *hash = gel_value_get_boxed(value);
                result = (hash != NULL && g_hash_table_size(hash) != 0);
                break;
            }
            else
            if(g_value_fits_pointer(value))
            {
                result = (gel_value_peek_pointer(value) != NULL);
                break;
            }
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
        case G_TYPE_INT:
        case G_TYPE_UINT:
        case G_TYPE_ULONG:
        case G_TYPE_LONG:
        case G_TYPE_INT64:
        case G_TYPE_UINT64:
            return G_TYPE_INT64;
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
                return G_TYPE_INT64;
            if(GEL_VALUE_HOLDS(value, GEL_TYPE_VALUE_ARRAY))
                return GEL_TYPE_VALUE_ARRAY;
            if(GEL_VALUE_HOLDS(value, G_TYPE_HASH_TABLE))
                return G_TYPE_HASH_TABLE;
            if(g_value_fits_pointer(value))
                return G_TYPE_POINTER;

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

    if(l_type == G_TYPE_DOUBLE && r_type == G_TYPE_INT64)
        return G_TYPE_DOUBLE;

    if(r_type == G_TYPE_DOUBLE && l_type == G_TYPE_INT64)
        return G_TYPE_DOUBLE;

    return G_TYPE_INVALID;
}


static
guint gel_value_hash(const GValue *value)
{
    GType type = GEL_VALUE_TYPE(value);
    switch(type)
    {
        case G_TYPE_STRING:
            return g_str_hash(gel_value_get_string(value));
        default:
            return value->data[0].v_uint;
    }
}


GHashTable* gel_hash_table_new(void)
{
    GHashTable *hash = g_hash_table_new_full(
            (GHashFunc)gel_value_hash, (GEqualFunc)gel_values_eq,
            (GDestroyNotify)gel_value_free, (GDestroyNotify)gel_value_free);

    return hash;
}


GList* gel_args_from_array(const GelValueArray *vars, gchar **variadic,
                           gchar **invalid)
{
    gboolean next_is_variadic = FALSE;
    GList *args = NULL;
    gboolean failed = FALSE;

    guint n_vars = gel_value_array_get_n_values(vars);
    const GValue *var_values = gel_value_array_get_values(vars);

    for(guint i = 0; i < n_vars; i++)
    {
        const GValue *value = var_values + i;
        if(GEL_VALUE_HOLDS(value, GEL_TYPE_SYMBOL))
        {
            GelSymbol *symbol = gel_value_get_boxed(value);
            const gchar *arg = gel_symbol_get_name(symbol);

            if(*variadic == NULL)
            {
                if(g_strcmp0(arg, "&") == 0)
                {
                    next_is_variadic = TRUE;
                    arg = NULL;
                }
                else
                if(next_is_variadic)
                {
                    *variadic = g_strdup(arg);
                    arg = NULL;
                }
            }

            if(arg != NULL)
                args = g_list_append(args, g_strdup(arg));
        }
        else
        {
            *invalid = gel_value_repr(value);
            failed = TRUE;
            break;
        }
    }

    if(failed)
    {
        g_list_foreach(args, (GFunc)g_free, NULL);
        g_list_free(args);
        args = NULL;
    }

    return args;
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
        case G_TYPE_INT64:
            gel_value_set_int64(dest_value,
                gel_value_get_int64(v1)
                + gel_value_get_int64(v2));
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
            if(type == GEL_TYPE_VALUE_ARRAY)
            {
                GelValueArray *a1 = gel_value_get_boxed(v1);
                GelValueArray *a2 = gel_value_get_boxed(v2);
                guint n1_values = gel_value_array_get_n_values(a1);
                guint n2_values = gel_value_array_get_n_values(a2);

                GelValueArray *array =
                    gel_value_array_new(n1_values + n2_values);

                const GValue *a1_values = gel_value_array_get_values(a1);
                for(guint i = 0; i < n1_values; i++)
                    gel_value_array_append(array, a1_values + i);

                const GValue *a2_values = gel_value_array_get_values(a2);
                for(guint i = 0; i < n2_values; i++)
                    gel_value_array_append(array, a2_values + i);

                gel_value_take_boxed(dest_value, array);
                return TRUE;
            }
            else
            if(type == G_TYPE_HASH_TABLE)
            {
                GHashTable *h1 = gel_value_get_boxed(v1);
                GHashTable *h2 = gel_value_get_boxed(v2);
                GHashTable *hash = gel_hash_table_new();

                GHashTableIter iter;
                GValue *k = NULL;
                GValue *v = NULL;

                g_hash_table_iter_init(&iter, h1);
                while(g_hash_table_iter_next(&iter, (void**)&k, (void**)&v))
                    g_hash_table_insert(hash,
                        gel_value_dup(k), gel_value_dup(v));

                g_hash_table_iter_init(&iter, h2);
                while(g_hash_table_iter_next(&iter, (void**)&k, (void**)&v))
                    g_hash_table_insert(hash,
                        gel_value_dup(k), gel_value_dup(v));

                gel_value_take_boxed(dest_value, hash);
                return TRUE;
            }
            return FALSE;
    }
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
        case G_TYPE_INT64:
            gel_value_set_int64(dest_value,
                gel_value_get_int64(v1)
                - gel_value_get_int64(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                gel_value_get_double(v1)
                - gel_value_get_double(v2));
            return TRUE;
        default:
            return FALSE;
    }
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
        case G_TYPE_INT64:
            gel_value_set_int64(dest_value,
                gel_value_get_int64(v1)
                * gel_value_get_int64(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                gel_value_get_double(v1)
                * gel_value_get_double(v2));
            return TRUE;
        default:
            return FALSE;
    }
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
        case G_TYPE_INT64:
        {
            gint64 divisor = gel_value_get_int64(v2);
            if(divisor == 0)
                return FALSE;
            gel_value_set_int64(dest_value, gel_value_get_int64(v1) / divisor);
            return TRUE;
        }
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                gel_value_get_double(v1) / gel_value_get_double(v2));
            return TRUE;
        default:
            return FALSE;
    }
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
        case G_TYPE_INT64:
            gel_value_set_int64(dest_value,
                gel_value_get_int64(v1)
                % gel_value_get_int64(v2));
            return TRUE;
        case G_TYPE_DOUBLE:
            gel_value_set_double(dest_value,
                (gint64)gel_value_get_double(v1)
                % (gint64)gel_value_get_double(v2));
            return TRUE;
        default:
            return FALSE;
    }
}


static
GType gel_values_simple_transform(const GValue *v1, const GValue *v2,
                                  GValue *vv1, GValue *vv2,
                                  const GValue **vvv1, const GValue **vvv2)
{
    g_return_val_if_fail(v1 != NULL, FALSE);
    g_return_val_if_fail(v2 != NULL, FALSE);

    GType simple_type = gel_values_simple_type(v1, v2);

    if(simple_type == G_TYPE_INVALID)
        return G_TYPE_INVALID;

    if(GEL_VALUE_TYPE(v1) == simple_type && GEL_VALUE_TYPE(v2) == simple_type)
    {
        *vvv1 = v1;
        *vvv2 = v2;
        return simple_type;
    }


    g_value_init(vv1, simple_type);
    g_value_init(vv2, simple_type);

    GType result = G_TYPE_INVALID;
    if(!g_value_transform(v1, vv1))
        g_value_unset(vv1);
    else
    if(g_value_transform(v2, vv2))
        result = simple_type;
    else
    {
        g_value_unset(vv1);
        g_value_unset(vv2);
    }

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
        case G_TYPE_INT64:
        case G_TYPE_DOUBLE:
        case G_TYPE_STRING:
        case G_TYPE_POINTER:
        case G_TYPE_BOOLEAN:
            return TRUE;
        default:
            if(type == GEL_TYPE_VALUE_ARRAY)
                return TRUE;
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

    gboolean result = FALSE;
    GType dest_type =
        gel_values_simple_transform(v1, v2, &tmp1, &tmp2, &vv1, &vv2);

    if(dest_type != G_TYPE_INVALID)
    {
        g_value_init(dest_value, dest_type);
        result = values_function(vv1, vv2, dest_value);

        if(GEL_IS_VALUE(&tmp1))
            g_value_unset(&tmp1);
        if(GEL_IS_VALUE(&tmp2))
            g_value_unset(&tmp2);
    }

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

    if(simple_type == G_TYPE_INVALID)
        return FALSE;

    gboolean result = -1;
    switch(simple_type)
    {
        case G_TYPE_INT64:
        {
            gint64 i1 = gel_value_get_int64(vv1);
            gint64 i2 = gel_value_get_int64(vv2);
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
            
            void *p1 = gel_value_peek_pointer(vv1);
            void *p2 = gel_value_peek_pointer(vv2);

            result = p1 > p2 ? 1 : p1 < p2 ? -1 : 0;
            break;
        }
        default:
            if(simple_type == GEL_TYPE_VALUE_ARRAY)
            {
                GelValueArray *a1 = gel_value_get_boxed(vv1);
                GelValueArray *a2 = gel_value_get_boxed(vv2);

                guint a1_n = gel_value_array_get_n_values(a1);
                guint a2_n = gel_value_array_get_n_values(a2);
                guint n_values = MIN(a1_n, a2_n);

                GValue *a1_values = gel_value_array_get_values(a1);
                GValue *a2_values = gel_value_array_get_values(a2);

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
gint gel_values_logic(const GValue *v1, const GValue *v2,
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

    if(dest_type == G_TYPE_INVALID)
        return -1;

    gint result = values_function(vv1, vv2) ? 1 : 0;

    if(GEL_IS_VALUE(&tmp1))
        g_value_unset(&tmp1);
    if(GEL_IS_VALUE(&tmp2))
        g_value_unset(&tmp2);

    return result;
}


#define DEFINE_LOGIC(op) \
gint gel_values_##op(const GValue *v1, const GValue *v2) \
{ \
    return gel_values_logic(v1, v2, gel_values_simple_##op); \
}

/**
 * gel_values_gt:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 *
 * Checks if @v1 > @v2
 *
 * Returns: 1 if true, 0 if false, or -1 if the values cannot be compared
 */
DEFINE_LOGIC(gt)

/**
 * gel_values_ge:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 *
 * Checks if @v1 >= @v2
 *
 * Returns: 1 if true, 0 if false, or -1 if the values cannot be compared
 */
DEFINE_LOGIC(ge)

/**
 * gel_values_eq:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 *
 * Checks if @v1 == @v2
 *
 * Returns: 1 if true, 0 if false, or -1 if the values cannot be compared
 */
DEFINE_LOGIC(eq)

/**
 * gel_values_le:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 *
 * Checks if @v1 <= @v2
 *
 * Returns: 1 if true, 0 if false, or -1 if the values cannot be compared
 */
DEFINE_LOGIC(le)

/**
 * gel_values_lt:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 *
 * Checks if @v1 < @v2
 *
 * Returns: 1 if true, 0 if false, or -1 if the values cannot be compared
 */
DEFINE_LOGIC(lt)

/**
 * gel_values_ne:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 *
 * Checks if @v1 != @v2
 *
 * Returns: 1 if true, 0 if false, or -1 if the values cannot be compared
 */
DEFINE_LOGIC(ne)


#define DEFINE_ARITHMETIC(op) \
gboolean gel_values_##op(const GValue *v1, const GValue *v2, GValue *dest_value) \
{ \
    return gel_values_arithmetic(v1, v2, dest_value, gel_values_simple_##op); \
}


/**
 * gel_values_add:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 * @dest_value: A pointer to #GValue to store the result of the operation
 *
 * Performs @dest_value = @v1 + @v2
 *
 * Returns: #TRUE if the operation was possible, #FALSE otherwise
 */
DEFINE_ARITHMETIC(add)

/**
 * gel_values_sub:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 * @dest_value: A pointer to #GValue to store the result of the operation
 *
 * Performs @dest_value = @v1 - @v2
 *
 * Returns: #TRUE if the operation was possible, #FALSE otherwise
 */
DEFINE_ARITHMETIC(sub)

/**
 * gel_values_mul:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 * @dest_value: A pointer to #GValue to store the result of the operation
 *
 * Performs @dest_value = @v1 * @v2
 *
 * Returns: #TRUE if the operation was possible, #FALSE otherwise
 */
DEFINE_ARITHMETIC(mul)

/**
 * gel_values_div:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 * @dest_value: A pointer to #GValue to store the result of the operation
 *
 * Performs @dest_value = @v1 / @v2
 *
 * Returns: #TRUE if the operation was possible, #FALSE otherwise
 */
DEFINE_ARITHMETIC(div)

/**
 * gel_values_mod:
 * @v1: A valid #GValue
 * @v2: A valid #GValue
 * @dest_value: A pointer to #GValue to store the result of the operation
 *
 * Performs @dest_value = @v1 % @v2
 *
 * Returns: #TRUE if the operation was possible, #FALSE otherwise
 */
DEFINE_ARITHMETIC(mod)

