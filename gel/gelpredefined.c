#include <gelcontext.h>
#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelclosure.h>


static
void set_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    const gchar *symbol = NULL;
    GValue *value = NULL;
    GList *list = NULL;

     if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "sV", &n_values, &values, &symbol, &value))
        return;

    GValue *symbol_value = gel_context_lookup_symbol(context, symbol);
    if(symbol_value != NULL)
        gel_value_copy(value, symbol_value);
    else
        gel_warning_unknown_symbol(__FUNCTION__, symbol);

    gel_value_list_free(list);
}


static
void object_new(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    const gchar *type_name = NULL;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "S", &n_values, &values, &type_name))
        return;

    GType type = g_type_from_name(type_name);
    if(type != G_TYPE_INVALID)
        if(G_TYPE_IS_INSTANTIATABLE(type))
        {
            GObject *new_object = (GObject*)g_object_new(type, NULL);
            if(G_IS_INITIALLY_UNOWNED(new_object))
                g_object_ref_sink(new_object);

            g_value_init(return_value, type);
            g_value_take_object(return_value, new_object);
        }
        else
            gel_warning_type_not_instantiatable(__FUNCTION__, type);
    else
        gel_warning_type_name_invalid(__FUNCTION__, type_name);

    gel_value_list_free(list);
}


static
void object_get(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GObject *object = NULL;
    const gchar *prop_name = NULL;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "OS", &n_values, &values, &object, &prop_name))
        return;

    if(G_IS_OBJECT(object))
    {
        GParamSpec *prop_spec =
            g_object_class_find_property(G_OBJECT_GET_CLASS(object), prop_name);
        if(prop_spec != NULL)
        {
            g_value_init(return_value, prop_spec->value_type);
            g_object_get_property(object, prop_name, return_value);
        }
        else
            gel_warning_no_such_property(__FUNCTION__, prop_name);
    }

    gel_value_list_free(list);
}


static
void object_set(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GObject *object = NULL;
    gchar *prop_name = NULL;
    GValue *value = NULL;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "OSV", &n_values, &values, &object, &prop_name, &value))
        return;

    if(G_IS_OBJECT(object))
    {
        GParamSpec *prop_spec =
            g_object_class_find_property(G_OBJECT_GET_CLASS(object), prop_name);
        if(prop_spec != NULL)
        {
            GValue result_value = {0};
            g_value_init(&result_value, prop_spec->value_type);

            if(g_value_transform(value, &result_value))
            {
                g_object_set_property(object, prop_name, &result_value);
                g_value_unset(&result_value);
            }
            else
                gel_warning_invalid_value_for_property(__FUNCTION__,
                    value, prop_spec);
        }
        else
            gel_warning_no_such_property(__FUNCTION__, prop_name);
    }

    gel_value_list_free(list);
}


static
GClosure* new_closure(GelContext *context, const gchar *name,
                      guint n_vars, const GValue *var_values,
                      guint n_values, const GValue *values)
{
    gchar **vars = (gchar**)g_new0(gchar*, n_vars + 1);
    gchar *invalid_arg_name = NULL;

    guint i;
    for(i = 0; i < n_vars; i++)
    {
        const GValue *value = var_values + i;
        if(GEL_VALUE_HOLDS(value, GEL_TYPE_SYMBOL))
        {
            GelSymbol *symbol = (GelSymbol*)gel_value_get_boxed(value);
            vars[i] = g_strdup(gel_symbol_get_name(symbol));
        }
        else
        {
            invalid_arg_name = gel_value_to_string(value);
            break;
        }
    }

    GClosure *self;
    if(invalid_arg_name == NULL)
    {
        GValueArray *code = g_value_array_new(n_values);
        for(i = 0; i < n_values; i++)
            g_value_array_append(code, values + i);
        self = gel_closure_new(name, vars, code, context);
        g_closure_ref(self);
        g_closure_sink(self);
    }
    else
    {
        gel_warning_invalid_argument_name(__FUNCTION__, invalid_arg_name);
        g_free(invalid_arg_name);
        for(i = 0; i < n_vars; i++)
            if(vars[i] != NULL)
                g_free(vars[i]);
        g_free(vars);
        self = NULL;
    }

    return self;
}


