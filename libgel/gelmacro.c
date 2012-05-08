#include <gelmacro.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelparser.h>


struct _GelMacro
{
    gchar *name;
    GList *args;
    gchar *variadic;
    GelValueArray *code;
};


static GHashTable *macros_HASH = NULL;


GelMacro* gel_macro_new(const gchar *name,
                        GList *args, gchar *variadic, GelValueArray *code)
{
    GelMacro *self = g_slice_new0(GelMacro);

    self->name = g_strdup(name);
    self->args = args;
    self->variadic = variadic;
    self->code = code;

    return self;
}


void gel_macro_free(GelMacro *self)
{
    g_free(self->name);
    g_list_foreach(self->args, (GFunc)g_free, NULL);
    g_list_free(self->args);
    g_free(self->variadic);
    gel_value_array_free(self->code);
    g_slice_free(GelMacro, self);
}


void gel_macros_new(void)
{
    g_return_if_fail(macros_HASH == NULL);

    macros_HASH = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)gel_macro_free);
}


void gel_macros_free(void)
{
    g_return_if_fail(macros_HASH != NULL);

    g_hash_table_unref(macros_HASH);
    macros_HASH = NULL;
}


static
void gel_macros_insert(const gchar *name, GList *args, gchar *variadic,
                       GelValueArray *code)
{
    g_return_if_fail(macros_HASH != NULL);

    g_hash_table_insert(macros_HASH,
        g_strdup(name), gel_macro_new(name, args, variadic, code));
}


GelMacro* gel_macros_lookup(const gchar *name)
{
    g_return_val_if_fail(macros_HASH != NULL, NULL);

    return g_hash_table_lookup(macros_HASH, name);
}


static
GelValueArray* gel_macro_map_code(const GelMacro *self,
                                 GHashTable *hash, GelValueArray *array,
                                 GError **error)
{
    GValue *values = gel_value_array_get_values(array);
    guint n_values = gel_value_array_get_n_values(array);
    GelValueArray *code = gel_value_array_new(n_values);

    for(guint i = 0; i < n_values; i++)
    {
        GType type = GEL_VALUE_TYPE(values + i);
        if(type == GEL_TYPE_SYMBOL)
        {
            GelSymbol *symbol = gel_value_get_boxed(values + i);
            const gchar *name = gel_symbol_get_name(symbol);
            GValue *value = g_hash_table_lookup(hash, name);

            if(g_strcmp0(name, self->variadic) != 0)
            {
                if(value != NULL)
                    gel_value_array_append(code, value);
                else
                    gel_value_array_append(code, values + i);
            }
            else
            if(GEL_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY))
            {
                GelValueArray *var_array = gel_value_get_boxed(value);
                guint var_n_values = gel_value_array_get_n_values(var_array);
                GValue *var_values = gel_value_array_get_values(var_array);

                for(guint i = 0; i < var_n_values; i++)
                    gel_value_array_append(code, var_values + i);
            }
        }
        else
        if(type == G_TYPE_VALUE_ARRAY)
        {
            GelValueArray *array = gel_value_get_boxed(values + i);
            GelValueArray *new_array =
                gel_macro_map_code(self, hash, array, error);

            GValue tmp_value = {0};
            g_value_init(&tmp_value, G_TYPE_VALUE_ARRAY);

            gel_value_take_boxed(&tmp_value, new_array);
            gel_value_array_append(code, &tmp_value);

            g_value_unset(&tmp_value);
        }
        else
            gel_value_array_append(code, values + i);
    }

    return code;
}


static
GelValueArray* gel_macro_invoke(const GelMacro *self,
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
                GEL_PARSE_ERROR, GEL_PARSE_ERROR_MACRO_ARGUMENTS,
                "Macro %s: requires at least %u arguments, got %u",
                self->name, n_args, n_values));
            return NULL;
        }
    }
    else
    if(n_values != n_args)
    {
        g_propagate_error(error, g_error_new(
            GEL_PARSE_ERROR, GEL_PARSE_ERROR_MACRO_ARGUMENTS,
            "Macro %s: requires %u arguments, got %u",
            self->name, n_args, n_values));
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
        GelValueArray *array = gel_value_array_new(array_n_values);

        for(guint j = 0; i < n_values; i++, j++)
            gel_value_array_append(array, values + i);

        GValue *value = gel_value_new_from_boxed(G_TYPE_VALUE_ARRAY, array);
        g_hash_table_insert(mappings, self->variadic, value);
    }

    GelValueArray *code =
        gel_macro_map_code(self, mappings, self->code, error);
    g_hash_table_unref(mappings);

    return code;
}


GelValueArray* gel_macro_code_from_value(GValue *pre_value, GError **error)
{
    if(GEL_VALUE_TYPE(pre_value) == G_TYPE_VALUE_ARRAY)
    {
        GelValueArray *array = gel_value_get_boxed(pre_value);
        GValue *values = gel_value_array_get_values(array);
        guint n_values = gel_value_array_get_n_values(array);

        GType type = GEL_VALUE_TYPE(values + 0);
        if(type != GEL_TYPE_SYMBOL)
            return NULL;

        GelSymbol *symbol = gel_value_get_boxed(values + 0);
        const gchar *name = gel_symbol_get_name(symbol);

        n_values--;
        values++;

        if(g_strcmp0(name, "macro") == 0)
        {
            if(n_values < 2)
            {
                g_propagate_error(error, g_error_new(
                    GEL_PARSE_ERROR, GEL_PARSE_ERROR_MACRO_MALFORMED,
                    "Macro: expected arguments and code"));
                return NULL;
            }

            symbol = gel_value_get_boxed(values + 0);
            name = gel_symbol_get_name(symbol);

            GelValueArray *vars = gel_value_get_boxed(values + 1);
            gchar *variadic = NULL;
            gchar *invalid = NULL;
            GList* args = gel_args_from_array(vars, &variadic, &invalid);

            if(invalid != NULL)
            {
                g_propagate_error(error, g_error_new(
                    GEL_PARSE_ERROR, GEL_PARSE_ERROR_MACRO_MALFORMED,
                    "Macro: '%s' is an invalid argument name", invalid));
                g_free(invalid);
                return NULL;
            }

            n_values -= 2;
            values += 2;

            GelValueArray *code = gel_value_array_new(n_values);
            for(guint i = 0; i < n_values; i++)
                gel_value_array_append(code, values + i);

            gel_macros_insert(name, args, variadic, code);
            return gel_value_array_new(0);
        }
        else
        {
            GelMacro *macro = gel_macros_lookup(name);
            if(macro != NULL)
                return gel_macro_invoke(macro, n_values, values, error);
        }
    }

    return NULL;
}

