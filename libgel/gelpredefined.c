#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelclosure.h>
#include <geltypelib.h>


static
GClosure* fn(GelContext *context, const gchar *name,
             guint n_vars, const GValue *var_values,
             guint n_values, const GValue *values)
{
    gchar **vars = g_new0(gchar*, n_vars + 1);
    gchar *invalid_arg_name = NULL;

    for(guint i = 0; i < n_vars; i++)
    {
        const GValue *value = var_values + i;
        if(GEL_VALUE_HOLDS(value, GEL_TYPE_SYMBOL))
        {
            GelSymbol *symbol = gel_value_get_boxed(value);
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
        for(guint i = 0; i < n_values; i++)
            g_value_array_append(code, values + i);
        self = gel_closure_new(name, vars, code, context);
        g_closure_ref(self);
        g_closure_sink(self);
    }
    else
    {
        gel_warning_invalid_argument_name(__FUNCTION__, invalid_arg_name);
        g_free(invalid_arg_name);
        for(guint i = 0; i < n_vars; i++)
            if(vars[i] != NULL)
                g_free(vars[i]);
        g_free(vars);
        self = NULL;
    }

    return self;
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
            gel_warning_symbol_exists(__FUNCTION__, name);
        else
            gel_context_insert(context, name, gel_value_dup(value));
    }

    gel_value_list_free(tmp_list);
}


static
void defn_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    gchar *name = NULL;
    GValueArray *vars = NULL;

    if(gel_context_eval_params(context, __FUNCTION__, &n_values, &values, 
            &tmp_list, "sa*", &name, &vars))
    {
        if(gel_context_get_variable(context, name) != NULL)
            gel_warning_symbol_exists(__FUNCTION__, name);
        else
        {
            GClosure *closure = fn(context, name,
                                   vars->n_values, vars->values,
                                   n_values, values);
            if(closure != NULL)
            {
                GValue *value =
                    gel_value_new_from_boxed(G_TYPE_CLOSURE, closure);
                gel_context_insert(context, name, value);
                gel_closure_close_over(closure);
            }
        }
    }

    gel_value_list_free(tmp_list);
}


static
void do_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    guint last = n_values-1;
    for(guint i = 0; i <= last; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);
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
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;
    GValueArray *bindings = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "a*", &bindings))
    {
        guint binding_n_values = bindings->n_values;
        const GValue *binding_values = bindings->values;
        
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
                    gel_context_insert(let_context, name, gel_value_dup(value));
                else
                    failed = TRUE;
            }

            if(!failed)
                do_(self, return_value, n_values, values, let_context);

            gel_context_free(let_context);
        }
        else
            g_warning("%s: bindings must be pairs", __FUNCTION__);
    }

    gel_value_list_free(tmp_list);
}


static
void fn_(GClosure *self, GValue *return_value,
         guint n_values, const GValue *values, GelContext *context)
{
    GValueArray *array;
    GList *tmp_list = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "a*", &array))
    {
        GClosure *closure = fn(context, "lambda",
            array->n_values, array->values, n_values, values);
        if(closure != NULL)
        {
            gel_closure_close_over(closure);
            g_value_init(return_value, G_TYPE_CLOSURE);
            gel_value_take_boxed(return_value, closure);
        }
    }

    gel_value_list_free(tmp_list);
}


static
void fn_name_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "C", &closure))
    {
        const gchar *name = gel_closure_get_name(closure);
        g_value_init(return_value, G_TYPE_STRING);
        g_value_set_string(return_value, name);
    }

    gel_value_list_free(tmp_list);
}


static
void fn_args_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "C", &closure))
    {
        gchar **args = gel_closure_get_args(closure);
        if(args != NULL)
        {
            guint n = g_strv_length(args);
            GValueArray *array = g_value_array_new(n);
            array->n_values = n;
            GValue *array_values = array->values;

            for(guint i = 0; i < n; i++)
            {
                g_value_init(array_values + i, G_TYPE_STRING);
                g_value_take_string(array_values + i, args[i]);
            }

            g_free(args);
            g_value_init(return_value, G_TYPE_VALUE_ARRAY);
            g_value_take_boxed(return_value, array);
        }
    }

    gel_value_list_free(tmp_list);
}