static
void define_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GList *list = NULL;
    gchar *name = NULL;
    GValue *value = NULL;
    gboolean defined = TRUE;

    GType type = GEL_VALUE_TYPE(values + 0);

    if(type == GEL_TYPE_SYMBOL)
    {
        GValue *r_value = NULL;
        if(gel_context_eval_params(context, __FUNCTION__, &list,
                "sV", &n_values, &values, &name, &r_value))
            if(gel_context_lookup_symbol(context, name) == NULL)
            {
                defined = FALSE;
                value = gel_value_dup(r_value);
            }
    }
    else
    if(type == G_TYPE_VALUE_ARRAY)
    {
        GValueArray *array = NULL;
        if(gel_context_eval_params(context, __FUNCTION__, &list,
                "a*", &n_values, &values, &array))
        {
            guint array_n_values = array->n_values;
            const GValue *array_values = array->values;
            if(!gel_context_eval_params(context, __FUNCTION__, &list,
                    "s*", &array_n_values, &array_values, &name))
                type = G_TYPE_INVALID;
            else
                if(gel_context_lookup_symbol(context, name) == NULL)
                {
                    defined = FALSE;
                    GClosure *closure = new_closure(context, name,
                            array_n_values, array_values, n_values, values);
                    if(closure != NULL)
                    {
                        value = gel_value_new_of_type(G_TYPE_CLOSURE);
                        g_value_take_boxed(value, closure);
                    }
                }
        }
        else
            type = G_TYPE_INVALID;
    }

    if(type == G_TYPE_INVALID)
        g_warning("%s: Expected a symbol or array of symbols", __FUNCTION__);
    else
        if(defined)
            g_warning("%s: Symbol '%s' already exists", __FUNCTION__, name);
        else
            if(value != NULL)
                gel_context_insert_symbol(context, name, value);

    gel_value_list_free(list);
}


static
void lambda_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    GValueArray *array;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "a*", &n_values, &values, &array))
        return;

    GClosure *closure = new_closure(context, "lambda",
            array->n_values, array->values, n_values, values);

    if(closure != NULL)
    {
        g_value_init(return_value, G_TYPE_CLOSURE);
        g_value_take_boxed(return_value, closure);
    }

    gel_value_list_free(list);
}


static
void object_connect(GClosure *self, GValue *return_value,
                    guint n_values, const GValue *values, GelContext *context)
{
    GObject *object = NULL;
    gchar *signal = NULL;
    GClosure *callback = NULL;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "OSC", &n_values, &values, &object, &signal, &callback))
        return;

    if(G_IS_OBJECT(object))
    {
        g_value_init(return_value, G_TYPE_UINT);
        gel_value_set_uint(return_value,
            g_signal_connect_closure(object,
                signal, g_closure_ref(callback), FALSE));
    }
    else
        gel_warning_value_not_of_type(__FUNCTION__, values + 0, G_TYPE_OBJECT);

    gel_value_list_free(list);
}


static
void print_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    guint last = n_values - 1;
    guint i;
    for(i = 0; i <= last; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_value(context, values + i, &tmp_value);
        if(GEL_IS_VALUE(value))
        {
            gchar *value_string = gel_value_to_string(value);
            g_print("%s", value_string);
            g_free(value_string);
        }
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
    g_print("\n");
}


static
void arithmetic(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values,
                GelContext *context, GelValuesArithmetic values_function)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp1 = {0};
    GValue tmp2 = {0};
    GValue tmp3 = {0};
    GValue *v2 = &tmp3;

    gboolean running = values_function(
            gel_context_eval_value(context, values + 0, &tmp1),
            gel_context_eval_value(context, values + 1, &tmp2),
            &tmp3);

    if(GEL_IS_VALUE(&tmp1))
        g_value_unset(&tmp1);
    if(GEL_IS_VALUE(&tmp2))
        g_value_unset(&tmp2);

    guint i;
    for(i = 2; i < n_values && running; i++)
    {
        GValue *v1;

        if(i%2 == 0)
            v1 = &tmp3, v2 = &tmp2;
        else
            v2 = &tmp3, v1 = &tmp2;

        running = values_function(
            v1, gel_context_eval_value(context, values + i, &tmp1), v2);

        if(GEL_IS_VALUE(v1))
            g_value_unset(v1);
        if(GEL_IS_VALUE(&tmp1))
            g_value_unset(&tmp1);
    }

    if(running)
        gel_value_copy(v2, return_value);
    g_value_unset(v2);
}


