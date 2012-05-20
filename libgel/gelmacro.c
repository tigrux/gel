#include <gelmacro.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelparser.h>


struct _GelMacro
{
    gchar *name;
    GList *args;
    gchar *variadic;
    GelArray *code;
};


GelMacro* gel_macro_new(GList *args, gchar *variadic, GelArray *code)
{
    GelMacro *self = g_slice_new0(GelMacro);

    self->args = args;
    self->variadic = variadic;
    self->code = code;

    return self;
}


void gel_macro_free(GelMacro *self)
{
    g_list_foreach(self->args, (GFunc)g_free, NULL);
    g_list_free(self->args);
    g_free(self->variadic);
    gel_array_free(self->code);
    g_slice_free(GelMacro, self);
}


static
GelArray* gel_macro_map_code(const GelMacro *self,
                             GHashTable *mappings, GelArray *array,
                             GError **error)
{
    GValue *values = gel_array_get_values(array);
    guint n_values = gel_array_get_n_values(array);
    GelArray *code = gel_array_new(n_values);

    for(guint i = 0; i < n_values; i++)
    {
        GType type = G_VALUE_TYPE(values + i);
        if(type == GEL_TYPE_SYMBOL)
        {
            GelSymbol *symbol = g_value_get_boxed(values + i);
            const gchar *name = gel_symbol_get_name(symbol);
            GValue *value = g_hash_table_lookup(mappings, name);

            if(g_strcmp0(name, self->variadic) != 0)
            {
                if(value != NULL)
                    gel_array_append(code, value);
                else
                    gel_array_append(code, values + i);
            }
            else
            if(G_VALUE_HOLDS(value, GEL_TYPE_ARRAY))
            {
                GelArray *var_array = g_value_get_boxed(value);
                guint var_n_values = gel_array_get_n_values(var_array);
                GValue *var_values = gel_array_get_values(var_array);

                for(guint i = 0; i < var_n_values; i++)
                    gel_array_append(code, var_values + i);
            }
        }
        else
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = g_value_get_boxed(values + i);
            GelArray *new_array =
                gel_macro_map_code(self, mappings, array, error);

            GValue tmp_value = {0};
            g_value_init(&tmp_value, GEL_TYPE_ARRAY);

            g_value_take_boxed(&tmp_value, new_array);
            gel_array_append(code, &tmp_value);

            g_value_unset(&tmp_value);
        }
        else
            gel_array_append(code, values + i);
    }

    return code;
}


GelArray* gel_macro_invoke(const GelMacro *self, const gchar *name,
                           guint n_values, const GValue *values,
                           GError **error)
{
    guint n_args = g_list_length(self->args);
    gboolean is_variadic = (self->variadic != NULL);

    if(is_variadic)
    {
        if(n_values < n_args)
        {
            g_propagate_error(error, g_error_new(
                GEL_PARSER_ERROR, GEL_PARSER_ERROR_MACRO_ARGUMENTS,
                "Macro %s: requires at least %u arguments, got %u",
                name, n_args, n_values));
            return NULL;
        }
    }
    else
    if(n_values != n_args)
    {
        g_propagate_error(error, g_error_new(
            GEL_PARSER_ERROR, GEL_PARSER_ERROR_MACRO_ARGUMENTS,
            "Macro %s: requires %u arguments, got %u",
            name, n_args, n_values));
        return NULL;
    }

    GHashTable *mappings = g_hash_table_new(g_str_hash, g_str_equal);

    guint i = 0;
    for(GList *iter = self->args; iter != NULL; iter = iter->next, i++)
    {
        const gchar *arg = iter->data;
        const GValue *value = values + i;
        g_hash_table_insert(mappings, (void* )arg, (void *)value);
    }

    if(is_variadic)
    {
        guint array_n_values = n_values - i;
        GelArray *array = gel_array_new(array_n_values);

        for(guint j = 0; i < n_values; i++, j++)
            gel_array_append(array, values + i);

        GValue *value = gel_value_new_from_boxed(GEL_TYPE_ARRAY, array);
        g_hash_table_insert(mappings, self->variadic, value);
    }

    GelArray *code =
        gel_macro_map_code(self, mappings, self->code, error);
    g_hash_table_unref(mappings);

    return code;
}

