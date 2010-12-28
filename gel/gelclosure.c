#include <gelcontext.h>
#include <gelclosure.h>
#include <gelvalue.h>


struct _GelClosure
{
    GClosure closure;

    gchar **vars;
    GValueArray *code;
};


static
void gel_closure_finalize(GelContext *context, GelClosure *closure)
{
    g_return_if_fail(context != NULL);

    g_strfreev(closure->vars);
    g_value_array_free(closure->code);
}


static
void gel_closure_marshal(GelClosure *closure, GValue *return_value,
                         guint n_param_values, const GValue *param_values,
                         gpointer invocation_hint, gpointer marshal_data)
{
    const guint n_vars = g_strv_length(closure->vars);
    g_return_if_fail(n_param_values == n_vars);

    GelContext *outer = (GelContext*)closure->closure.data;
    GelContext *context = gel_context_new_with_outer(outer);

    register guint i;
    for(i = 0; i < n_vars; i++)
        gel_context_add_symbol(context,
            closure->vars[i], gel_value_dup(param_values + i));

    const guint n_values = closure->code->n_values;
    if(n_values > 0)
    {
        const guint last = n_values - 1;
        for(i = 0; i <= last; i++)
        {
            GValue tmp_value = {0};
            const GValue *value = gel_context_eval_value(context,
                closure->code->values + i, &tmp_value);
            if(i == last && return_value != NULL)
                gel_value_copy(value, return_value);
            if(G_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
        }
    }
    gel_context_unref(context);
}


GClosure* gel_closure_new(GelContext *context, gchar **vars, GValueArray *code)
{
    g_return_val_if_fail(context != NULL, NULL);
    g_return_val_if_fail(vars != NULL, NULL);
    g_return_val_if_fail(code != NULL, NULL);

    GClosure *closure =
        g_closure_new_object(sizeof(GelClosure), G_OBJECT(context));

    g_closure_set_marshal(closure, (GClosureMarshal)gel_closure_marshal);
    g_closure_add_finalize_notifier(
        closure, context, (GClosureNotify)gel_closure_finalize);

    GelClosure *gel_closure = (GelClosure*)closure;
    gel_closure->vars = vars;
    gel_closure->code = code;

    return closure;
}


gboolean gel_closure_is_pure(GClosure *closure)
{
    g_return_val_if_fail(closure != NULL, FALSE);
    return closure->marshal == (GClosureMarshal)gel_closure_marshal;
}