static
void logic(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, GelValuesLogic values_function)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    gboolean result = TRUE;
    guint last = n_values - 1;

    guint i;
    for(i = 0; i < last && result; i++)
    {
        GValue tmp1 = {0};
        GValue tmp2 = {0};
        const GValue *i_value = values + i;
        result = values_function(
            gel_context_eval_value(context, i_value, &tmp1),
            gel_context_eval_value(context, i_value+1, &tmp2));

        if(GEL_IS_VALUE(&tmp1))
            g_value_unset(&tmp1);
        if(GEL_IS_VALUE(&tmp2))
            g_value_unset(&tmp2);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, result);
}


static
void not_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *value =
        gel_context_eval_value(context, values + 0, &tmp_value);
    gboolean value_bool = gel_value_to_boolean(value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, !value_bool);
}


static
void and_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    gboolean result = TRUE;
    guint last = n_values - 1;
    guint i;
    for(i = 0; i <= last && result == TRUE; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_value(context, values + i, &tmp_value);
        result = gel_value_to_boolean(value);
        if(!result || i == last)
            gel_value_copy(value, return_value);
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
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    guint last =  n_values - 1;
    gboolean result = FALSE;
    guint i;
    for(i = 0; i <= last && result == FALSE; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_value(context, values + i, &tmp_value);
        result = gel_value_to_boolean(value);
        if(result || i == last)
            gel_value_copy(value, return_value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
}


static
void range_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    glong first = 0;
    glong last = 0;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "II", &n_values, &values, &first, &last))
        return;

    GValueArray *array = g_value_array_new(ABS(last-first) + 1);
    GValue value = {0};
    g_value_init(&value, G_TYPE_LONG);

    glong i;
    if(last > first)
        for(i = first; i <= last; i++)
        {
            gel_value_set_long(&value, i);
            g_value_array_append(array, &value);
        }
    else
        for(i = first; i >= last; i--)
        {
            gel_value_set_long(&value, i);
            g_value_array_append(array, &value);
        }

    g_value_unset(&value);
    g_value_init(return_value, G_TYPE_VALUE_ARRAY);
    g_value_take_boxed(return_value, array);
}


static
void any_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    gboolean result = FALSE;
    const GValue *array_values = array->values;
    guint i;
    for(i = 0; i < array->n_values && !result; i++)
    {
        GValue value = {0};
        g_closure_invoke(closure, &value, 1, array_values + i, context);
        result = gel_value_to_boolean(&value);
        g_value_unset(&value);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, result);
    gel_value_list_free(list);
}


static
void all_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    gboolean result = TRUE;
    guint last = array->n_values;
    const GValue *array_values = array->values;
    guint i;
    for(i = 0; i < last && result; i++)
    {
        GValue value = {0};
        g_closure_invoke(closure, &value, 1, array_values + i, context);
        result = gel_value_to_boolean(&value);
        g_value_unset(&value);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, result);
    gel_value_list_free(list);
}



static
void append_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "A*", &n_values, &values, &array))
        return;

    guint i;
    for(i = 0; i < n_values; i++)
    {
        GValue tmp = {0};
        const GValue *value = gel_context_eval_value(context, values + i, &tmp);
        g_value_array_append(array, value);
        if(GEL_IS_VALUE(&tmp))
            g_value_unset(&tmp);
    }

    gel_value_list_free(list);
}


static
void nth_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GValueArray *array = NULL;
    glong index = 0;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "AI", &n_values, &values, &array, &index))
        return;

    guint array_n_values = array->n_values;
    if(index < 0)
        index += array_n_values;

    if(index < 0 || index >= array_n_values)
    {
        gel_warning_index_out_of_bounds(__FUNCTION__);
        return;
    }

    gel_value_copy(array->values + index, return_value);
    gel_value_list_free(list);
}


static
void index_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GValueArray *array = NULL;
    GValue *value = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "AV", &n_values, &values, &array, &value))
        return;

    guint array_n_values = array->n_values;
    const GValue *array_values = array->values;

    glong i;
    for(i = 0; i < array_n_values; i++)
        if(gel_values_eq(value, array_values + i))
            break;

    g_value_init(return_value, G_TYPE_LONG);
    g_value_set_long(return_value, i == array_n_values ? -1 : i);
    gel_value_list_free(list);
}


