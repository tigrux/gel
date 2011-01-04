#include <gelcontext.h>
#include <gelclosure.h>
#include <gelvalueprivate.h>


/**
 * SECTION:gelclosure
 * @short_description: Closure used by functions written in gel.
 * @title: GelClosure
 * @include: gel.h
 *
 * A #GelClosure is a specialization of GClosure for gel function callbacks.
 */


struct _GelClosure
{
    GClosure closure;

    gchar **args;
    GValueArray *code;
};


static
void gel_closure_finalize(GelContext *context, GelClosure *closure)
{
    g_return_if_fail(context != NULL);

    g_strfreev(closure->args);
    g_value_array_free(closure->code);
}


static
void gel_closure_marshal(GelClosure *closure, GValue *return_value,
                         guint n_param_values, const GValue *param_values,
                         gpointer invocation_hint, gpointer marshal_data)
{
    const guint n_args = g_strv_length(closure->args);
    g_return_if_fail(n_param_values == n_args);

    GelContext *outer = (GelContext*)closure->closure.data;
    GelContext *context = gel_context_new_with_outer(outer);

    gchar **const closure_args = closure->args;
    register guint i;
    for(i = 0; i < n_args; i++)
        gel_context_add_symbol(context,
            closure_args[i], gel_value_dup(param_values + i));

    const guint n_values = closure->code->n_values;
    if(n_values > 0)
    {
        const guint last = n_values - 1;
        const GValue *const closure_code_values = closure->code->values;
        for(i = 0; i <= last; i++)
        {
            GValue tmp_value = {0};
            const GValue *value = gel_context_eval_value(context,
                closure_code_values + i, &tmp_value);
            if(i == last && return_value != NULL && G_IS_VALUE(value))
                gel_value_copy(value, return_value);
            if(G_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
        }
    }
    gel_context_free(context);
}


/**
 * gel_closure_new:
 * @context: instance of #GelContext
 * @args: #NULL terminated array of strings with the closure argument names.
 * @code: #GValueArray with the code of the closure.
 *
 * Creates a new #GClosure where @context is used as the marshal of the new
 * closure, @args contains the names of the arguments of the closure, and
 * @code contains the values that the closure shall evaluate when invoked.
 *
 * The closure takes ownership of @args and @code so they should not be freed.
 *
 * Returns: a new #GelClosure
 */
GClosure* gel_closure_new(GelContext *context, gchar **args, GValueArray *code)
{
    g_return_val_if_fail(context != NULL, NULL);
    g_return_val_if_fail(args != NULL, NULL);
    g_return_val_if_fail(code != NULL, NULL);

    GClosure *closure =
        g_closure_new_simple(sizeof(GelClosure), context);

    g_closure_set_marshal(closure, (GClosureMarshal)gel_closure_marshal);
    g_closure_add_finalize_notifier(
        closure, context, (GClosureNotify)gel_closure_finalize);

    GelClosure *gel_closure = (GelClosure*)closure;
    gel_closure->args = args;
    gel_closure->code = code;

    return closure;
}


/**
 * gel_closure_is_gel:
 * @closure: an instance of #GClosure 
 *
 * Checks whether the closure is written in gel or not.
 *
 * Returns: #TRUE if the closure is written in gel, #FALSE otherwise
 */
gboolean gel_closure_is_gel(GClosure *closure)
{
    g_return_val_if_fail(closure != NULL, FALSE);
    return closure->marshal == (GClosureMarshal)gel_closure_marshal;
}

