#include <config.h>

#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <gelerrors.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelclosure.h>
#include <gelclosureprivate.h>

#ifdef HAVE_GOBJECT_INTROSPECTION
#include <geltypelib.h>
#endif


static
void symbol_set(GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GValue *dest_value = NULL;
    GValue tmp1_value = {0};
    GValue tmp2_value = {0};

    GType type = GEL_VALUE_TYPE(values + 0);
    if(type == GEL_TYPE_SYMBOL)
    {
        GelSymbol *symbol = gel_value_get_boxed(values + 0);
        dest_value = gel_symbol_get_value(symbol);
        if(dest_value == NULL)
        {
            const gchar *name = gel_symbol_get_name(symbol);
            dest_value = gel_context_lookup(context, name);
        }
    }
    else
    {
        const GValue *value =
            gel_context_eval_into_value(context, values + 0, &tmp1_value);

        if(gel_context_error(context))
            goto end;

        if(GEL_VALUE_HOLDS(value, GEL_TYPE_VARIABLE))
        {
            GelVariable *dest_variable = gel_value_get_boxed(value);
            dest_value = gel_variable_get_value(dest_variable);
        }
    }

    gboolean copied = FALSE;
    if(dest_value != NULL)
    {
        const GValue *src_value =
            gel_context_eval_param_into_value(context, values + 1, &tmp2_value);

        if(gel_context_error(context))
            goto end;

        if(src_value != NULL)
        {
            gel_value_copy(src_value, dest_value);
            copied = TRUE;
        }
    }

    if(!copied)
        gel_error_expected(context, __FUNCTION__, "symbol or variable");

    end:
    if(GEL_IS_VALUE(&tmp1_value))
        g_value_unset(&tmp1_value);

    if(GEL_IS_VALUE(&tmp2_value))
        g_value_unset(&tmp2_value);
}


static
void array_set(GelArray *array, GValue *return_value,
               guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    gint64 index = 0;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "IV", &index, &value))
    {
        guint array_n_values = gel_array_get_n_values(array);
        if(index < 0)
            index += array_n_values;
        if(index < 0 || index >= array_n_values)
        {
            gel_error_index_out_of_bounds(context, __FUNCTION__, index);
            return;
        }
        gel_value_copy(value, gel_array_get_values(array) + index);
    }

    gel_list_free(tmp_list);
}


static
void hash_set(GHashTable *hash, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValue *key = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "VV", &key, &value))
        g_hash_table_insert(hash, gel_value_dup(key), gel_value_dup(value));

    gel_list_free(tmp_list);
}


static
void object_set(GObject *object, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    gchar *name = NULL;
    GValue *value = NULL;
    GList *tmp_list = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "SV", &name, &value))
        if(G_IS_OBJECT(object))
        {
            GObjectClass *gclass = G_OBJECT_GET_CLASS(object);
            GParamSpec *spec = g_object_class_find_property(gclass, name);
            if(spec != NULL)
            {
                GValue result_value = {0};
                g_value_init(&result_value, spec->value_type);

                if(g_value_transform(value, &result_value))
                {
                    g_object_set_property(object, name, &result_value);
                    g_value_unset(&result_value);
                }
                else
                    gel_error_invalid_value_for_property(context,
                        __FUNCTION__, value, spec);
            }
            else
                gel_error_no_such_property(context, __FUNCTION__, name);
        }

    gel_list_free(tmp_list);
}


static
void array_get(GelArray *array, GValue *return_value,
               guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    gint64 index = 0;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "I", &index)) 
    {
        guint array_n_values = gel_array_get_n_values(array);
        if(index < 0)
            index += array_n_values;

        if(index < 0 || index >= array_n_values)
        {
            gel_error_index_out_of_bounds(context, __FUNCTION__, index);
            return;
        }

        gel_value_copy(gel_array_get_values(array) + index, return_value);
    }

    gel_list_free(tmp_list);
}


static
void hash_get(GHashTable *hash, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValue *key = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V", &key))
    {
        GValue *value = g_hash_table_lookup(hash, key);
        if(value != NULL)
            gel_value_copy(value, return_value);
        else
            gel_error_invalid_key(context, __FUNCTION__, key);
    }

    gel_list_free(tmp_list);
}


static
void object_get(GObject *object, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    const gchar *name = NULL;
    GList *tmp_list = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "S", &name))
        if(G_IS_OBJECT(object))
        {
            GObjectClass *gclass = G_OBJECT_GET_CLASS(object);
            GParamSpec *spec = g_object_class_find_property(gclass, name);

            if(spec != NULL)
            {
                g_value_init(return_value, spec->value_type);
                g_object_get_property(object, name, return_value);
            }
            else
                gel_error_no_such_property(context, __FUNCTION__, name);
        }

    gel_list_free(tmp_list);
}


static
void array_append(GelArray *array, GValue *return_value,
                  guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    for(guint i = 0; i < n_values; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);

        if(gel_context_error(context))
            goto end;

        gel_array_append(array, value);

        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    end:
    gel_list_free(tmp_list);
}


static
void hash_append(GHashTable *hash, GValue *return_value,
                 guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    if(n_values % 2 == 0)
        while(n_values > 0)
        {
            GValue *key = NULL;
            GValue *value = NULL;

            if(gel_context_eval_params(context, __FUNCTION__,
                    &n_values, &values, &tmp_list, "VV*", &key, &value))
                g_hash_table_insert(hash,
                    gel_value_dup(key), gel_value_dup(value));
            else
                break;
        }
    else
        gel_error_expected(context, __FUNCTION__, "an even number of values");

    gel_list_free(tmp_list);
}