static
void remove_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GValueArray *array = NULL;
    glong index = 0;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "AI", &n_values, &values, &array, &index))
        return;

    guint array_n_values = array->n_values;
    if(index < 0)
        index += array_n_values;

    if(index < 0 || index >= array_n_values)
    {
        gel_warning_index_out_of_bounds(__FUNCTION__);
        return;
    }

    g_value_array_remove(array, index);
    gel_value_list_free(list);
}


static
void len_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "A", &n_values, &values, &array))
        return;

    g_value_init(return_value, G_TYPE_UINT);
    gel_value_set_uint(return_value, array->n_values);
    gel_value_list_free(list);
}


static
void sort_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "A", &n_values, &values, &array))
        return;

    g_value_array_sort(array, (GCompareFunc)gel_values_cmp);
    gel_value_list_free(list);
}


static
void map_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    const GValue *array_values = array->values;
    const guint array_n_values = array->n_values;

    GValueArray *result_array = g_value_array_new(array_n_values);
    result_array->n_values = array_n_values;
    GValue *result_array_values = result_array->values;

    guint i;
    for(i = 0; i < array_n_values; i++)
        g_closure_invoke(closure,
            result_array_values + i, 1, array_values + i, context);

    g_value_init(return_value, G_TYPE_VALUE_ARRAY);
    g_value_take_boxed(return_value, result_array);
    gel_value_list_free(list);
}


static
void apply_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    g_closure_invoke(closure, return_value,
        array->n_values, array->values, context);
    gel_value_list_free(list);
}


static
void zip_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;

    guint n_arrays = n_values;
    GValueArray **arrays = g_new0(GValueArray*, n_values);

    guint i;
    for(i = 0; i < n_arrays; i++)
        if(!gel_context_eval_params(context, __FUNCTION__, &list,
                "A*", &n_values, &values, arrays + i))
        break;

    if(i == n_arrays)
    {
        guint min_n_values = arrays[0]->n_values;
        for(i = 1; i < n_arrays; i++)
        {
            guint i_n_values = arrays[i]->n_values;
            if(i_n_values < min_n_values)
                min_n_values = i_n_values;
        }

        GValueArray *result_array = g_value_array_new(min_n_values);
        GValue *result_array_values = result_array->values;
        result_array->n_values = min_n_values;

        for(i = 0; i < min_n_values; i++)
        {
            GValueArray *array = g_value_array_new(n_arrays);
            guint j;
            for(j = 0; j < n_arrays; j++)
                g_value_array_append(array, arrays[j]->values + i);
            g_value_init(result_array_values + i, G_TYPE_VALUE_ARRAY);
            g_value_take_boxed(result_array_values + i, array);
        }
        g_value_init(return_value, G_TYPE_VALUE_ARRAY);
        g_value_take_boxed(return_value, result_array);
    }
    else
        g_warning("%s: Argument %u is not an array", __FUNCTION__, i);

    g_free(arrays);
    gel_value_list_free(list);
}


static
void filter_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    GValueArray *result_array = g_value_array_new(array->n_values);

    guint i;
    for(i = 0; i < array->n_values; i++)
    {
        GValue tmp_value = {0};
        g_closure_invoke(closure, &tmp_value, 1, array->values + i, context);
        if(gel_value_to_boolean(&tmp_value))
            g_value_array_append(result_array, array->values + i);
        g_value_unset(&tmp_value);
    }

    g_value_init(return_value, G_TYPE_VALUE_ARRAY);
    g_value_take_boxed(return_value, result_array);
    gel_value_list_free(list);
}


static
void branch(GelContext *outer, const GValue *case_value,
            GValue *return_value, guint n_values, const GValue *values)
{
    gboolean match = FALSE;
    gboolean testing = TRUE;

    GelContext *context = gel_context_new_with_outer(outer);
    while(testing)
    {
        if(n_values > 1)
        {
            GValue tmp_value = {0};
            const GValue *if_value =
                gel_context_eval_value(context, values + 0, &tmp_value);

            if(case_value != NULL)
                match = gel_values_eq(if_value, case_value);
            else
                match = gel_value_to_boolean(if_value);

            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);

            if(match)
                n_values--, values++;
            else
                n_values -= 2, values += 2;
        }
        else if(n_values == 1)
            match = TRUE;
        else
            testing = FALSE;
        
        if(match)
        {
            GValue tmp_value = {0};
            const GValue *then_value =
                gel_context_eval_value(context, values + 0, &tmp_value);
            gel_value_copy(then_value, return_value);
            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
            testing = FALSE;
        }
    }
    gel_context_free(context);
}