static
void fn_code_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "C", &closure))
    {
        GValueArray *code = gel_closure_get_code(closure);
        g_value_init(return_value, G_TYPE_VALUE_ARRAY);
        gel_value_take_boxed(return_value, code);
    }

    gel_value_list_free(tmp_list);
}


static
void fn_call_(GClosure *self, GValue *return_value,
              guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "CA", &closure, &array))
        g_closure_invoke(closure, return_value,
            array->n_values, array->values, context);
    gel_value_list_free(tmp_list);
}


static
void fn_map_(GClosure *self, GValue *return_value,
             guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GClosure *closure = NULL;
    GValueArray *array = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "CA", &closure, &array))
    {
        const GValue *array_values = array->values;
        const guint array_n_values = array->n_values;

        GValueArray *result_array = g_value_array_new(array_n_values);
        result_array->n_values = array_n_values;
        GValue *result_array_values = result_array->values;

        for(guint i = 0; i < array_n_values; i++)
            g_closure_invoke(closure,
                result_array_values + i, 1, array_values + i, context);

        g_value_init(return_value, G_TYPE_VALUE_ARRAY);
        gel_value_take_boxed(return_value, result_array);
    }

    gel_value_list_free(tmp_list);
}


static
void set_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{

    guint n_args = 2;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
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
        if(src_value != NULL)
            copied = gel_value_copy(src_value, dest_value);
    }

    if(!copied)
        gel_warning_expected(__FUNCTION__, "symbol or variable");

    if(GEL_IS_VALUE(&tmp1_value))
        g_value_unset(&tmp1_value);

    if(GEL_IS_VALUE(&tmp2_value))
        g_value_unset(&tmp2_value);
}


static
void var_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 1;
    if(n_values != n_args)
    {
        gel_warning_needs_n_arguments(__FUNCTION__, n_args);
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
            gel_warning_unknown_symbol(__FUNCTION__, name);
        }
    }
    else
        gel_warning_value_not_of_type(__FUNCTION__, values, GEL_TYPE_SYMBOL);
}


static
void if_(GClosure *self, GValue *return_value,
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
        gel_context_eval_into_value(context, values + 0, &tmp_value);

    if(gel_value_to_boolean(cond_value))
        gel_context_eval(context, values + 1, return_value);
    else
        if(n_values > 2)
            gel_context_eval(context, values + 2, return_value);

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
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GList *tmp_list = NULL;

    while(n_values > 0)
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

    gel_value_list_free(tmp_list);
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

    GList *tmp_list = NULL;
    GValue *probe_value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "V*", &probe_value))
        while(n_values > 0)
        {
            if(n_values == 1)
                do_(self, return_value, n_values--, values++, context);
            else
            {
                GValueArray *tests = NULL;
                GValue *value = NULL;
                if(gel_context_eval_params(context, __FUNCTION__,
                    &n_values, &values, &tmp_list, "av*", &tests, &value))
                {
                    const GValue *test_values = tests->values;
                    guint test_n_values = tests->n_values;
                    for(guint i = 0; i < test_n_values; i++)
                        if(gel_values_eq(test_values + i, probe_value))
                        {
                            do_(self, return_value, 1, value, context);
                            n_values = 0;
                            break;
                        }
                }
            }
        }

    gel_value_list_free(tmp_list);
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

    gboolean cond_is_true = FALSE;
    GelContext *loop_context = gel_context_new_with_outer(context);
    gboolean running = TRUE;

    while(running)
    {
        GValue tmp_value = {0};
        const GValue *cond_value =
            gel_context_eval_into_value(loop_context, values + 0, &tmp_value);

        cond_is_true = gel_value_to_boolean(cond_value);
        if(cond_is_true)
            do_(self, return_value, n_values-1, values+1, loop_context);
        else
            running = FALSE;

        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, !cond_is_true);
    gel_context_free(loop_context);
}