static
void array_remove(GelArray *array, GValue *return_value,
                  guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    gint64 index = 0;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values,&tmp_list, "I", &index))
    {
        guint array_n_values = gel_array_get_n_values(array);
        if(index < 0)
            index += array_n_values;
        if(index < 0 || index >= array_n_values)
        {
            gel_error_index_out_of_bounds(context, __FUNCTION__, index);
            return;
        }

        gel_value_copy(gel_array_get_values(array) + index, return_value);
        gel_array_remove(array, index);
    }

    gel_list_free(tmp_list);
}


static
void hash_remove(GHashTable *hash, GValue *return_value,
                 guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValue *key = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V", &key))
    {
        GValue *value = g_hash_table_lookup(hash, key);
        if(value != NULL)
        {
            gel_value_copy(value, return_value); 
            g_hash_table_remove(hash, key);
        }
    }

    gel_list_free(tmp_list);
}


static
void array_size(GelArray *array, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    g_value_init(return_value, G_TYPE_INT64);
    gel_value_set_int64(return_value, gel_array_get_n_values(array));

    gel_list_free(tmp_list);
}


static
void hash_size(GHashTable *hash, GValue *return_value,
               guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    g_value_init(return_value, G_TYPE_INT64);
    guint result = g_hash_table_size(hash);
    gel_value_set_int64(return_value, result);

    gel_list_free(tmp_list);
}


static
void array_find(GClosure *closure, GelArray *array, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    const GValue *array_values = gel_array_get_values(array);
    guint array_n_values = gel_array_get_n_values(array);
    gint64 result = -1;

    for(guint i = 0; i < array_n_values && result == -1; i++)
    {
        GValue value = {0};
        g_closure_invoke(closure, &value, 1, array_values + i, context);
        if(gel_value_to_boolean(&value))
            result = i;
        g_value_unset(&value);
    }

    g_value_init(return_value, G_TYPE_INT64);
    gel_value_set_int64(return_value, result);

    gel_list_free(tmp_list);
}


static
void hash_find(GClosure *closure, GHashTable *hash, GValue *return_value,
               guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    GHashTableIter iter = {0};
    g_hash_table_iter_init(&iter, hash);

    const GValue *k = NULL;
    const GValue *v = NULL;
    gboolean running = TRUE;

    while(g_hash_table_iter_next(&iter, (void**)&k, (void**)&v) && running)
    {
        GValue value = {0};
        g_closure_invoke(closure, &value, 1, v, context);
        if(gel_value_to_boolean(&value))
        {
            gel_value_copy(k, return_value);
            running = FALSE;
        }
        g_value_unset(&value);
    }

    gel_list_free(tmp_list);
}


static
void array_filter(GClosure *closure, GelArray *array, GValue *return_value,
                  guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    guint array_n_values = gel_array_get_n_values(array);
    GValue *array_values = gel_array_get_values(array);
    GelArray *result_array = gel_array_new(array_n_values);

    for(guint i = 0; i < array_n_values; i++)
    {
        GValue tmp_value = {0};
        g_closure_invoke(closure,
            &tmp_value, 1, array_values + i, context);
        if(gel_value_to_boolean(&tmp_value))
            gel_array_append(result_array, array_values + i);
        g_value_unset(&tmp_value);
    }

    g_value_init(return_value, GEL_TYPE_ARRAY);
    gel_value_take_boxed(return_value, result_array);

    gel_list_free(tmp_list);
}


static
void hash_filter(GClosure *closure, GHashTable *hash, GValue *return_value,
                 guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;

    GHashTable *result_hash = gel_hash_table_new();
    GHashTableIter iter = {0};
    g_hash_table_iter_init(&iter, hash);

    const GValue *k = NULL;
    const GValue *v = NULL;

    while(g_hash_table_iter_next(&iter, (void**)&k, (void**)&v))
    {
        GValue value = {0};
        g_closure_invoke(closure, &value, 1, v, context);
        if(gel_value_to_boolean(&value))
            g_hash_table_insert(result_hash,
                gel_value_dup(k), gel_value_dup(v));
        g_value_unset(&value);
    }

    g_value_init(return_value, G_TYPE_HASH_TABLE);
    gel_value_take_boxed(return_value, result_hash);

    gel_list_free(tmp_list);
}


static
void def_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    gchar *name = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "sV", &name, &value))
    {
        if(gel_context_get_variable(context, name) != NULL)
            gel_error_symbol_exists(context, __FUNCTION__, name);
        else
            gel_context_define_value(context, name, gel_value_dup(value));
    }

    gel_list_free(tmp_list);
}


static
void defn_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    const gchar *name = NULL;
    const GelArray *vars = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "sa*", &name, &vars))
    {
        gchar *variadic = NULL;
        gchar *invalid = NULL;
        GList *args = gel_args_from_array(vars, &variadic, &invalid);

        if(invalid == NULL)
        {
            GClosure *closure =
                gel_closure_new(name, args, variadic,
                    n_values, values, context);

            GValue *value = gel_value_new_from_boxed(G_TYPE_CLOSURE, closure);

            gel_context_define_value(context, name, value);
            gel_closure_close_over(closure);
        }
        else
        {
            gel_error_invalid_argument_name(context, __FUNCTION__, invalid);
            g_free(invalid);
        }
    }
}


