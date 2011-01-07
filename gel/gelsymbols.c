#include <gelcontext.h>
#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>


static
void let_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const gchar *symbol = NULL;
    GValue *value = NULL;
    GList *list = NULL;

     if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "sV", &n_values, &values, &symbol, &value))
        return;

    register GValue *symbol_value = gel_context_find_symbol(context, symbol);
    if(symbol_value != NULL)
        gel_value_copy(value, symbol_value);
    else
        gel_warning_unknown_symbol(__FUNCTION__, symbol);
    gel_value_list_free(list);
}


static
void var_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const gchar *symbol = NULL;
    GValue *value = NULL;
    GList *list = NULL;

     if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "sV", &n_values, &values, &symbol, &value))
        return;

    gel_context_add_symbol(context, symbol, gel_value_dup(value));
    gel_value_list_free(list);
}


static
void new_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const gchar *type_name = NULL;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "s", &n_values, &values, &type_name))
        return;

    register GType type = g_type_from_name(type_name);
    if(type != G_TYPE_INVALID)
    {
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
    }
    else
        gel_warning_type_name_invalid(__FUNCTION__, type_name);

    gel_value_list_free(list);
}


static
void quote_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
        return;
    }

    g_value_init(return_value, G_TYPE_STRING);
    g_value_take_string(return_value, gel_value_to_string(values + 0));
}


static
void get_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
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
void set_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
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
        register GParamSpec *prop_spec =
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
GClosure* new_closure(GelContext *context,
                      GValueArray *vars_array,
                      guint n_values, const GValue *values)
{
    const guint n_vars = vars_array->n_values;
    gchar **const vars = (gchar**)g_new0(gchar*, n_vars + 1);
    gchar *invalid_arg_name = NULL;
    const GValue *const vars_array_values = vars_array->values;

    register guint i;
    for(i = 0; i < n_vars; i++)
    {
        const GValue *const value = vars_array_values + i;
        if(GEL_VALUE_HOLDS(value, G_TYPE_STRING))
            vars[i] = g_value_dup_string(value);
        else
        {
            invalid_arg_name = gel_value_to_string(value);
            break;
        }
    }

    register GClosure *self;
    if(invalid_arg_name == NULL)
    {
        GValueArray *code = g_value_array_new(n_values);
        for(i = 0; i < n_values; i++)
            g_value_array_append(code, values+i);
        self = gel_context_closure_new(context, vars, code);
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
void def_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const gchar *symbol;
    GValueArray *array;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "sa*", &n_values, &values, &symbol, &array))
        return;

    GClosure *closure = new_closure(context, array, n_values, values);
    if(closure != NULL)
    {
        GValue *value = gel_value_new_from_closure(closure);
        gel_context_add_symbol(context, symbol, value);
    }

    if(list != NULL)
        gel_value_list_free(list);
}


static
void lambda_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values,
             GelContext *context, gpointer marshal_data)
{
    GValueArray *array;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "a*", &n_values, &values, &array))
        return;

    register GClosure *closure = new_closure(context, array, n_values, values);

    if(closure != NULL)
    {
        g_value_init(return_value, G_TYPE_CLOSURE);
        g_value_take_boxed(return_value, closure);
    }

    if(list != NULL)
        gel_value_list_free(list);
}


static
void connect_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values,
              GelContext *context, gpointer marshal_data)
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

    if(list != NULL)
        gel_value_list_free(list);
}


