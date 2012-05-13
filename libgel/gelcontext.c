#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <gelerrors.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelvariable.h>
#include <gelclosure.h>

#include <gobject/gvaluecollector.h>

#ifndef GEL_CONTEXT_USE_POOL
#define GEL_CONTEXT_USE_POOL 1
#endif

/**
 * SECTION:gelcontext
 * @short_description: Class used to keep symbols and evaluate values.
 * @title: GelContext
 * @include: gel.h
 *
 * #GelContext is a class where symbols are stored and evaluated.
 */


struct _GelContext
{
    GHashTable *variables;
    GelContext *outer;
    GHashTable *inner;
    GError *error;
};


GQuark gel_context_error_quark(void)
{
    return g_quark_from_static_string("gel-context-error");
}


#if GEL_CONTEXT_USE_POOL
static guint contexts_COUNT;
static GList *contexts_POOL;
#endif


static GelContext *context_SOLITON;


static
GelContext* gel_context_alloc(void)
{
    GelContext *self = g_slice_new0(GelContext);
    self->inner = g_hash_table_new(g_direct_hash, g_direct_equal);
    self->variables = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        (GDestroyNotify)g_free, (GDestroyNotify)gel_variable_unref);

    return self;
}


static
void gel_context_dispose(GelContext *self)
{
    g_hash_table_unref(self->variables);
    g_hash_table_unref(self->inner);
    g_slice_free(GelContext, self);
}


/**
 * gel_context_new:
 *
 * Creates a #GelContext, with no outer context set.
 *
 * Returns: A new #GelContext, with no outer context assigned.
 */
GelContext* gel_context_new(void)
{
    if(context_SOLITON == NULL)
        context_SOLITON = gel_context_new_with_outer(NULL);

    return context_SOLITON;
}


/**
 * gel_context_new_with_outer:
 * @outer: The outer #GelContext
 *
 * Creates a #GelContext, using @outer as the outer context.
 * This method is used when invoking functions to have local variables.
 *
 * Returns: A new created #GelContext, with @outer as the outer context.
 */
GelContext* gel_context_new_with_outer(GelContext *outer)
{
    GelContext *self = NULL;
#if GEL_CONTEXT_USE_POOL
    if(contexts_POOL != NULL)
    {
        self = contexts_POOL->data;
        contexts_POOL = g_list_delete_link(contexts_POOL, contexts_POOL);
    }
    else
        self = gel_context_alloc();
    contexts_COUNT++;
#else
    self = gel_context_alloc();
#endif

    gel_context_set_outer(self, outer);

    return self;
}


GelContext* gel_context_validate(GelContext *context)
{
    /* Hack to work around closures invoked as signal callbacks */
    if(((GSignalInvocationHint*)context)->signal_id < 32767)
        context = context_SOLITON;
    return context;
}


/**
 * gel_context_dup:
 * @self: #GelContext to duplicate
 *
 * Constructs a duplicate of @self
 *
 * Returns: a #GelContext that is a duplicate of @self
 */
GelContext* gel_context_dup(const GelContext *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    const gchar *name;
    GelVariable *variable;
    GelContext *context = gel_context_new_with_outer(self->outer);

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, self->variables);

    while(g_hash_table_iter_next(&iter, (void **)&name, (void **)&variable))
        gel_context_define_variable(context, name, variable);

    return context;
}


/**
 * gel_context_free:
 * @self: #GelContext to free
 *
 * Releases resources of @self
 */
void gel_context_free(GelContext *self)
{
    g_return_if_fail(self != NULL);

    GList *inner_list = g_hash_table_get_keys(self->inner);
    for(GList *iter = inner_list; iter != NULL; iter = iter->next)
    {
        GelContext *inner = iter->data;
        gel_context_set_outer(inner, self->outer);
    }
    g_list_free(inner_list);

    if(self->error != NULL)
    {
        if(self->outer != NULL)
            gel_context_set_error(self->outer, self->error);
        else
            g_error_free(self->error);
        self->error = NULL;
    }

#if GEL_CONTEXT_USE_POOL
    g_hash_table_remove_all(self->variables);
    g_hash_table_remove_all(self->inner);

    contexts_POOL = g_list_append(contexts_POOL, self);
    if(--contexts_COUNT == 0)
    {
        g_list_foreach(contexts_POOL, (GFunc)gel_context_dispose, NULL);
        g_list_free(contexts_POOL);
    }
#else
    gel_context_dispose(self);
#endif
    if(self == context_SOLITON)
        context_SOLITON = NULL;
}