static
void fn_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    const GelArray *vars = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "a*", &vars))
    {
        gchar *variadic = NULL;
        gchar *invalid = NULL;
        GList *args = gel_args_from_array(vars, &variadic, &invalid);

        if(invalid == NULL)
        {
            GClosure *closure =
                gel_closure_new(NULL, args, variadic,
                    n_values, values, context);

            gel_closure_close_over(closure);

            g_value_init(return_value, G_TYPE_CLOSURE);
            g_value_set_boxed(return_value, closure);
        }
        else
        {
            gel_error_invalid_argument_name(context, __FUNCTION__, invalid);
            g_free(invalid);
        }
    }
}


static
void do_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    if(n_values < 1)
        return;

    guint last = n_values-1;
    for(guint i = 0; i <= last; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);

        if(gel_context_error(context))
            return;

        if(i == last && GEL_IS_VALUE(value))
            gel_value_copy(value, return_value);

        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
}


static
void let_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GelArray *bindings = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "a*", &bindings))
    {
        guint binding_n_values = gel_array_get_n_values(bindings);
        const GValue *binding_values = gel_array_get_values(bindings);
        
        if(binding_n_values % 2 == 0)
        {
            GelContext *let_context = gel_context_new_with_outer(context);
            gboolean failed = FALSE;

            while(binding_n_values > 0 && !failed)
            {
                const gchar *name = NULL;
                GValue *value = NULL;

                if(gel_context_eval_params(let_context, __FUNCTION__,
                        &binding_n_values, &binding_values,
                        &tmp_list, "sV*", &name, &value))
                    gel_context_define_value(let_context, name, gel_value_dup(value));
                else
                    failed = TRUE;
            }

            if(!failed)
                do_(self, return_value, n_values, values, let_context);

            if(gel_context_error(let_context))
                failed = TRUE;

            gel_context_free(let_context);
        }
        else
            gel_error_expected(context, __FUNCTION__,
                "an even number of values in bindings");
    }

    gel_list_free(tmp_list);
}


static
void eval_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    if(gel_context_eval_value(context, values + 0, &tmp_value))
    {
        gel_context_eval_value(context, &tmp_value, return_value);
        g_value_unset(&tmp_value);
    }
    else
        gel_value_copy(values + 0, return_value);
}


static
void quote_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    gel_value_copy(values + 0, return_value);
}


static
void name_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GType type = GEL_VALUE_TYPE(values + 0);
    if(type == GEL_TYPE_SYMBOL)
    {
        const GelSymbol *symbol = gel_value_get_boxed(values + 0);
        const gchar *name = gel_symbol_get_name(symbol);
        g_value_init(return_value, G_TYPE_STRING);
        g_value_set_string(return_value, name);
    }
    else
        gel_error_expected(context, __FUNCTION__, "symbol");
}


static
void apply_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;
    GelArray *array = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "CA", &closure, &array))
        g_closure_invoke(closure, return_value,
            gel_array_get_n_values(array),
            gel_array_get_values(array),
            context);

    gel_list_free(tmp_list);
}


static
void map_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;

    guint n_arrays = n_values - 1;
    GelArray **arrays = g_new0(GelArray *, n_arrays);
    guint32 result_n_values = G_MAXUINT;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "C*", &closure))
    {
        guint i_array = 0;
        while(n_values > 0)
            if(gel_context_eval_params(context, __FUNCTION__,
                    &n_values, &values, &tmp_list, "A*", &arrays[i_array]))
            {
                result_n_values =
                    MIN(result_n_values,
                        gel_array_get_n_values(arrays[i_array]));
                    
                i_array++;
            }
            else
                break;

        if(i_array == n_arrays)
        {
            GelArray *result_array = gel_array_new(result_n_values);

            for(guint iv = 0; iv < result_n_values; iv++)
            {
                GelArray *array = gel_array_new(n_arrays);
                for(guint ia = 0; ia < n_arrays; ia++)
                    gel_array_append(array,
                        gel_array_get_values(arrays[ia]) + iv);
                    

                GValue tmp_value = {0};
                g_closure_invoke(closure, &tmp_value,
                    gel_array_get_n_values(array),
                    gel_array_get_values(array),
                    context);

                if(GEL_IS_VALUE(&tmp_value))
                {
                    gel_array_append(result_array, &tmp_value);
                    g_value_unset(&tmp_value);
                }

                gel_array_free(array);
            }

            g_value_init(return_value, GEL_TYPE_ARRAY);
            gel_value_take_boxed(return_value, result_array);
        }
    }

    gel_list_free(tmp_list);
}


static
void array_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GelArray *array = gel_array_new(n_values > 8 ? n_values : 8);
    guint i;

    for(i = 0; i < n_values; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);

        if(gel_context_error(context))
            break;

        gel_array_append(array, value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    if(i == n_values)
    {
        g_value_init(return_value, GEL_TYPE_ARRAY);
        gel_value_take_boxed(return_value, array);
    }
}


static
void hash_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = gel_hash_table_new();

    if(n_values % 2 == 0)
        while(n_values > 0)
        {
            GValue *key = NULL;
            GValue *value = NULL;
            if(gel_context_eval_params(context, __FUNCTION__,
                    &n_values, &values, &tmp_list, "VV*", &key, &value))
                g_hash_table_insert(hash,
                    gel_value_dup(key), gel_value_dup(value));
            else
                break;
        }
    else
        gel_error_expected(context, __FUNCTION__, "an even number of values");

    if(n_values == 0)
    {
        g_value_init(return_value, G_TYPE_HASH_TABLE);
        gel_value_take_boxed(return_value, hash);
    }

    gel_list_free(tmp_list);
}