static
void case_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *case_value =
        gel_context_eval_value(context, values + 0, &tmp_value);
    n_values--, values++;
    branch(context, case_value, return_value, n_values, values);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void begin_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    guint last = n_values-1;
    guint i;
    for(i = 0; i <= last && gel_context_get_running(context); i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_value(context, values + i, &tmp_value);
        if(i == last && GEL_IS_VALUE(value))
            gel_value_copy(value, return_value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
}


static
void cond_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *list = NULL;
    gboolean running = TRUE;
    while(n_values > 0 && running)
    {
        GValueArray *array = NULL;
        if(gel_context_eval_params(context, __FUNCTION__, &list,
                "a*", &n_values, &values, &array))
        {
            const GValue *array_values = array->values;
            guint array_n_values = array->n_values;
            GValue *cond_value = NULL;
            if(!gel_context_eval_params(context, __FUNCTION__, &list,
                    "V*", &array_n_values, &array_values, &cond_value))
                running = FALSE;
            else
                if(gel_value_to_boolean(cond_value))
                {
                    begin_(self, return_value,
                        array_n_values, array_values, context);
                    running = FALSE;
                }
        }
        else
            running = FALSE;
    }
    gel_value_list_free(list);
}


static
void array_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GValueArray *array = g_value_array_new(n_values);
    guint i;
    for(i = 0; i < n_values; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_value(context, values + i, &tmp_value);
        g_value_array_append(array, value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    g_value_init(return_value, G_TYPE_VALUE_ARRAY);
    g_value_take_boxed(return_value, array);
}


static
void for_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    const gchar *name;
    GValueArray *array;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "sA*", &n_values, &values, &name, &array))
        return;

    guint last = array->n_values;
    const GValue *array_values = array->values;

    GelContext *loop_context = gel_context_new_with_outer(context);
    GValue *value = gel_value_new();
    gel_context_insert_symbol(loop_context, name, value);

    guint i;
    for(i = 0; i < last && gel_context_get_running(loop_context); i++)
    {
        gel_value_copy(array_values + i, value);
        begin_(self, return_value, n_values, values, loop_context);
        g_value_unset(value);
    }
    gel_context_free(loop_context);

    gel_value_list_free(list);
}


static
void while_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    gboolean run = FALSE;
    GelContext *loop_context = gel_context_new_with_outer(context);
    while(gel_context_get_running(loop_context))
    {
        GValue tmp_value = {0};
        const GValue *cond_value =
            gel_context_eval_value(context, values + 0, &tmp_value);
        if(gel_value_to_boolean(cond_value))
        {
            run = TRUE;
            begin_(self, return_value, n_values-1, values+1, context);
        }
        else
        {
            gel_context_set_running(loop_context, FALSE);
            if(run == FALSE)
                gel_value_copy(cond_value, return_value);
        }
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
    gel_context_free(loop_context);
}


static
void break_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GelContext *i;
    for(i = context; i != NULL; i = gel_context_get_outer(i))
        if(gel_context_get_running(i))
        {
            gel_context_set_running(i, FALSE);
            break;
        }
}


static
void if_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 3;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *cond_value =
        gel_context_eval_value(context, values + 0, &tmp_value);

    if(gel_value_to_boolean(cond_value))
        gel_context_eval(context, values + 1, return_value);
    else
        gel_context_eval(context, values + 2, return_value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void when_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *cond_value =
        gel_context_eval_value(context, values + 0, &tmp_value);

    if(gel_value_to_boolean(cond_value))
        begin_(self, return_value, n_values-1, values+1, context);
    else
        gel_value_copy(cond_value, return_value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void unless_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *cond_value =
        gel_context_eval_value(context, values + 0, &tmp_value);

    if(!gel_value_to_boolean(cond_value))
        begin_(self, return_value, n_values-1, values+1, context);
    else
        gel_value_copy(cond_value, return_value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}

static
void str_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *value =
        gel_context_eval_value(context, values + 0, &tmp_value);

    gchar *value_string = gel_value_to_string(value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);

    g_value_init(return_value, G_TYPE_STRING);
    g_value_take_string(return_value, value_string);
}


static
void add_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values, context, gel_values_add);
}