static
void print_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    const guint last = n_values - 1;
    register guint i;
    for(i = 0; i <= last; i++)
    {
        GValue tmp_value = {0};
        register const GValue *value =
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
    const guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp1 = {0};
    GValue tmp2 = {0};
    GValue tmp3 = {0};
    register GValue *v2 = &tmp3;

    gboolean running = values_function(
            gel_context_eval_value(context, values + 0, &tmp1),
            gel_context_eval_value(context, values + 1, &tmp2),
            &tmp3);

    if(GEL_IS_VALUE(&tmp1))
        g_value_unset(&tmp1);
    if(GEL_IS_VALUE(&tmp2))
        g_value_unset(&tmp2);

    const guint last = n_values - 2;
    register guint i;
    for(i = 0; i < last && running; i++)
    {
        register GValue *v1;

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
    const guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    register gboolean result = TRUE;
    const guint last = n_values - 1;
    register guint i;
    for(i = 0; i < last && result; i++)
    {
        GValue tmp1 = {0};
        GValue tmp2 = {0};
        const GValue *const i_value = values + i;
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
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    register const GValue *value =
        gel_context_eval_value(context, values + 0, &tmp_value);
    register gboolean value_bool = gel_value_to_boolean(value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, !value_bool);
}


static
void and_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    register gboolean result = TRUE;
    const guint last = n_values - 1;
    register guint i;
    for(i = 0; i <= last && result == TRUE; i++)
    {
        GValue tmp_value = {0};
        register const GValue *value =
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
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    const guint last =  n_values - 1;
    register gboolean result = FALSE;
    register guint i;
    for(i = 0; i <= last && result == FALSE; i++)
    {
        GValue tmp_value = {0};
        register const GValue *value =
            gel_context_eval_value(context, values + i, &tmp_value);
        result = gel_value_to_boolean(value);
        if(result || i == last)
            gel_value_copy(value, return_value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
}


static
void any_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    register gboolean result = FALSE;
    const GValue *const array_values = array->values;
    register guint i;
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
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    register gboolean result = TRUE;
    const guint last = array->n_values;
    const GValue *const array_values = array->values;
    register guint i;
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
             guint n_values, const GValue *values,
             GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "A*", &n_values, &values, &array))
        return;

    register guint i;
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
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GValueArray *array = NULL;
    glong index = 0;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "AI", &n_values, &values, &array, &index))
        return;

    const guint array_n_values = array->n_values;
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
void remove_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values,
             GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GValueArray *array = NULL;
    glong index = 0;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "AI", &n_values, &values, &array, &index))
        return;

    const guint array_n_values = array->n_values;
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
void head_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "A", &n_values, &values, &array))
        return;

    if(array->n_values > 0)
        gel_value_copy(array->values + 0, return_value);
    gel_value_list_free(list);
}


static
void tail_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "A", &n_values, &values, &array))
        return;

    if(array->n_values > 0)
    {
        const guint tail_len = array->n_values - 1;
        register GValueArray *tail = g_value_array_new(tail_len);

        const GValue *const array_values = array->values;
        register guint i;
        for(i = 1; i <= tail_len; i++)
            g_value_array_append(tail, array_values + i);

        g_value_init(return_value, G_TYPE_VALUE_ARRAY);
        g_value_take_boxed(return_value, tail);
    }

    gel_value_list_free(list);
}


static
void len_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
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
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
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
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    register GValueArray *result_array = g_value_array_new(array->n_values);

    register guint i;
    for(i = 0; i < array->n_values; i++)
    {
        GValue tmp_value = {0};
        g_closure_invoke(closure, &tmp_value, 1, array->values + i, context);
        g_value_array_append(result_array, &tmp_value);
        g_value_unset(&tmp_value);
    }

    g_value_init(return_value, G_TYPE_VALUE_ARRAY);
    g_value_take_boxed(return_value, result_array);

    gel_value_list_free(list);
}


static
void apply_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
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
void filter_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values,
             GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "CA", &n_values, &values, &closure, &array))
        return;

    GValueArray *result_array = g_value_array_new(array->n_values);

    register guint i;
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


