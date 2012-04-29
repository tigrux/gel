#include <gelclosure.h>
#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <geltypeinfo.h>

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
    GelContext *context;
    gchar *name;
    GList *args;
    gchar *variadic;
    GHashTable *args_hash;
    GValueArray *code;
};


static
void gel_closure_marshal(GelClosure *closure, GValue *return_value,
                         guint n_values, const GValue *values,
                         GelContext *invocation_context)
{
    const guint n_args = g_list_length(closure->args);
    gboolean is_variadic = (closure->variadic != NULL);

    if(is_variadic)
    {
        if(n_values < n_args)
        {
            g_warning("Function '%s' takes at least %u arguments, got %u",
                closure->name, n_args, n_values);
            return;
        }
    }
    else
    {
        if(n_values != n_args)
        {
            g_warning("Function '%s' takes %u arguments, got %u",
                closure->name, n_args, n_values);
            return;
        }
    }

    GelContext *outer = gel_closure_get_context(closure);
    GelContext *context = gel_context_new_with_outer(outer);

    GList *iter = closure->args;
    guint i = 0;
    while(iter != NULL)
    {
        GValue *value = gel_value_new();
        gel_context_eval(invocation_context, values + i, value);
        const gchar *arg = iter->data;
        gel_context_insert(context, arg, value);
        iter = iter->next;
        i++;
    }

    if(is_variadic)
    {
        guint array_n_values = n_values - i;
        GValueArray *array = g_value_array_new(array_n_values);
        array->n_values = array_n_values;
        GValue *array_values = array->values;

        guint j = 0;
        while(i < n_values)
        {
            gel_context_eval(invocation_context, values + i, array_values + j);
            i++;
            j++;
        }

        GValue *value = gel_value_new_from_boxed(G_TYPE_VALUE_ARRAY, array);
        gel_context_insert(context, closure->variadic, value);
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
void gel_closure_finalize(void *data, GelClosure *self)
{
    g_free(self->name);
    g_list_foreach(self->args, (GFunc)g_free, NULL);
    g_list_free(self->args);
    if(self->variadic != NULL)
        g_free(self->variadic);
    g_hash_table_unref(self->args_hash);
    g_value_array_free(self->code);
    gel_context_free(self->context);
}


GelContext* gel_closure_get_context(GelClosure *self)
{
    return self->context;
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
        {
            GValueArray *array = gel_value_get_boxed(value);
            gel_closure_bind_symbols_of_array(self, array);
        }
        else
        if(type == GEL_TYPE_SYMBOL)
        {
            GelSymbol *symbol = gel_value_get_boxed(value);
            const gchar *name = gel_symbol_get_name(symbol);
            const GelContext *context = gel_closure_get_context(self);

            if(g_hash_table_lookup(self->args_hash, name) == NULL)
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
 * Creates a new #GClosure written in Gel
 * @name will be the name of the closure,
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

    GClosure *closure = g_closure_new_simple(sizeof(GelClosure), NULL);
    g_closure_set_marshal(closure, (GClosureMarshal)gel_closure_marshal);
    g_closure_add_finalize_notifier(closure,
        NULL, (GClosureNotify)gel_closure_finalize);

    GelClosure *self = (GelClosure*)closure;
    GHashTable *args_hash = g_hash_table_new(g_str_hash, g_str_equal);
    GList *list_args = NULL;
    gboolean next_is_variadic = FALSE;

    for(guint i = 0; args[i] != NULL; i++)
    {
        gchar *arg = args[i];
        if(self->variadic == NULL)
        {
            if(g_strcmp0(arg, "&") == 0)
            {
                next_is_variadic = TRUE;
                arg = NULL;
            }
            else
            if(next_is_variadic)
            {
                self->variadic = g_strdup(arg);
                g_hash_table_insert(args_hash, arg, arg);
                arg = NULL;
            }
        }

        if(arg != NULL)
        {
            list_args = g_list_append(list_args, g_strdup(arg));
            g_hash_table_insert(args_hash, arg, arg);
        }
    }

    GelContext *closure_context = gel_context_dup(context);
    gel_context_set_outer(closure_context, context);

    self->args_hash = args_hash;
    self->name = g_strdup(name);
    self->args = list_args;
    self->code = code;
    self->context = closure_context;

    return closure;
}


typedef struct _GelNativeClosure GelNativeClosure;

struct _GelNativeClosure
{
    GClosure closure;
    gchar *name;
    GClosureMarshal native_marshal;
};


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


struct _GelIntrospectionClosure
{
    GClosure closure;
    gchar *name;
    const GelTypeInfo *info;
    GObject *object;
};


static
void gel_introspection_closure_finalize(void *data,
                                        GelIntrospectionClosure *self)
{
    if(self->object != NULL)
        g_object_unref(self->object);
    g_free(self->name);
}


GClosure* gel_closure_new_introspection(const GelTypeInfo *info,
                                        GObject *object)
{
    GClosure *closure =
        g_closure_new_simple(sizeof (GelIntrospectionClosure), NULL);

    g_closure_set_marshal(closure,
        (GClosureMarshal)gel_type_info_closure_marshal);

    g_closure_add_finalize_notifier(closure, object,
        (GClosureNotify)gel_introspection_closure_finalize);

    GelIntrospectionClosure *self = (GelIntrospectionClosure*)closure;
    self->name = gel_type_info_to_string(info);
    self->info = info;
    if(object != NULL)
        self->object = g_object_ref(object);
    else
        self->object = NULL;
    return closure;
}


const GelTypeInfo* gel_introspection_closure_get_info(GelIntrospectionClosure *self)
{
    return self->info;
}


GObject* gel_introspection_closure_get_object(GelIntrospectionClosure *self)
{
    return self->object;
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
    if(closure->marshal == (GClosureMarshal)gel_closure_marshal)
        name = ((GelClosure*)closure)->name;
    else
    if(closure->marshal == (GClosureMarshal)gel_native_closure_marshal)
        name = ((GelNativeClosure*)closure)->name;
    else
    if(closure->marshal == (GClosureMarshal)gel_type_info_closure_marshal)
        name = ((GelIntrospectionClosure*)closure)->name;
    else
        name = NULL;

    return name;
}

