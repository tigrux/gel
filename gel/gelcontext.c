#include <string.h>

#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelvariable.h>
#include <gelclosure.h>


/**
 * SECTION:gelcontext
 * @short_description: Class used to keep symbols and evaluate values.
 * @title: GelContext
 * @include: gel.h
 *
 * #GelContext is a class where symbols are stored and evaluated.
 */


GType gel_context_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;
    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelContext",
                (GBoxedCopyFunc)gel_context_dup,
                (GBoxedFreeFunc)gel_context_free);
        g_once_init_leave(&once, 1);
    }
    return type;
}


struct _GelContext
{
    GHashTable *variables;
    GelContext *outer;
    gboolean running;
};


static guint contexts_COUNT;
static GList *contexts_POOL;
G_LOCK_DEFINE_STATIC(contexts);


static
GelContext *gel_context_alloc(void)
{
    GelContext *self = g_slice_new0(GelContext);
    self->variables = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        (GDestroyNotify)g_free, (GDestroyNotify)gel_variable_unref);
    return self;
}


static
void gel_context_dispose(GelContext *self)
{
    g_hash_table_unref(self->variables);
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
    GelContext *self = gel_context_new_with_outer(NULL);
    return self;
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
    GelContext *self;
    G_LOCK(contexts);
    if(contexts_POOL != NULL)
    {
        self = (GelContext*)contexts_POOL->data;
        contexts_POOL = g_list_delete_link(contexts_POOL, contexts_POOL);
    }
    else
        self = gel_context_alloc();
    contexts_COUNT++;
    G_UNLOCK(contexts);

    self->outer = outer;
    self->running = TRUE;

    return self;
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

    GHashTableIter iter;
    const gchar *name;
    GelVariable *variable;

    GelContext *context = gel_context_new_with_outer(self->outer);
    g_hash_table_iter_init(&iter, self->variables);
    while(g_hash_table_iter_next(&iter, (gpointer*)&name, (gpointer*)&variable))
        gel_context_insert_variable(context, name, variable);

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

    g_hash_table_remove_all(self->variables);

    G_LOCK(contexts);
    contexts_POOL = g_list_append(contexts_POOL, self);
    if(--contexts_COUNT == 0)
    {
        g_list_foreach(contexts_POOL, (GFunc)gel_context_dispose, NULL);
        g_list_free(contexts_POOL);
    }
    G_UNLOCK(contexts);
}


/**
 * gel_context_eval:
 * @self: #GelContext where to evaluate @value
 * @value: #GValue to evaluate
 * @dest: destination #GValue
 *
 * Evaluates @value, stores the result in @dest
 *
 * Returns: #TRUE if @dest was written, #FALSE otherwise.
 */
gboolean gel_context_eval(GelContext *self, const GValue *value, GValue *dest)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);
    g_return_val_if_fail(dest != NULL, FALSE);

    const GValue *result = gel_context_eval_into_value(self, value, dest);
    if(GEL_IS_VALUE(result))
    {
        if(result != dest)
            gel_value_copy(result, dest);
        return TRUE;
    }
    return FALSE;
}


/**
 * gel_context_eval_into_value:
 * @self: #GelContext where to evaluate @value
 * @value: value to evaluate
 * @out_value: value where a result may be written.
 *
 * Evaluates the given value.
 * If @value matches a symbol, returns the corresponding value.
 * If @value is a literal (number or string), returns @value itself.
 * If not, then @value is evaluated and the result stored in @out_value.
 *
 * Returns: A #GValue with the result
 */