static
void var_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    if(GEL_VALUE_HOLDS(values, GEL_TYPE_SYMBOL))
    {
        GelSymbol *symbol = gel_value_get_boxed(values);
        GelVariable *variable = gel_symbol_get_variable(symbol);
        if(variable != NULL)
        {
            g_value_init(return_value, GEL_TYPE_VARIABLE);
            g_value_set_boxed(return_value, variable);
        }
        else
        {
            const gchar *name = gel_symbol_get_name(symbol);
            gel_error_unknown_symbol(context, __FUNCTION__, name);
        }
    }
    else
        gel_error_value_not_of_type(context,
            __FUNCTION__, values, GEL_TYPE_SYMBOL);
}


static
void set_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    if(n_values == 2)
    {
        symbol_set(return_value, n_values, values, context);
        return;
    }

    GList *tmp_list = NULL;
    GValue *value = NULL;


    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V*", &value))
    {
        GType type = GEL_VALUE_TYPE(value);
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = gel_value_get_boxed(value);
            array_set(array, return_value, n_values, values, context);
        }
        else
        if(type == G_TYPE_HASH_TABLE)
        {
            GHashTable *hash = gel_value_get_boxed(value);
            hash_set(hash, return_value, n_values, values, context);
        }
        else
        if(G_TYPE_IS_OBJECT(type))
        {
            GObject *object = gel_value_get_object(value);
            object_set(object, return_value, n_values, values, context);
        }
        else
            gel_error_expected(context,
                __FUNCTION__, "array, hash or object");
    }

    gel_list_free(tmp_list);
}


static
void arithmetic(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values,
                GelContext *context,
                GelValuesArithmetic values_function, const gchar *f)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GValue tmp1 = {0};
    GValue tmp2 = {0};
    GValue tmp3 = {0};
    GValue *v2 = &tmp3;

    gboolean running = FALSE;

    const GValue *v0 =
        gel_context_eval_into_value(context, values + 0, &tmp1);

    if(!gel_context_error(context))
    {
        const GValue *v1 =
            gel_context_eval_into_value(context, values + 1, &tmp2);

        if(!gel_context_error(context))
            running = values_function(v0, v1, &tmp3);

        if(!running)
            gel_error_incompatible(context, f, v0, v1);
    }

    if(GEL_IS_VALUE(&tmp1))
        g_value_unset(&tmp1);
    if(GEL_IS_VALUE(&tmp2))
        g_value_unset(&tmp2);

    for(guint i = 2; i < n_values && running; i++)
    {
        GValue *v1;

        if(i%2 == 0)
            v1 = &tmp3, v2 = &tmp2;
        else
            v2 = &tmp3, v1 = &tmp2;

        const GValue *v =
            gel_context_eval_into_value(context, values + i, &tmp1);

        if(!gel_context_error(context))
        {
            running = values_function(v1, v, v2);
            if(!running)
                gel_error_incompatible(context, f, v1, v);
        }

        if(GEL_IS_VALUE(v1))
            g_value_unset(v1);
        if(GEL_IS_VALUE(&tmp1))
            g_value_unset(&tmp1);

        if(gel_context_error(context))
        {
            running = FALSE;
            break;
        }
    }

    if(running)
        gel_value_copy(v2, return_value);
    if(GEL_IS_VALUE(v2))
        g_value_unset(v2);
}


static
void add_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values,
        context, gel_values_add, __FUNCTION__);
}


static
void sub_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values,
        context, gel_values_sub, __FUNCTION__);
}


static
void mul_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values,
        context, gel_values_mul, __FUNCTION__);
}


static
void div_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values,
        context, gel_values_div, __FUNCTION__);
}


static
void mod_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values,
        context, gel_values_mod, __FUNCTION__);
}


static
void logic(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context,
           GelValuesLogic values_function, const gchar *f)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    gint result = -1;
    guint last = n_values - 1;
    gboolean failed = FALSE;

    for(guint i = 0; i < last && result != 1 && !failed; i++)
    {
        GValue tmp1 = {0};
        GValue tmp2 = {0};

        const GValue *j = values + i;
        const GValue *v1 = gel_context_eval_into_value(context, j, &tmp1);

        if(!gel_context_error(context))
        {
            const GValue *v2 =
                gel_context_eval_into_value(context, j+1, &tmp2);

            if(!gel_context_error(context))
            {
                result = values_function(v1, v2);
                if(result == -1)
                {
                    gel_error_incompatible(context, f, v1, v2);
                    failed = TRUE;
                }
            }
            else
                failed = TRUE;
        }
        else
            failed = TRUE;

        if(GEL_IS_VALUE(&tmp1))
            g_value_unset(&tmp1);
        if(GEL_IS_VALUE(&tmp2))
            g_value_unset(&tmp2);
    }

    if(!failed)
    {
        g_value_init(return_value, G_TYPE_BOOLEAN);
        gel_value_set_boolean(return_value, result == 1 ? TRUE : FALSE);
    }
}


static
void and_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    gboolean result = TRUE;
    guint last = n_values - 1;
    gboolean failed = FALSE;

    for(guint i = 0; i <= last && result == TRUE && !failed; i++)
    {
        GValue tmp_value = {0};

        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);

        if(!gel_context_error(context))
        {
            result = gel_value_to_boolean(value);
            if(!result || i == last)
                gel_value_copy(value, return_value);
        }
        else
            failed = TRUE;

        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
}