static
void for_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    const gchar *iter_name;
    GValueArray *array;
    GList *tmp_list = NULL;
    gboolean result = FALSE;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values,&tmp_list, "sA*", &iter_name, &array))
    {
        guint last = array->n_values;
        const GValue *array_values = array->values;

        GelContext *loop_context = gel_context_new_with_outer(context);
        GValue *iter_value = gel_value_new();
        gel_context_insert(loop_context, iter_name, iter_value);

        guint i;
        for(i = 0; i < last; i++)
        {
            gel_value_copy(array_values + i, iter_value);
            do_(self, return_value, n_values, values, loop_context);
            g_value_unset(iter_value);
        }
        if(i == last)
            result = TRUE;  
        gel_context_free(loop_context);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, result);

    gel_value_list_free(tmp_list);
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
        GValueArray *array = g_value_array_new(ABS(last-first) + 1);
        GValue value = {0};
        g_value_init(&value, G_TYPE_INT64);

        if(last > first)
            for(gint64 i = first; i <= last; i++)
            {
                gel_value_set_int64(&value, i);
                g_value_array_append(array, &value);
            }
        else
            for(gint64 i = first; i >= last; i--)
            {
                gel_value_set_int64(&value, i);
                g_value_array_append(array, &value);
            }

        g_value_unset(&value);
        g_value_init(return_value, G_TYPE_VALUE_ARRAY);
        gel_value_take_boxed(return_value, array);
    }

    gel_value_list_free(tmp_list);
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
    for(guint i = 0; i <= last; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);
        if(GEL_IS_VALUE(value))
        {
            gchar *value_string = gel_value_to_string(value);
            g_print("%s%s", value_string, i == last ? "" : " ");
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
            gel_context_eval_into_value(context, values + 0, &tmp1),
            gel_context_eval_into_value(context, values + 1, &tmp2),
            &tmp3);

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

        running = values_function(
            v1, gel_context_eval_into_value(context, values + i, &tmp1), v2);

        if(GEL_IS_VALUE(v1))
            g_value_unset(v1);
        if(GEL_IS_VALUE(&tmp1))
            g_value_unset(&tmp1);
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

    for(guint i = 0; i < last && result; i++)
    {
        GValue tmp1 = {0};
        GValue tmp2 = {0};
        const GValue *i_value = values + i;
        result = values_function(
            gel_context_eval_into_value(context, i_value, &tmp1),
            gel_context_eval_into_value(context, i_value+1, &tmp2));

        if(GEL_IS_VALUE(&tmp1))
            g_value_unset(&tmp1);
        if(GEL_IS_VALUE(&tmp2))
            g_value_unset(&tmp2);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, result);
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
    for(guint i = 0; i <= last && result == TRUE; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);
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
    for(guint i = 0; i <= last && result == FALSE; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);
        result = gel_value_to_boolean(value);
        if(result || i == last)
            gel_value_copy(value, return_value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
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


static
void array_(GClosure *self, GValue *return_value,
            guint n_values, const GValue *values, GelContext *context)
{
    GValueArray *array = g_value_array_new(n_values);
    for(guint i = 0; i < n_values; i++)
    {
        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_into_value(context, values + i, &tmp_value);
        g_value_array_append(array, value);
        if(GEL_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    g_value_init(return_value, G_TYPE_VALUE_ARRAY);
    gel_value_take_boxed(return_value, array);
}


static
void array_append_(GClosure *self, GValue *return_value,
                   guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValueArray *array = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "A*", &array))
        for(guint i = 0; i < n_values; i++)
        {
            GValue tmp_value = {0};
            const GValue *value =
                gel_context_eval_into_value(context,values + i, &tmp_value);
            g_value_array_append(array, value);
            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
        }

    gel_value_list_free(tmp_list);
}


static
void array_get_(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValueArray *array = NULL;
    gint64 index = 0;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "AI", &array, &index)) 
    {
        guint array_n_values = array->n_values;
        if(index < 0)
            index += array_n_values;

        if(index < 0 || index >= array_n_values)
        {
            gel_warning_index_out_of_bounds(__FUNCTION__);
            return;
        }

        gel_value_copy(array->values + index, return_value);
    }

    gel_value_list_free(tmp_list);
}


static
void array_set_(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValueArray *array = NULL;
    gint64 index = 0;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "AIV", &array, &index, &value))
    {
        guint array_n_values = array->n_values;
        if(index < 0)
            index += array_n_values;
        if(index < 0 || index >= array_n_values)
        {
            gel_warning_index_out_of_bounds(__FUNCTION__);
            return;
        }
        gel_value_copy(value, array->values + index);
    }
    gel_value_list_free(tmp_list);
}


