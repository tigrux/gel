#include <gelvalue.h>
#include <gelcontext.h>
#include <gelclosure.h>
#include <geldebug.h>
#include <gelvaluelist.h>


static
void quit_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           gpointer invocation_hint, gpointer marshal_data)
{
    GelContext *context = (GelContext*)self->data;
    g_signal_emit_by_name(context, "quit", NULL);
}


static
void let_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const gchar *symbol = NULL;
    GValue *value = NULL;
    GList *list = NULL;

     if(!gel_context_eval_params(context, &list, "sV", &n_values, &values,
            &symbol, &value))
        return;

    GValue *symbol_value = gel_context_find_symbol(context, symbol);
    if(symbol_value != NULL)
    {
        if(gel_value_copy(value, symbol_value))
            gel_value_copy(symbol_value, return_value);
    }
    else
        gel_warning_unknown_symbol(symbol);
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

     if(!gel_context_eval_params(context, &list, "sV", &n_values, &values,
            &symbol, &value))
        return;

    list = g_list_remove(list, value);
    gel_context_add_symbol(context, symbol, value);
    gel_value_copy(value, return_value);
    gel_value_list_free(list);
}


static
void new_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const gchar *type_name = NULL;
    GList *list = NULL;

    if(!gel_context_eval_params(context, &list,
            "s", &n_values, &values, &type_name))
        return;

    GType type = g_type_from_name(type_name);
    if(type != G_TYPE_INVALID)
    {
        if(G_TYPE_IS_INSTANTIATABLE(type))
        {
            GObject *new_object = (GObject*)(g_object_new(type, NULL));
            if(G_IS_INITIALLY_UNOWNED(new_object))
                g_object_ref_sink(new_object);

            GValue result_value = {0};
            g_value_init(&result_value, type);
            g_value_take_object(&result_value, new_object);
            gel_value_copy(&result_value, return_value);
            g_value_unset(&result_value);
        }
        else
            gel_warning_type_not_instantiatable(type);
    }
    else
        gel_warning_type_name_invalid(type_name);

    gel_value_list_free(list);
}


static
void quote_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values == n_args)
    {
        GValue result_value = {0};
        g_value_init(&result_value, G_TYPE_STRING);

        g_value_take_string(&result_value, gel_value_to_string(values + 0));
        gel_value_copy(&result_value, return_value);
        g_value_unset(&result_value);
    }
    else
        gel_warning_needs_n_arguments(n_args);
}


static
void get_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    GObject *object = NULL;
    const gchar *prop_name = NULL;
    GList *list = NULL;

    if(!gel_context_eval_params(context, &list, "OS", &n_values, &values,
            &object, &prop_name))
        return;

    if(G_IS_OBJECT(object))
    {
        GParamSpec *prop_spec =
            g_object_class_find_property(G_OBJECT_GET_CLASS(object), prop_name);
        if(prop_spec != NULL)
        {
            GValue result_value = {0};
            g_value_init(&result_value, prop_spec->value_type);
            g_object_get_property(object, prop_name, &result_value);
            gel_value_copy(&result_value, return_value);
            g_value_unset(&result_value);
        }
        else
            gel_warning_no_such_property(prop_name);
    }
    else
        gel_warning_value_not_of_type(
            (GValue*)g_list_nth_data(list, 0), G_TYPE_OBJECT);

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

    if(!gel_context_eval_params(context, &list, "OSV", &n_values, &values,
            &object, &prop_name, &value))
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
                gel_value_copy(&result_value, return_value);
                g_value_unset(&result_value);
            }
            else
                gel_warning_invalid_value_for_property(value, prop_spec);
        }
        else
            gel_warning_no_such_property(prop_name);
    }
    else
        gel_warning_value_not_of_type(
            (GValue*)g_list_nth_data(list, 0), G_TYPE_OBJECT);
    gel_value_list_free(list);
}