static
void or_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    guint last =  n_values - 1;
    gboolean result = FALSE;
    gboolean failed = FALSE;

    for(guint i = 0; i <= last && result == FALSE && !failed; i++)
    {
        GValue tmp_value = {0};

        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);

        if(!gel_context_error(context))
        {
            result = gel_value_to_boolean(value);
            if(result || i == last)
                gel_value_copy(value, return_value);
        }
        else
            failed = TRUE;

        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
}


static
void gt_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values,
        context, gel_values_gt, __FUNCTION__);
}


static
void ge_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values,
        context, gel_values_ge, __FUNCTION__);
}


static
void eq_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values,
        context, gel_values_eq, __FUNCTION__);
}


static
void le_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values, context,
        gel_values_le, __FUNCTION__);
}


static
void lt_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values,
        context, gel_values_lt, __FUNCTION__);
}


static
void ne_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values,
        context, gel_values_ne, __FUNCTION__);
}


static
void append_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V*", &value))
    {
        GType type = GEL_VALUE_TYPE(value);
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = gel_value_get_boxed(value);
            array_append(array, return_value, n_values, values, context);
        }
        else
        if(type == G_TYPE_HASH_TABLE)
        {
            GHashTable *hash = gel_value_get_boxed(value);
            hash_append(hash, return_value, n_values, values, context);
        }
        else
            gel_error_expected(context, __FUNCTION__, "array or hash");
    }

    gel_list_free(tmp_list);
}


static
void get_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V*", &value))
    {
        GType type = GEL_VALUE_TYPE(value);
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = gel_value_get_boxed(value);
            array_get(array, return_value, n_values, values, context);
        }
        else
        if(type == G_TYPE_HASH_TABLE)
        {
            GHashTable *hash = gel_value_get_boxed(value);
            hash_get(hash, return_value, n_values, values, context);
        }
        else
        if(G_TYPE_IS_OBJECT(type))
        {
            GObject *object = gel_value_get_object(value);
            object_get(object, return_value, n_values, values, context);
        }
        else
            gel_error_expected(context,
                __FUNCTION__, "array, hash or object");
    }

    gel_list_free(tmp_list);
}


static
void remove_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V*", &value))
    {
        GType type = GEL_VALUE_TYPE(value);
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = gel_value_get_boxed(value);
            array_remove(array, return_value, n_values, values, context);
        }
        else
        if(type == G_TYPE_HASH_TABLE)
        {
            GHashTable *hash = gel_value_get_boxed(value);
            hash_remove(hash, return_value, n_values, values, context);
        }
        else
            gel_error_expected(context, __FUNCTION__, "array or hash");
    }

    gel_list_free(tmp_list);
}


static
void size_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V*", &value))
    {
        GType type = GEL_VALUE_TYPE(value);
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = gel_value_get_boxed(value);
            array_size(array, return_value, n_values, values, context);
        }
        else
        if(type == G_TYPE_HASH_TABLE)
        {
            GHashTable *hash = gel_value_get_boxed(value);
            hash_size(hash, return_value, n_values, values, context);
        }
        else
            gel_error_expected(context, __FUNCTION__, "array or hash");
    }

    gel_list_free(tmp_list);
}



static
void find_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GClosure *closure = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "CV", &closure, &value))
    {
        GType type = GEL_VALUE_TYPE(value);
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = gel_value_get_boxed(value);
            array_find(closure, array, return_value, n_values, values, context);
        }
        else
        if(type == G_TYPE_HASH_TABLE)
        {
            GHashTable *hash = gel_value_get_boxed(value);
            hash_find(closure, hash, return_value, n_values, values, context);
        }
        else
            gel_error_expected(context, __FUNCTION__, "array or hash");
    }

    gel_list_free(tmp_list);
}


static
void filter_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values != n_args)
    {
        gel_error_needs_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GClosure *closure = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "CV", &closure, &value))
    {
        GType type = GEL_VALUE_TYPE(value);
        if(type == GEL_TYPE_ARRAY)
        {
            GelArray *array = gel_value_get_boxed(value);
            array_filter(closure,
                array, return_value, n_values, values, context);
        }
        else
        if(type == G_TYPE_HASH_TABLE)
        {
            GHashTable *hash = gel_value_get_boxed(value);
            hash_filter(closure, hash, return_value, n_values, values, context);
        }
        else
            gel_error_expected(context, __FUNCTION__, "array or hash");
    }

    gel_list_free(tmp_list);
}



static
void compare_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValue *v1 = NULL;
    GValue *v2 = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "VV", &v1, &v2))
    {
        g_value_init(return_value, G_TYPE_INT64);
        gel_value_set_int64(return_value, gel_values_cmp(v1, v2));
    }

    gel_list_free(tmp_list);
}


static
void sort_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;
    GelArray *array = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "CA", &closure, &array))
    {
        gint compare(GValue *v1, GValue *v2)
        {
            GelArray *pair = gel_array_new(2);
            gel_array_append(pair, v1);
            gel_array_append(pair, v2);

            GValue tmp_value = {0};
            g_closure_invoke(closure, &tmp_value, 2,
            gel_array_get_values(pair),
            context);

            gboolean result = gel_value_to_boolean(&tmp_value) ? -1 : 1;

            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
            gel_array_free(pair);

            return result;
        }

        GelArray *result_array = gel_array_copy(array);
        gel_array_sort(result_array, (GCompareFunc)compare);

        g_value_init(return_value, GEL_TYPE_ARRAY);
        g_value_take_boxed(return_value, result_array);
    }

    gel_list_free(tmp_list);
}