static
void array_remove_(GClosure *self, GValue *return_value,
                   guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValueArray *array = NULL;
    gint64 index = 0;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values,&tmp_list, "AI", &array, &index))
    {
        guint array_n_values = array->n_values;
        if(index < 0)
            index += array_n_values;
        if(index < 0 || index >= array_n_values)
        {
            gel_warning_index_out_of_bounds(__FUNCTION__);
            return;
        }
        g_value_array_remove(array, index);
    }
    gel_value_list_free(tmp_list);
}


static
void array_size_(GClosure *self, GValue *return_value,
                 guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValueArray *array = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "A", &array))
    {
        g_value_init(return_value, G_TYPE_INT64);
        gel_value_set_int64(return_value, array->n_values);
    }
    gel_value_list_free(tmp_list);
}


static
void array_find_(GClosure *self, GValue *return_value,
                 guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValueArray *array = NULL;
    GClosure *closure = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "AC", &array, &closure))
    {
        const GValue *array_values = array->values;
        gint64 result = -1;
        for(guint i = 0; i < array->n_values && result == -1; i++)
        {
            GValue value = {0};
            g_closure_invoke(closure, &value, 1, array_values + i, context);
            if(gel_value_to_boolean(&value))
                result = i;
            g_value_unset(&value);
        }

        g_value_init(return_value, G_TYPE_INT64);
        gel_value_set_int64(return_value, result);
    }
    gel_value_list_free(tmp_list);
}


static
void array_filter_(GClosure *self, GValue *return_value,
                   guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GValueArray *array = NULL;
    GClosure *closure = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "AC", &array, &closure))
    {
        GValueArray *result_array = g_value_array_new(array->n_values);
        for(guint i = 0; i < array->n_values; i++)
        {
            GValue tmp_value = {0};
            g_closure_invoke(closure,
                &tmp_value, 1, array->values + i, context);
            if(gel_value_to_boolean(&tmp_value))
                g_value_array_append(result_array, array->values + i);
            g_value_unset(&tmp_value);
        }

        g_value_init(return_value, G_TYPE_VALUE_ARRAY);
        gel_value_take_boxed(return_value, result_array);
    }
    gel_value_list_free(tmp_list);
}


static guint
gel_value_hash(const GValue *value)
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


static
void hash_(GClosure *self, GValue *return_value,
           guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = g_hash_table_new_full(
            (GHashFunc)gel_value_hash, (GEqualFunc)gel_values_eq,
            (GDestroyNotify)gel_value_free, (GDestroyNotify)gel_value_free);

    if(n_values % 2 == 0)
        while(n_values > 0)
        {
            GValue *key = NULL;
            GValue *value = NULL;
            if(gel_context_eval_params(context, __FUNCTION__,
                    &n_values, &values, &tmp_list, "VV*", &key, &value))
                g_hash_table_insert(hash,
                    gel_value_dup(key), gel_value_dup(value));
        }
    else
        g_warning("%s: arguments must be pairs", __FUNCTION__);

    g_value_init(return_value, G_TYPE_HASH_TABLE);
    gel_value_take_boxed(return_value, hash);
    gel_value_list_free(tmp_list);
}


static
void hash_append_(GClosure *self, GValue *return_value,
                  guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "H*", &hash))
    {
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
            g_warning("%s: arguments must be pairs", __FUNCTION__);
    }


    gel_value_list_free(tmp_list);
}


static
void hash_get_(GClosure *self, GValue *return_value,
               guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = NULL;
    GValue *key = NULL;
    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "HV", &hash, &key))
    {
        GValue *value = g_hash_table_lookup(hash, key);
        if(value != NULL)
            gel_value_copy(value, return_value);
    }

    gel_value_list_free(tmp_list);
}


static
void hash_set_(GClosure *self, GValue *return_value,
               guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = NULL;
    GValue *key = NULL;
    GValue *value = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "HVV", &hash, &key, &value))
        g_hash_table_insert(hash, gel_value_dup(key), gel_value_dup(value));

    gel_value_list_free(tmp_list);
}


static
void hash_remove_(GClosure *self, GValue *return_value,
                  guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = NULL;
    GValue *key = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "HV", &hash, &key))
        g_hash_table_remove(hash, key);

    gel_value_list_free(tmp_list);
}


static
void hash_size_(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "H", &hash))
    {
        g_value_init(return_value, G_TYPE_INT64);
        guint result = g_hash_table_size(hash);
        gel_value_set_int64(return_value, result);
    }

    gel_value_list_free(tmp_list);
}