/**
 * gel_context_eval:
 * @self: #GelContext where to evaluate @value
 * @value: #GValue to evaluate
 * @dest: destination #GValue
 * @error: return location for a #GError, or NULL
 *
 * Evaluates @value, stores the result in @dest
 *
 * Returns: #TRUE if @dest was written, #FALSE otherwise.
 */
gboolean gel_context_eval(GelContext *self,
                          const GValue *value, GValue *dest, GError **error)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);
    g_return_val_if_fail(dest != NULL, FALSE);

    gboolean result = gel_context_eval_value(self, value, dest);

    if(self->error != NULL)
    {
        g_propagate_error(error, self->error);
        self->error = NULL;
        result = FALSE;
    }

    return result;
}


gboolean gel_context_eval_value(GelContext *self,
                                const GValue *value, GValue *dest)
{
    const GValue *result = gel_context_eval_into_value(self, value, dest);

    if(GEL_IS_VALUE(result))
    {
        if(result != dest)
        {
            if(GEL_IS_VALUE(dest))
                g_value_unset(dest);
            gel_value_copy(result, dest);
        }
        return TRUE;
    }

    return FALSE;
}


const GValue* gel_context_eval_param_into_value(GelContext *self,
                                                const GValue *value,
                                                GValue *out_value)
{
    const GValue *result_value =
        gel_context_eval_into_value(self, value, out_value);

    if(!gel_context_error(self))
        if(GEL_VALUE_HOLDS(result_value, GEL_TYPE_VARIABLE))
        {
            const GelVariable *variable = gel_value_get_boxed(result_value);
            if(variable != NULL)
            {
                const GValue *value = gel_variable_get_value(variable);
                if(value != NULL)
                    result_value = value;
            }
        }

    return result_value;
}


const GValue* gel_context_eval_into_value(GelContext *self,
                                          const GValue *value,
                                          GValue *out_value)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(out_value != NULL, NULL);

    const GValue *result = NULL;
    GType type = GEL_VALUE_TYPE(value);

    if(type == GEL_TYPE_SYMBOL)
    {
        const GelSymbol *symbol = gel_value_get_boxed(value);
        const gchar *name = gel_symbol_get_name(symbol);

        const GelVariable *variable = gel_context_get_variable(self, name);
        result = (variable != NULL) ?
            gel_variable_get_value(variable) : gel_symbol_get_value(symbol);

        if(result == NULL)
            result = gel_context_lookup(self, name);

        if(result == NULL)
            gel_error_unknown_symbol(self, __FUNCTION__, name);
    }
    else
    if(type == GEL_TYPE_ARRAY)
    {
        GelArray *array = gel_value_get_boxed(value);
        const guint array_n_values = gel_array_get_n_values(array);
        if(array_n_values > 0)
        {
            const GValue *array_values = gel_array_get_values(array);

            GValue tmp_value = {0};
            const GValue *first_value =
                gel_context_eval_into_value(self, array_values + 0, &tmp_value);


            if(!gel_context_error(self))
                if(GEL_VALUE_HOLDS(first_value, G_TYPE_CLOSURE))
                {
                    GClosure *closure = gel_value_get_boxed(first_value);
                    g_closure_invoke(closure,
                        out_value, array_n_values - 1 , array_values + 1, self);
                    result = out_value;
                }

            if(GEL_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
        }
    }

    if(result == NULL)
        result = value;

    return result;
}


/**
 * gel_context_error:
 * @self: #GelContext to check for errors
 *
 * Retrieves the error associated to @self
 *
 * Returns: #TRUE if an error has occurred, #FALSE otherwise
 */
gboolean gel_context_error(const GelContext* self)
{
    g_return_val_if_fail(self != NULL, FALSE);

    return self->error != NULL;
}


void gel_context_set_error(GelContext* self, GError *error)
{
    if(self->error != NULL)
        g_error_free(self->error);
    self->error = error;
}


void gel_context_transfer_error(GelContext *self, GelContext *context)
{
    g_warn_if_fail(self != context);

    if(self != context)
    {
        gel_context_set_error(context, self->error);
        self->error = NULL;
    }
}


/**
 * gel_context_clear_error:
 * @self: #GelContext to clear its error, if any
 *
 * Clears the error associated to the context, if any
 */
void gel_context_clear_error(GelContext* self)
{
    g_return_if_fail(self != NULL);

    g_clear_error(&self->error);
}


GelVariable* gel_context_get_variable(const GelContext *self,
                                      const gchar *name)
{
    return g_hash_table_lookup(self->variables, name);
}


GelVariable* gel_context_lookup_variable(const GelContext *self,
                                         const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    GelVariable *variable = NULL;
    const GelContext *context = self;

    while(context != NULL && variable == NULL)
    {
        variable = gel_context_get_variable(context, name);
        context = context->outer;
    }

    return variable;
}