static
GClosure* new_closure(GelContext *context,
                      GValueArray *vars_array,
                      guint n_values, const GValue *values)
{
    const guint n_vars = vars_array->n_values;
    gchar **vars = (gchar**)g_new0(gchar*, n_vars + 1);
    gchar *invalid_arg_name = NULL;
    register guint i;
    for(i = 0; i < n_vars; i++)
    {
        GValue *value = vars_array->values + i;
        if(G_VALUE_HOLDS_STRING(value))
            vars[i] = g_value_dup_string(value);
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
            g_value_array_append(code, values+i);
        self = gel_closure_new(context, vars, code);
    }
    else
    {
        gel_warning_invalid_argument_name(invalid_arg_name);
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

    if(!gel_context_eval_params(context, &list, "sa*", &n_values, &values,
            &symbol, &array))
        return;

    GClosure *closure = new_closure(context, array, n_values, values);
    if(closure != NULL)
    {
        GValue *value = gel_value_new_from_closure(closure);
        gel_context_add_symbol(context, symbol, value);
        gel_value_copy(value, return_value);
    }
}


static
void lambda_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values,
             GelContext *context, gpointer marshal_data)
{
    GValueArray *array;
    GList *list = NULL;

    if(!gel_context_eval_params(context, &list, "a*", &n_values, &values,
            &array))
        return;

    GClosure *closure = new_closure(context, array, n_values, values);

    if(closure != NULL)
    {
        GValue result_value = {0};
        g_value_init(&result_value, G_TYPE_CLOSURE);
        g_value_take_boxed(&result_value, closure);
        gel_value_copy(&result_value, return_value);
        g_value_unset(&result_value);
    }
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

    if(!gel_context_eval_params(context, &list, "OSC", &n_values, &values,
            &object, &signal, &callback))
        return;

    if(G_IS_OBJECT(object))
    {
        GValue result_value = {0};
        g_value_init(&result_value, G_TYPE_UINT);

        g_value_set_uint(&result_value,
            g_signal_connect_closure(object, signal, callback, FALSE));
        gel_value_copy(&result_value, return_value);

        g_value_unset(&result_value);
    }
    else
        gel_warning_value_not_of_type(
            (GValue*)g_list_nth_data(list, 0), G_TYPE_OBJECT);
    gel_value_list_free(list);
}


static
void print_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    GString *string_repr = g_string_new(NULL);
    if(n_values > 0)
    {
        const guint last = n_values - 1;
        register guint i;
        for(i = 0; i <= last; i++)
        {
            GValue value = {0};
            gel_context_eval_value(context, values + i, &value);
            gchar *value_string = gel_value_to_string(&value);
            g_value_unset(&value);
            g_string_append_printf(string_repr, "%s", value_string);
            g_free(value_string);
        }
    }
    GValue result_value = {0};
    g_value_init(&result_value, G_TYPE_STRING);
    gchar *result_string = g_string_free(string_repr, FALSE);
    g_print("%s\n", result_string);
    g_value_take_string(&result_value, result_string);
    gel_value_copy(&result_value, return_value);
    g_value_unset(&result_value);
}


static
void arithmetic(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values,
                GelContext *context, GelValuesArithmetic values_function)
{
    const guint n_args = 2;
    if(n_values >= n_args)
    {
        GValue l_value = {0};
        gel_context_eval_value(context, values + 0, &l_value);

        GValue r_value = {0};
        gel_context_eval_value(context, values + 1, &r_value);

        GValue result_value = {0};
        if(values_function(&l_value, &r_value, &result_value))
        {
            const guint last = n_values;
            register guint i;
            for(i = 2; i < last; i++)
            {
                g_value_unset(&l_value);
                gel_value_copy(&result_value, &l_value);

                g_value_unset(&r_value);
                gel_context_eval_value(context, values + i, &r_value);

                g_value_unset(&result_value);
                if(!values_function(&l_value, &r_value, &result_value))
                    break;
            }
            if(i == last)
            {
                gel_value_copy(&result_value, return_value);
                g_value_unset(&result_value);
            }
            g_value_unset(&l_value);
            g_value_unset(&r_value);
        }
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void logic(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, GelValuesLogic values_function)
{
    const guint n_args = 2;
    if(n_values >= n_args)
    {
        gboolean result = TRUE;
        const guint last = n_values - 1;
        register guint i;
        for(i = 0; i < last && result; i++)
        {
            GValue l_value = {0};
            gel_context_eval_value(context, values + i, &l_value);

            GValue r_value = {0};
            gel_context_eval_value(context, values + i + 1, &r_value);

            result = values_function(&l_value, &r_value);

            g_value_unset(&l_value);
            g_value_unset(&r_value);
        }

        GValue result_value = {0};
        g_value_init(&result_value, G_TYPE_BOOLEAN);
        g_value_set_boolean(&result_value, result);

        gel_value_copy(&result_value, return_value);
        g_value_unset(&result_value);
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void not_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values == n_args)
    {
        GValue value = {0};
        gel_context_eval_value(context, values + 0, &value);
        gboolean value_bool = gel_value_to_boolean(&value);
        g_value_unset(&value);

        g_value_init(&value, G_TYPE_BOOLEAN);
        g_value_set_boolean(&value, !value_bool);
        gel_value_copy(&value, return_value);
        g_value_unset(&value);
    }
    else
        gel_warning_needs_n_arguments(n_args);
}


static
void and_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values >= n_args)
    {
        gboolean result = TRUE;
        GValue result_value = {0};
        const guint last = n_values - 1;
        register guint i;
        for(i = 0; i <= last && result == TRUE; i++)
        {
            GValue value = {0};
            gel_context_eval_value(context, values + i, &value);
            result = gel_value_to_boolean(&value);
            if(!result || i == last)
                gel_value_copy(&value, &result_value);
            g_value_unset(&value);
        }
        gel_value_copy(&result_value, return_value);
        g_value_unset(&result_value);
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void or_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values >= n_args)
    {
        GValue result_value = {0};
        const guint last =  n_values - 1;
        register gboolean result = FALSE;
        register guint i;
        for(i = 0; i <= last && result == FALSE; i++)
        {
            GValue value = {0};
            gel_context_eval_value(context, values + i, &value);
            result = gel_value_to_boolean(&value);
            if(result || i == last)
                gel_value_copy(&value, &result_value);
            g_value_unset(&value);
        }
        gel_value_copy(&result_value, return_value);
        g_value_unset(&result_value);
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void any_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
          GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, &list, "CA", &n_values, &values,
            &closure, &array))
        return;

    gboolean result = FALSE;
    register guint i;
    for(i = 0; i < array->n_values && !result; i++)
    {
        GValue value = {0};
        g_closure_invoke(closure, &value, 1, array->values + i, context);
        result = gel_value_to_boolean(&value);
        g_value_unset(&value);
    }

    GValue result_value = {0};
    g_value_init(&result_value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&result_value, result);
    gel_value_copy(&result_value, return_value);

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

    if(!gel_context_eval_params(context, &list, "CA", &n_values, &values,
            &closure, &array))
        return;

    gboolean result = TRUE;
    register guint i;
    for(i = 0; i < array->n_values && result; i++)
    {
        GValue value = {0};
        g_closure_invoke(closure, &value, 1, array->values + i, context);
        result = gel_value_to_boolean(&value);
        g_value_unset(&value);
    }

    GValue result_value = {0};
    g_value_init(&result_value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&result_value, result);
    gel_value_copy(&result_value, return_value);

    gel_value_list_free(list);
}