static
void hash_keys_(GClosure *self, GValue *return_value,
                guint n_values, const GValue *values, GelContext *context)
{
    GList *tmp_list = NULL;
    GHashTable *hash = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "H", &hash))
    {
        guint size = g_hash_table_size(hash);
        GValueArray *array = g_value_array_new(size);
        GList *keys = g_hash_table_get_keys(hash);

        for(GList *iter = keys; iter != NULL; iter = g_list_next(iter))
            g_value_array_append(array, iter->data);
        g_value_init(return_value, G_TYPE_VALUE_ARRAY);
        gel_value_take_boxed(return_value, array);
        g_list_free(keys);
    }

    gel_value_list_free(tmp_list);
}


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
                gel_context_insert(context, namespace_, value);
            }    
        }
        else
            gel_warning_symbol_exists(__FUNCTION__, namespace_);
    }

    g_value_init(return_value, G_TYPE_BOOLEAN);
    gel_value_set_boolean(return_value, ns != NULL);

    gel_value_list_free(tmp_list);
}


static
void dot_(GClosure *self, GValue *return_value,
          guint n_values, const GValue *values, GelContext *context)
{
    guint n_args = 2;
    if(n_values < n_args)
    {
        gel_warning_needs_at_least_n_arguments(__FUNCTION__, n_args);
        return;
    }

    GValue tmp_value = {0};
    const GValue *value =
        gel_context_eval_into_value(context, values + 0, &tmp_value);

    values++;
    n_values--;

    const GelTypelib *typelib = NULL;
    const GelTypeInfo *type_info = NULL;
    GObject *object = NULL;

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
            gel_warning_type_name_invalid(__FUNCTION__, g_type_name(type));
    }
    else
    if(G_TYPE_IS_OBJECT(type))
    {
        type_info = gel_type_info_from_gtype(type);
        if(type_info == NULL)
            gel_warning_type_name_invalid(__FUNCTION__, g_type_name(type));
        else
            object = gel_value_get_object(value);
    }
    else
        gel_warning_expected(__FUNCTION__, "typelib, type or object");

    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);

    if(type_info == NULL && typelib == NULL)
        return;

    while(n_values > 0)
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
                g_warning("%s: Could not resolve '%s'", __FUNCTION__, name);
                break;
            }
        }
        else
            gel_warning_expected(__FUNCTION__, "symbol or string");

        values++;
        n_values--;    
    }

    if(type_info != NULL)
        gel_type_info_to_value(type_info, object, return_value);
}


static
void object_new_(GClosure *self, GValue *return_value,
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
        gel_context_eval_into_value(context, values + 0, &tmp_value);

    GType type = G_TYPE_INVALID;

    GType value_type = GEL_VALUE_TYPE(value);
    if(value_type == G_TYPE_STRING)
    {
        const gchar *type_name = gel_value_get_string(value);
        type = g_type_from_name(type_name);
        if(type == G_TYPE_INVALID)
            gel_warning_type_name_invalid(__FUNCTION__, type_name);
    }
    else
    if(value_type == G_TYPE_GTYPE)
        type = gel_value_get_gtype(&tmp_value);
    else
        gel_warning_expected(__FUNCTION__, "typename or type");

    if(type != G_TYPE_INVALID)
    {
        g_value_init(return_value, type);
        if(G_TYPE_IS_INSTANTIATABLE(type))
        {
            GObject *new_object = g_object_new(type, NULL);
            if(G_IS_INITIALLY_UNOWNED(new_object))
                g_object_ref_sink(new_object);
            gel_value_take_boxed(return_value, new_object);
        }
    }

    if(GEL_IS_VALUE(&tmp_value))
        g_value_unset(&tmp_value);
}


static
void object_get_(GClosure *self, GValue *return_value,
                 guint n_values, const GValue *values, GelContext *context)
{
    GObject *object = NULL;
    const gchar *name = NULL;
    GList *tmp_list = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "OS", &object, &name))
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
                gel_warning_no_such_property(__FUNCTION__, name);
        }

    gel_value_list_free(tmp_list);
}