static
void reverse_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GelArray *array = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "A", &array))
    {
        guint array_n_values = gel_array_get_n_values(array);
        const GValue *array_values = gel_array_get_values(array);
        GelArray *result_array = gel_array_new(array_n_values);
        
        for(guint i = array_n_values; i > 0; i--)
            gel_array_append(result_array, array_values + (i-1));

        g_value_init(return_value, GEL_TYPE_ARRAY);
        gel_value_take_boxed(return_value, result_array);
    }

}


static
void keys_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "H", &hash))
    {
        guint size = g_hash_table_size(hash);
        GelArray *array = gel_array_new(size);
        GList *keys = g_hash_table_get_keys(hash);

        for(GList *iter = keys; iter != NULL; iter = g_list_next(iter))
            gel_array_append(array, iter->data);

        g_value_init(return_value, GEL_TYPE_ARRAY);
        gel_value_take_boxed(return_value, array);
        g_list_free(keys);
    }

    gel_list_free(tmp_list);
}



static
void new_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *value =
        gel_context_eval_into_value(context, values + 0, &tmp_value);

    GType type = G_TYPE_INVALID;

    GType value_type = GEL_VALUE_TYPE(value);
    if(value_type == G_TYPE_STRING)
    {
        const gchar *type_name = gel_value_get_string(value);
        type = g_type_from_name(type_name);
        if(type == G_TYPE_INVALID)
            gel_error_type_name_invalid(context, __FUNCTION__, type_name);
    }
    else
    if(value_type == G_TYPE_GTYPE)
        type = gel_value_get_gtype(&tmp_value);
    else
        gel_error_expected(context, __FUNCTION__, "typename or type");

    if(type != G_TYPE_INVALID)
    {
        g_value_init(return_value, type);
        if(G_TYPE_IS_INSTANTIATABLE(type))
        {
            n_values--;
            values++;

            guint n_params = n_values/2;
            GParameter *params = g_new0(GParameter, n_params);
            for(guint i = 0; i < n_params; i++)
            {
                params[i].name = gel_value_to_string(values++);
                gel_context_eval_value(context, values++, &params[i].value);
            }

            GObject *new_object = g_object_newv(type, n_params, params);

            for(guint i = 0; i < n_params; i++)
            {
                g_free((gchar *)params[i].name);
                g_value_unset(&params[i].value);
            }
            g_free(params);

            if(G_IS_INITIALLY_UNOWNED(new_object))
                g_object_ref_sink(new_object);
            gel_value_take_object(return_value, new_object);
        }
    }

    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void connect_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GObject *object = NULL;
    gchar *signal = NULL;
    GClosure *callback = NULL;
    GList *tmp_list = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "OSC", &object, &signal, &callback))
    {
        if(G_IS_OBJECT(object))
        {
            g_value_init(return_value, G_TYPE_INT64);
            guint connect_id =
                g_signal_connect_closure(object,
                    signal, g_closure_ref(callback), FALSE);
            gel_value_set_int64(return_value, connect_id);
        }
        else
            gel_error_value_not_of_type(context,
                __FUNCTION__, values + 0, G_TYPE_OBJECT);
    }

    gel_list_free(tmp_list);
}


static
void if_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context,
            __FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *cond_value =
        gel_context_eval_into_value(context, values + 0, &tmp_value);

    if(gel_value_to_boolean(cond_value))
        gel_context_eval_value(context, values + 1, return_value);
    else
        if(n_values > 2)
            gel_context_eval_value(context, values + 2, return_value);

    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void cond_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;

    while(n_values > 0 && !gel_context_error(context))
        if(n_values == 1)
            do_(self, return_value, n_values--, values++, context);
        else
        {
            GValue *test_value = NULL;
            GValue *value = NULL;
            if(gel_context_eval_params(context, __FUNCTION__,
                    &n_values, &values, &tmp_list, "Vv*", &test_value, &value))
                if(gel_value_to_boolean(test_value))
                {
                    do_(self, return_value, 1, value, context);
                    n_values = 0;
                }
        }

    gel_list_free(tmp_list);
}


static
void case_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GValue *probe_value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V*", &probe_value))
        while(n_values > 0 && !gel_context_error(context))
            if(n_values == 1)
                do_(self, return_value, n_values--, values++, context);
            else
            {
                GelArray *tests = NULL;
                GValue *value = NULL;
                if(gel_context_eval_params(context, __FUNCTION__,
                    &n_values, &values, &tmp_list, "av*", &tests, &value))
                {
                    const GValue *test_values =
                        gel_array_get_values(tests);
                    guint test_n_values = gel_array_get_n_values(tests);

                    for(guint i = 0; i < test_n_values; i++)
                    {
                        if(gel_values_eq(test_values + i, probe_value))
                        {
                            do_(self, return_value, 1, value, context);
                            n_values = 0;
                            break;
                        }

                        if(gel_context_error(context))
                            break;
                    }
                }
            }

    gel_list_free(tmp_list);
}


static
void while_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    gboolean cond_is_true = FALSE;
    GelContext *loop_context = gel_context_new_with_outer(context);
    gboolean running = TRUE;

    while(running)
    {
        GValue tmp_value = {0};
        const GValue *cond_value =
            gel_context_eval_into_value(loop_context, values + 0, &tmp_value);

        if(!gel_context_error(loop_context))
        {
            cond_is_true = gel_value_to_boolean(cond_value);
            if(cond_is_true)
                do_(self, return_value, n_values-1, values+1, loop_context);
            else
                running = FALSE;
        }
        else
            running = FALSE;

        if(gel_context_error(loop_context))
            running = FALSE;

        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    gel_context_free(loop_context);
}