static
void head_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, &list, "A", &n_values, &values,
            &array))
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

    if(!gel_context_eval_params(context, &list, "A", &n_values, &values,
            &array))
        return;

    if(array->n_values > 0)
    {
        const guint tail_len = array->n_values - 1;
        GValueArray *tail = g_value_array_new(tail_len);
        register guint i;
        for(i = 1; i <= tail_len; i++)
            g_value_array_append(tail, array->values + i);
        GValue result_value = {0};
        g_value_init(&result_value, G_TYPE_VALUE_ARRAY);
        g_value_take_boxed(&result_value, tail);
        gel_value_copy(&result_value, return_value);
        g_value_unset(&result_value);
    }

    gel_value_list_free(list);
}

//
static
void len_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    GList *list = NULL;
    GValueArray *array = NULL;

    if(!gel_context_eval_params(context, &list, "A", &n_values, &values,
            &array))
        return;

    GValue result_value = {0};
    g_value_init(&result_value, G_TYPE_UINT);
    g_value_set_uint(&result_value, array->n_values);
    gel_value_copy(&result_value, return_value);
    g_value_unset(&result_value);

    gel_value_list_free(list);
}

//

void branch(guint n_values, const GValue *values, GelContext *outer,
            GValue *return_value, const GValue *case_value)
{
    register gboolean match = FALSE;
    register gboolean testing = TRUE;

    GelContext *context = gel_context_new_with_outer(outer);
    while(testing)
    {
        if(n_values > 1)
        {
            GValue if_value = {0};
            gel_context_eval_value(context, values + 0, &if_value);

            if(case_value != NULL)
                match = gel_values_eq(&if_value, case_value);
            else
                match = gel_value_to_boolean(&if_value);
            g_value_unset(&if_value);

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
            GValue then_value = {0};
            gel_context_eval_value(context, values + 0, &then_value);
            gel_value_copy(&then_value, return_value);
            g_value_unset(&then_value);
            testing = FALSE;
        }
    }
    gel_context_unref(context);
}