void branch(guint n_values, const GValue *values, GelContext *outer,
            GValue *return_value, const GValue *case_value)
{
    register gboolean match = FALSE;
    register gboolean testing = TRUE;

    register GelContext *context = gel_context_new_with_outer(outer);
    while(testing)
    {
        if(n_values > 1)
        {
            GValue tmp_value = {0};
            register const GValue *if_value =
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
            register const GValue *then_value =
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
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    register const GValue *case_value =
        gel_context_eval_value(context, values + 0, &tmp_value);
    n_values--, values++;
    branch(n_values, values, context, return_value, case_value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void cond_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    branch(n_values, values, context, return_value, NULL);
}


static
void do_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *outer, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    const guint last = n_values-1;
    register guint i;
    register GelContext *context = gel_context_new_with_outer(outer);
    for(i = 0; i <= last; i++)
    {
        GValue tmp_value = {0};
        register const GValue *value =
            gel_context_eval_value(context, values + i, &tmp_value);
        if(i == last && GEL_IS_VALUE(value))
            gel_value_copy(value, return_value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
    gel_context_free(context);
}


static
void array_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    GValueArray *array = g_value_array_new(n_values);
    register guint i;
    for(i = 0; i < n_values; i++)
    {
        GValue tmp_value = {0};
        register const GValue *value =
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
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const gchar *symbol;
    GValueArray *array;
    GList *list = NULL;

    if(!gel_context_eval_params(context, __FUNCTION__, &list,
            "sA*", &n_values, &values, &symbol, &array))
        return;

    const guint last = array->n_values;
    const GValue *const array_values = array->values;
    register guint i;
    for(i = 0; i < last; i++)
    {
        register GelContext *inner = gel_context_new_with_outer(context);
        gel_context_add_symbol(inner, symbol, gel_value_dup(array_values + i));
        do_(self, return_value, n_values, values, inner, marshal_data);
        gel_context_free(inner);
    }
    gel_value_list_free(list);
}


static
void while_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    register gboolean running = TRUE;
    register gboolean run = FALSE;
    while(running)
    {
        GValue tmp_value = {0};
        register const GValue *cond_value =
            gel_context_eval_value(context, values + 0, &tmp_value);
        if(gel_value_to_boolean(cond_value))
        {
            run = TRUE;
            do_(self, return_value, n_values-1, values+1,
                context, marshal_data);
        }
        else
        {
            running = FALSE;
            if(run == FALSE)
                gel_value_copy(cond_value, return_value);
        }
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
}


static
void if_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    register const GValue *cond_value =
        gel_context_eval_value(context, values + 0, &tmp_value);

    if(gel_value_to_boolean(cond_value))
        do_(self, return_value, n_values-1, values+1,
            context, marshal_data);
    else
        gel_value_copy(cond_value, return_value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void str_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    register const GValue *value =
        gel_context_eval_value(context, values + 0, &tmp_value);

    register gchar *value_string = gel_value_to_string(value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);

    g_value_init(return_value, G_TYPE_STRING);
    g_value_take_string(return_value, value_string);
}


static
void type_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    register const GValue *value =
        gel_context_eval_value(context, values + 0, &tmp_value);
    register GType value_type = GEL_VALUE_TYPE(value);
    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);

    g_value_init(return_value, G_TYPE_STRING);
    g_value_set_string(return_value, g_type_name(value_type));
}


static
void add_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    arithmetic(self, return_value, n_values, values, context,
        gel_values_add);
}


static
void sub_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    arithmetic(self, return_value, n_values, values, context,
        gel_values_sub);
}


static
void mul_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    arithmetic(self, return_value, n_values, values, context,
        gel_values_mul);
}


static
void div_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    arithmetic(self, return_value, n_values, values, context, 
        gel_values_div);
}


static
void mod_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    arithmetic(self, return_value, n_values, values, context, 
        gel_values_mod);
}


static
void gt_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    logic(self, return_value, n_values, values, context, gel_values_gt);
}


static
void ge_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    logic(self, return_value, n_values, values, context, gel_values_ge);
}


static
void eq_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    logic(self, return_value, n_values, values, context, gel_values_eq);
}


static
void le_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    logic(self, return_value, n_values, values, context, gel_values_le);
}


static
void lt_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    logic(self, return_value, n_values, values, context, gel_values_lt);
}


static
void ne_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    logic(self, return_value, n_values, values, context, gel_values_ne);
}


