#include <gelclosure.h>
#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>


/**
 * SECTION:gelclosure
 * @short_description: Functions to create closures to be used by gel.
 * @title: GelClosure
 * @include: gel.h
 *
 * Gel provides two type of closures: native and written in gel.
 * All the predefined closures are native functions written in C.
 *
 * Closures written in gel behave as normal closures and can be
 * called via a #g_closure_invoke.
 *
 * Closures predefined or written in gel provide a name that can be queried
 * in runtime to make them debug friendly.
 */


struct _GelClosure
{
    GClosure closure;
    gchar *name;
    gchar **args;
    GHashTable *args_hash;
    GValueArray *code;
};


typedef struct _GelNativeClosure GelNativeClosure;

struct _GelNativeClosure
{
    GClosure closure;
    gchar *name;
    GClosureMarshal native_marshal;
};


static
void gel_closure_marshal(GelClosure *closure, GValue *return_value,
                         guint n_values, const GValue *values,
                         GelContext *invocation_context)
{
    const guint n_args = g_strv_length(closure->args);
    g_return_if_fail(n_values == n_args);

    GelContext *outer = gel_closure_get_context(closure);
    GelContext *context = gel_context_new_with_outer(outer);

    gchar **closure_args = closure->args;
    for(guint i = 0; i < n_args; i++)
    {
        GValue *value = gel_value_new();
        gel_context_eval(invocation_context, values + i, value);
        gel_context_insert(context, closure_args[i], value);
    }

    guint closure_code_n_values = closure->code->n_values;
    if(closure_code_n_values > 0)
    {
        guint last = closure_code_n_values - 1;
        const GValue *closure_code_values = closure->code->values;
        for(guint i = 0; i <= last; i++)
        {
            GValue tmp_value = {0};
            const GValue *value = gel_context_eval_into_value(context,
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

    g_free(closure->name);
    g_strfreev(closure->args);
    g_hash_table_unref(closure->args_hash);
    g_value_array_free(closure->code);
    gel_context_free(self);
}


GelContext* gel_closure_get_context(GelClosure *self)
{
    return (GelContext*)self->closure.data;
}


static
gboolean gel_closure_has_argument(const GelClosure *self, const gchar *name)
{
    return g_hash_table_lookup(self->args_hash, name) != NULL;
}


static
void gel_closure_bind_symbols_of_array(GelClosure *self, GValueArray *array)
{
    guint array_n_values = array->n_values;
    GValue *array_values = array->values;

    for(guint i = 0; i < array_n_values; i++)
    {
        const GValue *value = array_values + i;
        GType type = GEL_VALUE_TYPE(value);
        if(type == G_TYPE_VALUE_ARRAY)
            gel_closure_bind_symbols_of_array(self,
                (GValueArray*)gel_value_get_boxed(value));
        else
        if(type == GEL_TYPE_SYMBOL)
        {
            GelSymbol *symbol = (GelSymbol*)gel_value_get_boxed(value);
            const gchar *name = gel_symbol_get_name(symbol);
            const GelContext *context = gel_closure_get_context(self);
            if(!gel_closure_has_argument(self, name))
            {
                GelVariable *variable =
                    gel_context_lookup_variable(context, name);
                if(variable != NULL)
                    gel_symbol_set_variable(symbol, variable);
            }
        }
    }
}


void gel_closure_close_over(GClosure *closure)
{
    GelClosure *self = (GelClosure *)closure;
    gel_closure_bind_symbols_of_array(self, self->code);
}


/**
 * gel_closure_new:
 * @name: name of the closure.
 * @args: #NULL terminated array of strings with the closure argument names.
 * @code: #GValueArray with the code of the closure.
 * @context: #GelContext where to define a #GClosure.
 *
 * Creates a new #GClosure where @context is used as the data of the new
 * closure, @name will be the name of the closure,
 * @args contains the names of the arguments of the closure, and
 * @code contains the values that the closure shall evaluate when invoked.
 *
 * The closure takes ownership of @args and @code so they should not be freed.
 *
 * Returns: a new #GClosure
 */
GClosure* gel_closure_new(const gchar *name, gchar **args, GValueArray *code,
                          GelContext *context)
{
    g_return_val_if_fail(context != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);
    g_return_val_if_fail(args != NULL, NULL);
    g_return_val_if_fail(code != NULL, NULL);

    GelContext *new_context = gel_context_dup(context);
    gel_context_set_outer(new_context, context);
    GClosure *closure = g_closure_new_simple(sizeof(GelClosure), new_context);
    g_closure_set_marshal(closure, (GClosureMarshal)gel_closure_marshal);
    g_closure_add_finalize_notifier(closure, 
        new_context, (GClosureNotify)gel_closure_finalize);

    GHashTable *args_hash = g_hash_table_new(g_str_hash, g_str_equal);
    for(guint i = 0; args[i] != NULL; i++)
        g_hash_table_insert(args_hash, args[i], args);

    GelClosure *self = (GelClosure*)closure;
    self->args_hash = args_hash;
    self->name = g_strdup(name);
    self->args = args;
    self->code = code;

    return closure;
}


static
void gel_native_closure_marshal(GClosure *closure, GValue *return_value,
                                guint n_values, const GValue *values,
                                GelContext *context)
{
    /* Hack to work around closures invoked as signal callbacks */
    if(((GSignalInvocationHint*)context)->signal_id < 32767)
        context = NULL;
    ((GelNativeClosure *)closure)->native_marshal(
        closure, return_value, n_values, values, context, closure->data);
}


/**
 * gel_closure_new_native:
 * @name: name of the closure
 * @marshal: a #GClosureMarshal to call when the closure is invoked.
 *
 * Creates a new #GClosure named @name that will call @marshal when invoked.
 * The sole purpose of this function is to create predefined closures
 * that are assigned a name to retrieve with #gel_closure_get_name.
 *
 * Returns: a new gel native #GClosure
 */
GClosure* gel_closure_new_native(const gchar *name, GClosureMarshal marshal)
{
    g_return_val_if_fail(name != NULL, NULL);
    g_return_val_if_fail(marshal != NULL, NULL);

    GClosure *closure = g_closure_new_simple(sizeof (GelNativeClosure), NULL);
    GelNativeClosure *self = (GelNativeClosure*)closure;
    gchar *dup_name = g_strdup(name);
    self->name = dup_name;
    self->native_marshal = marshal;
    g_closure_set_marshal(closure, (GClosureMarshal)gel_native_closure_marshal);
    g_closure_add_finalize_notifier(closure, dup_name, (GClosureNotify)g_free);

    return closure;
}


/**
 * gel_closure_get_name:
 * @closure: a #GClosure whose name will be retrieved
 *
 * Gets the name for closures predefined or written in gel.
 *
 * Returns: The name of the gel closure, or #NULL if it is not predefined nor written in gel.
 */
const gchar* gel_closure_get_name(const GClosure *closure)
{
    const gchar *name;
    if(closure->marshal == (GClosureMarshal)gel_native_closure_marshal)
        name = ((GelNativeClosure*)closure)->name;
    else
    if(closure->marshal == (GClosureMarshal)gel_closure_marshal)
        name = ((GelClosure*)closure)->name;
    else
        name = NULL;

    return name;
}