static
void case_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values >= n_args)
    {
        GValue case_value = {0};
        gel_context_eval_value(context, values + 0, &case_value);
        n_values--, values++;
        branch(n_values, values, context, return_value, &case_value);
        g_value_unset(&case_value);
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void cond_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values >= n_args)
        branch(n_values, values, context, return_value, NULL);
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void do_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *outer, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values >= n_args)
    {
        const guint last = n_values-1;
        register guint i;
        GelContext *context = gel_context_new_with_outer(outer);
        for(i = 0; i <= last; i++)
        {
            GValue value = {0};
            gel_context_eval_value(context, values + i, &value);
            if(i == last)
                gel_value_copy(&value, return_value);
            g_value_unset(&value);
        }
        gel_context_unref(context);
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void list_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    GValueArray *array = g_value_array_new(n_values);
    register guint i;
    for(i = 0; i < n_values; i++)
    {
        GValue value = {0};
        gel_context_eval_value(context, values + i, &value);
        g_value_array_append(array, &value);
        g_value_unset(&value);
    }
    GValue result_value = {0};
    g_value_init(&result_value, G_TYPE_VALUE_ARRAY);
    g_value_take_boxed(&result_value, array);
    gel_value_copy(&result_value, return_value);
    g_value_unset(&result_value);
}


static
void for_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    const gchar *symbol;
    GValueArray *array;
    GList *list = NULL;

    if(!gel_context_eval_params(context, &list, "sA*", &n_values, &values,
            &symbol, &array))
        return;

    register guint i;
    for(i = 0; i < array->n_values; i++)
    {
        GelContext *inner = gel_context_new_with_outer(context);
        gel_context_add_symbol(inner, symbol, gel_value_dup(array->values + i));
        do_(self, return_value, n_values, values,
            inner, marshal_data);
        gel_context_unref(inner);
    }
    gel_value_list_free(list);
}


static
void while_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values,
            GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values >= n_args)
    {
        register gboolean running = TRUE;
        gboolean run = FALSE;
        while(running)
        {
            GValue cond_value = {0};
            gel_context_eval_value(context, values + 0, &cond_value);
            if(gel_value_to_boolean(&cond_value))
            {
                run = TRUE;
                do_(self, return_value, n_values-1, values+1,
                    context, marshal_data);
            }
            else
            {
                running = FALSE;
                if(run == FALSE)
                    gel_value_copy(&cond_value, return_value);
            }
            g_value_unset(&cond_value);
        }
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void if_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values,
         GelContext *context, gpointer marshal_data)
{
    const guint n_args = 2;
    if(n_values >= n_args)
    {
        GValue cond_value = {0};
        gel_context_eval_value(context, values + 0, &cond_value);
        if(gel_value_to_boolean(&cond_value))
            do_(self, return_value, n_values-1, values+1,
                context, marshal_data);
        else
            gel_value_copy(&cond_value, return_value);
        g_value_unset(&cond_value);
    }
    else
        gel_warning_needs_at_least_n_arguments(n_args);
}


static
void str_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values == n_args)
    {
        GValue value = {0};
        gel_context_eval_value(context, values + 0, &value);
        gchar *value_string = gel_value_to_string(&value);
        g_value_unset(&value);

        g_value_init(&value, G_TYPE_STRING);
        g_value_take_string(&value, value_string);
        gel_value_copy(&value, return_value);
        g_value_unset(&value);
    }
    else
        gel_warning_needs_n_arguments(n_args);
}


static
void type_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values,
           GelContext *context, gpointer marshal_data)
{
    const guint n_args = 1;
    if(n_values == n_args)
    {
        GValue value = {0};
        gel_context_eval_value(context, values + 0, &value);
        GType value_type = G_VALUE_TYPE(&value);
        g_value_unset(&value);

        g_value_init(&value, G_TYPE_GTYPE);
        g_value_set_gtype(&value, value_type);
        gel_value_copy(&value, return_value);
        g_value_unset(&value);
    }
    else
        gel_warning_needs_n_arguments(n_args);
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
     gel_value_new_closure_from_marshall((GClosureMarshal)S##_, G_OBJECT(self))}


void gel_context_add_default_symbols(GelContext *self)
{
    struct Symbol {const gchar *name; GValue *value;} symbols[] =
    {
        CLOSURE(quit),
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
        CLOSURE(list),
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
        CLOSURE(any),
        CLOSURE(all),
        CLOSURE(head),
        CLOSURE(tail),
        CLOSURE(len),
        CLOSURE(gt),
        CLOSURE(ge),
        CLOSURE(eq),
        CLOSURE(lt),
        CLOSURE(le),
        CLOSURE(ne),
        {"TRUE", gel_value_new_from_boolean(TRUE)},
        {"FALSE", gel_value_new_from_boolean(FALSE)},
        {"NULL", gel_value_new_from_pointer(NULL)},
        {NULL,NULL}
    };

    register struct Symbol *p;
    for(p = symbols; p->name != NULL; p++)
        gel_context_add_symbol(self, p->name, p->value);
}