static
void sub_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values, context, gel_values_sub);
}


static
void mul_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values, context, gel_values_mul);
}


static
void div_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values, context, gel_values_div);
}


static
void mod_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    arithmetic(self, return_value, n_values, values, context, gel_values_mod);
}


static
void gt_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values, context, gel_values_gt);
}


static
void ge_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values, context, gel_values_ge);
}


static
void eq_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values, context, gel_values_eq);
}


static
void le_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values, context, gel_values_le);
}


static
void lt_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values, context, gel_values_lt);
}


static
void ne_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    logic(self, return_value, n_values, values, context, gel_values_ne);
}


#define CLOSURE_NAME(N, S) {N, (GClosureMarshal)S}
#define CLOSURE(S) {#S, (GClosureMarshal)S##_}


static
GHashTable* gel_make_default_symbols(void)
{
    struct {gchar *name; GClosureMarshal marshal;} *c, closures[] =
    {
        CLOSURE(set),/* string */
        CLOSURE(define),/* string */
        CLOSURE_NAME("object-new", object_new),
        CLOSURE_NAME("object-get", object_get),
        CLOSURE_NAME("object-set", object_set),
        CLOSURE(lambda),/* array */
        CLOSURE_NAME("object-connect", object_connect),
        CLOSURE(print),
        CLOSURE(case),
        CLOSURE(cond),
        CLOSURE(begin),
        CLOSURE(array),
        CLOSURE(for),/* string */
        CLOSURE(while),
        CLOSURE(break),
        CLOSURE(if),
        CLOSURE(when),
        CLOSURE(unless),
        CLOSURE(str),
        CLOSURE(add),
        CLOSURE(sub),
        CLOSURE(mul),
        CLOSURE(div),
        CLOSURE(mod),
        CLOSURE(and),
        CLOSURE(not),
        CLOSURE(or),
        CLOSURE(gt),
        CLOSURE(ge),
        CLOSURE(eq),
        CLOSURE(lt),
        CLOSURE(le),
        CLOSURE(ne),
        CLOSURE(range),
        CLOSURE(any),
        CLOSURE(all),
        CLOSURE(append),
        CLOSURE(nth),
        CLOSURE(index),
        CLOSURE(remove),
        CLOSURE(len),
        CLOSURE(apply),
        CLOSURE(filter),
        CLOSURE(map),
        CLOSURE(zip),
        CLOSURE(sort),
        {NULL,NULL}
    };

    GValue *value;
    GHashTable *symbols = g_hash_table_new(g_str_hash, g_str_equal);

    for(c = closures; c->name != NULL; c++)
    {
        value = gel_value_new_of_type(G_TYPE_CLOSURE);
        GClosure *closure = gel_closure_new_native(c->name, c->marshal);
        g_value_set_boxed(value, closure);
        g_hash_table_insert(symbols, c->name, value);
    }

    value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    gel_value_set_boolean(value, TRUE);
    g_hash_table_insert(symbols, "TRUE", value);

    value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    gel_value_set_boolean(value, FALSE);
    g_hash_table_insert(symbols, "FALSE", value);

    value = gel_value_new_of_type(G_TYPE_POINTER);
    gel_value_set_pointer(value, NULL);
    g_hash_table_insert(symbols, "NULL", value);

    return symbols;
}

/**
 * gel_value_lookup_predefined:
 * @name: name of the predefined value to lookup
 *
 * Lookups the table of predefined values and retrieves
 * the value that correspond to @name.
 *
 * Returns: The #GValue that correspond to @name
 */
GValue *gel_value_lookup_predefined(const gchar *name)
{
    static volatile gsize symbols_once = 0;
    static GHashTable *symbols = NULL;
    if(g_once_init_enter(&symbols_once))
    {
        symbols = gel_make_default_symbols();
        g_once_init_leave(&symbols_once, 1);
    }
    return g_hash_table_lookup(symbols, name);
}

