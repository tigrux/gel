#include <gelcontext.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>


typedef struct _GelClosure GelClosure;

struct _GelClosure
{
    GClosure closure;

    gchar **args;
    GValueArray *code;
};


static
void gel_closure_marshal(GelClosure *closure, GValue *return_value,
                         guint n_values, const GValue *values,
                         GelContext *invocation_context)
{
    const guint n_args = g_strv_length(closure->args);
    g_return_if_fail(n_values == n_args);

    GelContext *outer = (GelContext*)closure->closure.data;
    GelContext *context = gel_context_new_with_outer(outer);

    gchar **closure_args = closure->args;
    guint i;
    for(i = 0; i < n_args; i++)
    {
        GValue *value = gel_value_new();
        gel_context_eval(invocation_context, values + i, value);
        gel_context_insert_symbol(context, closure_args[i], value);
    }

    guint closure_code_n_values = closure->code->n_values;
    if(closure_code_n_values > 0)
    {
        guint last = closure_code_n_values - 1;
        const GValue *closure_code_values = closure->code->values;
        for(i = 0; i <= last; i++)
        {
            GValue tmp_value = {0};
            const GValue *value = gel_context_eval_value(context,
                closure_code_values + i, &tmp_value);
            if(i == last && return_value != NULL && GEL_IS_VALUE(value))
                gel_value_copy(value, return_value);
            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
        }
    }

    gel_context_free(context);
}


static
void gel_closure_finalize(GelContext *self, GelClosure *closure)
{
    g_return_if_fail(self != NULL);

    g_strfreev(closure->args);
    g_value_array_free(closure->code);
}


/**
 * gel_context_closure_new:
 * @self: #GelContext where to define a #GClosure
 * @args: #NULL terminated array of strings with the closure argument names.
 * @code: #GValueArray with the code of the closure.
 *
 * Creates a new #GClosure where @self is used as the data of the new
 * closure, @args contains the names of the arguments of the closure, and
 * @code contains the values that the closure shall evaluate when invoked.
 *
 * The closure takes ownership of @args and @code so they should not be freed.
 *
 * Returns: a new #GClosure
 */
GClosure* gel_context_closure_new(GelContext *self,
                                  gchar **args, GValueArray *code)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(args != NULL, NULL);
    g_return_val_if_fail(code != NULL, NULL);

    GClosure *closure = g_closure_new_simple(sizeof(GelClosure), self);

    g_closure_set_marshal(closure, (GClosureMarshal)gel_closure_marshal);
    g_closure_add_finalize_notifier(closure,
        self, (GClosureNotify)gel_closure_finalize);

    GelClosure *gel_closure = (GelClosure*)closure;
    gel_closure->args = args;
    gel_closure->code = code;

    return closure;
}