/**
 * gel_context_lookup:
 * @self: #GelContext where to look for the symbol named @name
 * @name: name of the symbol to lookup
 *
 * If @context has a definition for @name, then returns its value.
 * If not, then it queries the outer context.
 * The returned value is owned by the context so it should not be freed.
 *
 * Returns: The value corresponding to @name, or #NULL if could not find it.
 */
GValue* gel_context_lookup(const GelContext *self, const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    GelVariable *variable = gel_context_lookup_variable(self, name);
    GValue *result = NULL;

    if(variable != NULL)
        result = gel_variable_get_value(variable);

    return result;
}


void gel_context_define_variable(GelContext *self,
                                 const gchar *name, GelVariable *variable)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(variable != NULL);

    g_hash_table_insert(self->variables,
        g_strdup(name), gel_variable_ref(variable));
}


/**
 * gel_context_define_value:
 * @self: #GelContext where to define the symbol
 * @name: name of the symbol to define
 * @value: value of the symbol to define
 *
 * defines a new symbol with the name given by @name.
 * @self takes ownership of @value so it should not be freed or unset.
 */
void gel_context_define_value(GelContext *self,
                              const gchar *name, GValue *value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(value != NULL);

    g_hash_table_insert(self->variables,
        g_strdup(name), gel_variable_new(value, TRUE));
}


/**
 * gel_context_define:
 * @self: #GelContext where to define the value
 * @name: name of the symbol to define
 * @type: the #GType of the value to define
 * @...: the value of the first object, as a literal
 *
 * A wrapper for #gel_context_define that calls @marshal when
 * @self evaluates a call to a function named @name.
 */
void gel_context_define(GelContext *self, const gchar *name, GType type, ...)
{
    va_list value_va;
    va_start(value_va, type);

    GValue *value = g_new0(GValue, 1);
    gchar *error = NULL;

    G_VALUE_COLLECT_INIT(value, type, value_va, 0, &error);

    if(error == NULL)
        gel_context_define_value(self, name, value);
    else
    {
        g_warning("Error defining '%s': %s", name, error);
        g_free(error);
        g_free(value);
    }

    va_end(value_va);
}


/**
 * gel_context_define_object:
 * @self: #GelContext where to define the object
 * @name: name of the symbol to define
 * @object: object to define
 *
 * A wrapper for #gel_context_define.
 * @self takes ownership of @object so it should not be freed or unset.
 */
void gel_context_define_object(GelContext *self, const gchar *name,
                               GObject *object)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(object != NULL);
    g_return_if_fail(G_IS_OBJECT(object));

    GValue *value = gel_value_new_of_type(G_OBJECT_TYPE(object));
    if(G_IS_INITIALLY_UNOWNED(object))
        g_object_ref_sink(object);

    gel_value_take_object(value, object);
    gel_context_define_value(self, name, value);
}


/**
 * gel_context_define_function:
 * @self: #GelContext where to define the function
 * @name: name of the symbol to define
 * @function: a #GelFunction to invoke
 * @user_data: extra data to pass to @function
 *
 * A wrapper for #gel_context_define that calls @marshal when
 * @self evaluates a call to a function named @name.
 */
void gel_context_define_function(GelContext *self, const gchar *name,
                                 GelFunction function, void *user_data)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(function != NULL);

    GValue *value = gel_value_new_of_type(G_TYPE_CLOSURE);
    GClosure *closure = gel_closure_new_native(
            g_strdup(name), (GClosureMarshal)function);
    closure->data = user_data;

    gel_value_take_boxed(value, closure);
    gel_context_define_value(self, name, value);
}


/**
 * gel_context_remove:
 * @self: #GelContext where to remove the value
 * @name: name of the value to remove
 *
 * Removes the value given by @name.
 *
 * Returns: #TRUE is the value was removed, #FALSE otherwise.
 */
gboolean gel_context_remove(GelContext *self, const gchar *name)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(name != NULL, FALSE);

    return g_hash_table_remove(self->variables, name);
}


/**
 * gel_context_get_outer:
 * @self: #GelContext to get its outer context
 *
 * Retrieves the outer context of @self
 *
 * Returns: the outer context, or #NULL if @self is the outermost context.
 */
GelContext* gel_context_get_outer(const GelContext* self)
{
    g_return_val_if_fail(self != NULL, NULL);

    return self->outer;
}


void gel_context_set_outer(GelContext *self, GelContext *context)
{
    g_return_if_fail(self != NULL);

    GelContext *old_outer = self->outer;
    if(old_outer != NULL)
        g_hash_table_remove(old_outer->inner, self);

    if(context != NULL)
        g_hash_table_insert(context->inner, self, self);

    self->outer = context;
}