const GValue* gel_context_eval_into_value(GelContext *self,
                                     const GValue *value, GValue *out_value)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(out_value != NULL, NULL);

    const GValue *result = NULL;
    GType type = GEL_VALUE_TYPE(value);

    if(type == GEL_TYPE_SYMBOL)
    {
        const GelSymbol *symbol = (GelSymbol*)gel_value_get_boxed(value);
        const gchar *symbol_name = gel_symbol_get_name(symbol);
        if(gel_context_has_variable(self, symbol_name))
            result = gel_context_lookup(self, symbol_name);
        else
            result = gel_symbol_get_value(symbol);

        if(result == NULL)
        {
            GelContext *outer = gel_context_get_outer(self);
            if(outer != NULL)
                result = gel_context_lookup(self, symbol_name);
        }

        if(result == NULL)
            gel_warning_unknown_symbol(__FUNCTION__, symbol_name);
    }
    else
    if(type == G_TYPE_VALUE_ARRAY)
    {
        GValueArray *array = (GValueArray*)gel_value_get_boxed(value);
        const guint array_n_values = array->n_values;
        if(array_n_values > 0)
        {
            const GValue *array_values = array->values;

            GValue tmp_value = {0};
            const GValue *first_value =
                gel_context_eval_into_value(self, array_values + 0, &tmp_value);

            if(GEL_VALUE_HOLDS(first_value, G_TYPE_CLOSURE))
            {
                g_closure_invoke((GClosure*)gel_value_get_boxed(first_value),
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


GList* gel_context_get_variables(const GelContext *self)
{
    return g_hash_table_get_keys(self->variables);
}


gboolean gel_context_has_variable(const GelContext *self, const gchar *name)
{
    return g_hash_table_lookup(self->variables, name) != NULL;
}


GelVariable* gel_context_lookup_variable(const GelContext *self,
                                         const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    GelVariable *variable = NULL;
    const GelContext *c;
    for(c = self; c != NULL && variable == NULL; c = c->outer)
        variable = (GelVariable*)g_hash_table_lookup(c->variables, name);
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
    GValue *result;
    if(variable != NULL)
        result = gel_variable_get_value(variable);
    else
        result = NULL;
    return result;
}


void gel_context_insert_variable(GelContext *self,
                                 const gchar *name, GelVariable *variable)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(variable != NULL);

    g_hash_table_insert(self->variables,
        g_strdup(name), gel_variable_ref(variable));
}


/**
 * gel_context_insert:
 * @self: #GelContext where to insert the symbol
 * @name: name of the symbol to insert
 * @value: value of the symbol to insert
 *
 * Inserts a new symbol to @self with the name given by @name.
 * @self takes ownership of @value so it should not be freed or unset.
 */
void gel_context_insert(GelContext *self,
                        const gchar *name, GValue *value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(value != NULL);

    g_hash_table_insert(self->variables,
        g_strdup(name), gel_variable_new(value, TRUE));
}


/**
 * gel_context_insert_object:
 * @self: #GelContext where to insert the object
 * @name: name of the symbol to insert
 * @object: object to insert
 *
 * A wrapper for #gel_context_insert.
 * @self takes ownership of @object so it should not be freed or unset.
 */
void gel_context_insert_object(GelContext *self, const gchar *name,
                               GObject *object)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(object != NULL);
    g_return_if_fail(G_IS_OBJECT(object));

    GValue *value = gel_value_new_of_type(G_OBJECT_TYPE(object));
    if(G_IS_INITIALLY_UNOWNED(object))
        g_object_ref_sink(object);
    g_value_take_object(value, object);
    gel_context_insert(self, name, value);
}


/**
 * gel_context_insert_function:
 * @self: #GelContext where to insert the function
 * @name: name of the symbol to insert
 * @function: a #GelFunction to invoke
 * @user_data: extra data to pass to @function
 *
 * A wrapper for #gel_context_insert that calls @marshal when
 * @self evaluates a call to a function named @name.
 */
void gel_context_insert_function(GelContext *self, const gchar *name,
                                 GelFunction function, gpointer user_data)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(function != NULL);

    GValue *value = gel_value_new_of_type(G_TYPE_CLOSURE);
    GClosure *closure = gel_closure_new_native(
            g_strdup(name), (GClosureMarshal)function);
    closure->data = user_data;
    g_value_take_boxed(value, closure);
    gel_context_insert(self, name, value);
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
 * Retrieves @self's outer context
 *
 * Returns: the outer context, or #NULL if @self is the outermost context.
 */
GelContext* gel_context_get_outer(const GelContext* self)
{
    g_return_val_if_fail(self != NULL, NULL);
    return self->outer;
}


/**
 * gel_context_get_running:
 * @self: #GelContext to check if it is running or not
 *
 * Checks if @self is running.
 * A #GelContext is considered as running when it is
 * currently evaluating a loop (for, while).
 *
 * Returns: #TRUE if #self is running, #FALSE otherwise
 */
gboolean gel_context_get_running(const GelContext *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    return self->running;
}


/**
 * gel_context_set_running:
 * @self: #GelContext whose running state will be set
 * @running: #TRUE to set the context as running
 *
 * Set @self's running state as @running
 * If @running is #FALSE then current execution loop (for, while) is stopped.
 * It is used internally by the function break.
 */
void gel_context_set_running(GelContext *self, gboolean running)
{
    g_return_if_fail(self != NULL);
    self->running = running;
}