static
void object_set_(GClosure *self, GValue *return_value,
                 guint n_values, const GValue *values, GelContext *context)
{
    GObject *object = NULL;
    gchar *name = NULL;
    GValue *value = NULL;
    GList *tmp_list = NULL;

    if(gel_context_eval_params(context, __FUNCTION__,
            &n_values, &values, &tmp_list, "OSV", &object, &name, &value))
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
                    gel_warning_invalid_value_for_property(__FUNCTION__,
                        value, spec);
            }
            else
                gel_warning_no_such_property(__FUNCTION__, name);
        }

    gel_value_list_free(tmp_list);
}


static
void object_connect_(GClosure *self, GValue *return_value,
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
            gel_warning_value_not_of_type(
                __FUNCTION__, values + 0, G_TYPE_OBJECT);
    }

    gel_value_list_free(tmp_list);
}


#define CLOSURE_NAME(N, S) {N, (GClosureMarshal)S##_}
#define CLOSURE(S) {#S, (GClosureMarshal)S##_}


static
GHashTable* gel_make_default_symbols(void)
{
    struct {const gchar *name; GClosureMarshal marshal;} *c, closures[] =
    {
        /* binding */
        CLOSURE(def),
        CLOSURE(defn),
        CLOSURE(do),
        CLOSURE(let),

        /* closures */
        CLOSURE(fn),
        CLOSURE_NAME("fn-name", fn_name),
        CLOSURE_NAME("fn-args", fn_args),
        CLOSURE_NAME("fn-code", fn_code),
        CLOSURE_NAME("fn-call", fn_call),
        CLOSURE_NAME("fn-map", fn_map),

        /* imperative */
        CLOSURE_NAME("set!", set),
        CLOSURE_NAME("var", var),

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

        /* arithmetic */
        CLOSURE_NAME("+", add),
        CLOSURE_NAME("-", sub),
        CLOSURE_NAME("*", mul),
        CLOSURE_NAME("/", div),
        CLOSURE_NAME("%", mod),

        /* logic */
        CLOSURE(and),
        CLOSURE(or),
        CLOSURE_NAME(">", gt),
        CLOSURE_NAME(">=", ge),
        CLOSURE_NAME("=", eq),
        CLOSURE_NAME("<", lt),
        CLOSURE_NAME("<=", le),
        CLOSURE_NAME("!=", ne),

        /* array */
        CLOSURE(array),
        CLOSURE_NAME("array-append!", array_append),
        CLOSURE_NAME("array-get", array_get),
        CLOSURE_NAME("array-set!", array_set),
        CLOSURE_NAME("array-remove!", array_remove),
        CLOSURE_NAME("array-size", array_size),
        CLOSURE_NAME("array-find", array_find),
        CLOSURE_NAME("array-filter", array_filter),

        /* hash */
        CLOSURE(hash),
        CLOSURE_NAME("hash-append!", hash_append),
        CLOSURE_NAME("hash-get", hash_get),
        CLOSURE_NAME("hash-set!", hash_set),
        CLOSURE_NAME("hash-remove!", hash_remove),
        CLOSURE_NAME("hash-size", hash_size),
        CLOSURE_NAME("hash-keys", hash_keys),

        /* introspection */
        CLOSURE(require),
        CLOSURE_NAME(".", dot),

        /* object */
        CLOSURE_NAME("object-new", object_new),
        CLOSURE_NAME("object-get", object_get),
        CLOSURE_NAME("object-set!", object_set),
        CLOSURE_NAME("object-connect", object_connect),

        {NULL,NULL}
    };

    GValue *value;
    GHashTable *symbols = g_hash_table_new_full(
            g_str_hash, g_str_equal,
            (GDestroyNotify)g_free, (GDestroyNotify)gel_value_free);

    for(c = closures; c->name != NULL; c++)
    {
        GClosure *closure = gel_closure_new_native(c->name, c->marshal);
        value = gel_value_new_from_boxed(G_TYPE_CLOSURE, closure);
        g_hash_table_insert(symbols, g_strdup(c->name), value);
    }

    value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    gel_value_set_boolean(value, TRUE);
    g_hash_table_insert(symbols, g_strdup("TRUE"), value);

    value = gel_value_new_of_type(G_TYPE_BOOLEAN);
    gel_value_set_boolean(value, FALSE);
    g_hash_table_insert(symbols, g_strdup("FALSE"), value);

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
const GValue *gel_value_lookup_predefined(const gchar *name)
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