static
void for_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    const gchar *iter_name;
    GelArray *array;
    GList *tmp_list = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values,&tmp_list, "sA*", &iter_name, &array))
    {
        guint last = gel_array_get_n_values(array);
        const GValue *array_values = gel_array_get_values(array);

        GelContext *loop_context = gel_context_new_with_outer(context);
        GValue *iter_value = gel_value_new();
        gel_context_define_value(loop_context, iter_name, iter_value);

        gboolean running = TRUE;

        for(guint i = 0; i < last && running; i++)
        {
            gel_value_copy(array_values + i, iter_value);
            GValue tmp_value = {0};
            do_(self, &tmp_value, n_values, values, loop_context);

            if(gel_context_error(loop_context))
                running = FALSE;

            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
            g_value_unset(iter_value);
        }

        gel_context_free(loop_context);
    }

    gel_list_free(tmp_list);
}


static
void range_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    gint64 first = 0;
    gint64 last = 0;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "II", &first, &last))
    {
        GelArray *array = gel_array_new(ABS(last-first) + 1);
        GValue value = {0};
        g_value_init(&value, G_TYPE_INT64);

        if(last > first)
            for(gint64 i = first; i <= last; i++)
            {
                gel_value_set_int64(&value, i);
                gel_array_append(array, &value);
            }
        else
            for(gint64 i = first; i >= last; i--)
            {
                gel_value_set_int64(&value, i);
                gel_array_append(array, &value);
            }

        g_value_unset(&value);
        g_value_init(return_value, GEL_TYPE_ARRAY);
        gel_value_take_boxed(return_value, array);
    }

    gel_list_free(tmp_list);
}


static
void print_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    if(n_values > 0)
    {
        guint last = n_values - 1;
        for(guint i = 0; i <= last; i++)
        {
            GValue tmp_value = {0};
            const GValue *value =
                gel_context_eval_into_value(context, values + i, &tmp_value);

            if(!gel_context_error(context))
            {
                if(GEL_IS_VALUE(value))
                {
                    gchar *value_string = gel_value_to_string(value);
                    g_print("%s%s", value_string, i == last ? "" : " ");
                    g_free(value_string);
                }

                if(GEL_IS_VALUE(&tmp_value))
                    g_value_unset(&tmp_value);
            }
            else
                break;
        }
    }
    g_print("\n");
}


static
void str_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GString *buffer = g_string_new("");

    for(guint i = 0; i < n_values; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);

        if(!gel_context_error(context))
        {
            if(GEL_IS_VALUE(value))
            {
                gchar *value_string = gel_value_to_string(value);
                g_string_append_printf(buffer, "%s", value_string);
                g_free(value_string);
            }

            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
        }
        else
            break;
    }
 
    g_value_init(return_value, G_TYPE_STRING);
    gel_value_take_string(return_value, g_string_free(buffer, FALSE));
}


static
void type_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValue *value;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V", &value))
    {
        g_value_init(return_value, G_TYPE_GTYPE);
        gel_value_set_gtype(return_value, GEL_VALUE_TYPE(value));
    }

    g_list_free(tmp_list);
}


#ifdef HAVE_GOBJECT_INTROSPECTION
static
void require_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    const gchar *namespace_ = NULL;
    GList *tmp_list = NULL;
    GelTypelib *ns = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "s", &namespace_))
    {
        if(gel_context_get_variable(context, namespace_) == NULL)
        {
            ns = gel_typelib_new(namespace_, NULL);
            if(ns != NULL)
            {
                GValue *value = gel_value_new_from_boxed(GEL_TYPE_TYPELIB, ns);
                gel_context_define_value(context, namespace_, value);
            }    
        }
        else
            gel_error_symbol_exists(context, __FUNCTION__, namespace_);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, ns != NULL);

    gel_list_free(tmp_list);
}


static
void dot_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_error_needs_at_least_n_arguments(context, __FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *value =
        gel_context_eval_into_value(context, values + 0, &tmp_value);

    if(gel_context_error(context))
        return;

    values++;
    n_values--;

    const GelTypelib *typelib = NULL;
    const GelTypeInfo *type_info = NULL;
    void *instance = NULL;

    GType type = GEL_VALUE_TYPE(value);
    if(type == GEL_TYPE_TYPELIB)
        typelib = gel_value_get_boxed(value);
    else
    if(type == GEL_TYPE_TYPEINFO)
        type_info = gel_value_get_boxed(value);
    else
    if(type == G_TYPE_GTYPE)
    {
        type_info = gel_type_info_from_gtype(gel_value_get_gtype(value));
        if(type_info == NULL)
            gel_error_type_name_invalid(context,
                __FUNCTION__, g_type_name(type));
    }
    else
    if(G_TYPE_IS_OBJECT(type))
    {
        type_info = gel_type_info_from_gtype(type);
        if(type_info == NULL)
            gel_error_type_name_invalid(context,
                __FUNCTION__, g_type_name(type));
        else
            instance = gel_value_get_object(value);
    }
    else
    if(G_TYPE_IS_BOXED(type))
    {
        type_info = gel_type_info_from_gtype(type);
        if(type_info == NULL)
            gel_error_type_name_invalid(context,
                __FUNCTION__, g_type_name(type));
        else
            instance = gel_value_get_boxed(value);
    }
    else
        gel_error_expected(context, __FUNCTION__,
            "typelib, type, boxed or object");

    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);

    if(type_info == NULL && typelib == NULL)
        return;

    gboolean failed = FALSE;

    while(n_values > 0 && !failed)
    {
        const gchar *name = NULL;
        value = values + 0;
        type = GEL_VALUE_TYPE(value);

        if(type == GEL_TYPE_SYMBOL)
        {
            GelSymbol *symbol = gel_value_get_boxed(value);
            name = gel_symbol_get_name(symbol);
        }
        else
        if(type == G_TYPE_STRING)
            name = gel_value_get_string(value);

        if(name != NULL)
        {
            if(type_info != NULL)
                type_info = gel_type_info_lookup(type_info, name);
            else
            if(typelib != NULL)
                type_info = gel_typelib_lookup(typelib, name);

            if(type_info == NULL)
            {
                gel_error_unknown_symbol(context, __FUNCTION__, name);
                break;
            }
        }
        else
        {
            gel_error_expected(context, __FUNCTION__, "symbol or string");
            break;
        }

        values++;
        n_values--;    
    }

    if(!failed && type_info != NULL)
        gel_type_info_to_value(type_info, instance, return_value);
}
#endif