#define CLOSURE(S) \
    {#S, \
     gel_value_new_from_closure_marshal((GClosureMarshal)S##_, self)}

/**
 * gel_context_add_default_symbols:
 * @self: context
 *
 * Adds a bunch of predefined functions and symbols to @self.
 * This method is automatically called for contexts created with
 * #gel_context_new.
 *
 * Current default symbols are:
 * <itemizedlist>
 *   <listitem><para>
 *    [let symbol value]
 *   </para></listitem>
 *   <listitem><para>
 *    [var symbol value]
 *   </para></listitem>
 *   <listitem><para>
 *    [new typename]
 *   </para></listitem>
 *   <listitem><para>
 *    [quote literal]
 *   </para></listitem>
 *   <listitem><para>
 *    [get object property]
 *   </para></listitem>
 *   <listitem><para>
 *    [set object property value]
 *   </para></listitem>
 *   <listitem><para>
 *    [def name [args...] code...]
 *   </para></listitem>
 *   <listitem><para>
 *    [lambda [args...] code...]
 *   </para></listitem>
 *   <listitem><para>
 *    [connect object signal callback]
 *   </para></listitem>
 *   <listitem><para>
 *    [print values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [case value ifeq_1 then_1 ... ifeq_n then_n default]
 *   </para></listitem>
 *   <listitem><para>
 *    [cond test_1 value_1 ... test_n value_n default]
 *   </para></listitem>
 *   <listitem><para>
 *    [do code...]
 *   </para></listitem>
 *   <listitem><para>
 *    [array values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [for iter array code...]
 *   </para></listitem>
 *   <listitem><para>
 *    [while test ...]
 *   </para></listitem>
 *   <listitem><para>
 *    [if test code...]
 *   </para></listitem>
 *   <listitem><para>
 *    [str value]
 *   </para></listitem>
 *   <listitem><para>
 *    [type value]
 *   </para></listitem>
 *   <listitem><para>
 *    [add values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [sub values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [mul values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [div values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [mod values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [and values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [not value]
 *   </para></listitem>
 *   <listitem><para>
 *    [or values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [any function array]
 *   </para></listitem>
 *   <listitem><para>
 *    [all function array]
 *   </para></listitem>
 *   <listitem><para>
 *    [append array values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [nth array index]
 *   </para></listitem>
 *   <listitem><para>
 *    [remove array index]
 *   </para></listitem>
 *   <listitem><para>
 *    [head array]
 *   </para></listitem>
 *   <listitem><para>
 *    [tail array]
 *   </para></listitem>
 *   <listitem><para>
 *    [len array]
 *   </para></listitem>
 *   <listitem><para>
 *    [apply function array]
 *   </para></listitem>
 *   <listitem><para>
 *    [filter function array]
 *   </para></listitem>
 *   <listitem><para>
 *    [map function array]
 *   </para></listitem>
 *   <listitem><para>
 *    [sort array]
 *   </para></listitem>
 *   <listitem><para>
 *    [gt values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [ge values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [eq values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [lt values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [le values...]
 *   </para></listitem>
 *   <listitem><para>
 *    [ne values...]
 *   </para></listitem>
 *   <listitem><para>
 *    TRUE
 *   </para></listitem>
 *   <listitem><para>
 *    FALSE
 *   </para></listitem>
 *   <listitem><para>
 *    NULL
 *   </para></listitem>
 * </itemizedlist>
 */


void gel_context_add_default_symbols(GelContext *self)
{
    struct {const gchar *name; GValue *value;} *p, symbols[] =
    {
        CLOSURE(let),
        CLOSURE(var),
        CLOSURE(new),
        CLOSURE(quote),
        CLOSURE(get),
        CLOSURE(set),
        CLOSURE(def),
        CLOSURE(lambda),
        CLOSURE(connect),
        CLOSURE(print),
        CLOSURE(case),
        CLOSURE(cond),
        CLOSURE(do),
        CLOSURE(array),
        CLOSURE(for),
        CLOSURE(while),
        CLOSURE(if),
        CLOSURE(str),
        CLOSURE(type),
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
        CLOSURE(any),
        CLOSURE(all),
        CLOSURE(append),
        CLOSURE(nth),
        CLOSURE(remove),
        CLOSURE(head),
        CLOSURE(tail),
        CLOSURE(len),
        CLOSURE(apply),
        CLOSURE(filter),
        CLOSURE(map),
        CLOSURE(sort),
        {"TRUE", gel_value_new_from_boolean(TRUE)},
        {"FALSE", gel_value_new_from_boolean(FALSE)},
        {"NULL", gel_value_new_from_pointer(NULL)},
        {NULL,NULL}
    };

    for(p = symbols; p->name != NULL; p++)
        gel_context_add_symbol(self, p->name, p->value);
}