#define CLOSURE_NAME(N, S) {N, (GClosureMarshal)S##_}
#define CLOSURE(S) {#S, (GClosureMarshal)S##_}


static
GHashTable* gel_make_default_symbols(void)
{
    struct {const gchar *name; GClosureMarshal marshal;} *c, closures[] =
    {
        /* bindings */
        CLOSURE(def),
        CLOSURE(defn),
        CLOSURE(fn),
        CLOSURE(do),
        CLOSURE(let),
        CLOSURE(eval),
        CLOSURE(quote),

        /* closures */
        CLOSURE(apply),  /* array */
        CLOSURE(map),  /* array */

        /* structures */
        CLOSURE(array),
        CLOSURE(hash),

        /* symbols */
        CLOSURE(var),
        CLOSURE(name),

        /* accesors */
        CLOSURE(set), /* symbol variable array hash object */
        CLOSURE(get), /* array hash object */
        CLOSURE(append), /* array hash */
        CLOSURE(remove), /* array hash */
        CLOSURE(size), /* array hash */
        CLOSURE(find), /* array hash */
        CLOSURE(filter), /* array hash */
        CLOSURE(compare),
        CLOSURE(sort), /* array */
        CLOSURE(reverse), /* array */
        CLOSURE(keys), /* hash */

        /* objects */
        CLOSURE(new), /* object */
        CLOSURE(connect), /* object */

        /* conditional */
        CLOSURE(if),
        CLOSURE(cond),
        CLOSURE(case),

        /* loop */
        CLOSURE(while),
        CLOSURE(for),
        CLOSURE(range),

        /* output */
        CLOSURE(print),
        CLOSURE(str),
        CLOSURE(type),

        /* arithmetic */
        CLOSURE_NAME("+", add), /* number string array hash */
        CLOSURE_NAME("-", sub), /* number */
        CLOSURE_NAME("*", mul), /* number */
        CLOSURE_NAME("/", div), /* number */
        CLOSURE_NAME("%", mod), /* number */

        /* logic */
        CLOSURE(and),
        CLOSURE(or),
        CLOSURE_NAME(">", gt), /* number string */
        CLOSURE_NAME(">=", ge), /* number string */
        CLOSURE_NAME("=", eq), /* number string array */
        CLOSURE_NAME("<", lt), /* number string */
        CLOSURE_NAME("<=", le), /* number string */
        CLOSURE_NAME("!=", ne), /* number string */

#ifdef HAVE_GOBJECT_INTROSPECTION
        /* introspection */
        CLOSURE(require),
        CLOSURE_NAME(".", dot),
#endif

        {NULL,NULL}
    };


    GHashTable *symbols = g_hash_table_new_full(
            g_str_hash, g_str_equal,
            (GDestroyNotify)g_free, (GDestroyNotify)gel_variable_unref);

    GValue *value;
    GelVariable *variable;

    for(c = closures; c->name != NULL; c++)
    {
        GClosure *closure = gel_closure_new_native(c->name, c->marshal);
        value = gel_value_new_from_boxed(G_TYPE_CLOSURE, closure);
        variable = gel_variable_new(value, TRUE);
        g_hash_table_insert(symbols, g_strdup(c->name), variable);
    }

    value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    gel_value_set_boolean(value, TRUE);
    variable = gel_variable_new(value, TRUE);
    g_hash_table_insert(symbols, g_strdup("TRUE"), variable);

    value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    gel_value_set_boolean(value, FALSE);
    variable = gel_variable_new(value, TRUE);
    g_hash_table_insert(symbols, g_strdup("FALSE"), variable);

    value = gel_value_new_of_type(G_TYPE_POINTER);
    gel_value_set_pointer(value, NULL);
    variable = gel_variable_new(value, TRUE);
    g_hash_table_insert(symbols, g_strdup("NULL"), variable);
    return symbols;
}


GelVariable* gel_variable_lookup_predefined(const gchar *name)
{
    static GHashTable *symbols = NULL;
    static volatile gsize once = 0;

    if(g_once_init_enter(&once))
    {
        symbols = gel_make_default_symbols();
        g_once_init_leave(&once, 1);
    }

    return g_hash_table_lookup(symbols, name);
}


const GValue* gel_value_lookup_predefined(const gchar *name)
{
    const GValue *value = NULL;

    const GelVariable *variable = gel_variable_lookup_predefined(name);
    if(variable != NULL)
        value = gel_variable_get_value(variable);

    return value;
}

