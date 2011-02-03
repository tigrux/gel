#include <string.h>

#include <gelcontext.h>
#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>
#include <gelvariable.h>


/**
 * SECTION:gelcontext
 * @short_description: Class used to keep symbols and evaluate values.
 * @title: GelContext
 * @include: gel.h
 *
 * #GelContext is a class where symbols are stored and evaluated.
 */


G_DEFINE_BOXED_TYPE(GelContext, gel_context, gel_context_dup, gel_context_free)


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
    register GelContext *self = g_slice_new0(GelContext);
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
    register GelContext *self = gel_context_new_with_outer(NULL);
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
    register GelContext *self;
    G_LOCK(contexts);
    if(contexts_POOL != NULL)
    {
        self = (GelContext*)contexts_POOL->data;
        contexts_POOL = g_list_delete_link(contexts_POOL, contexts_POOL);
    }
    else
        self = gel_context_alloc();
    self->outer = outer;
    self->running = TRUE;
    contexts_COUNT++;
    G_UNLOCK(contexts);

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

    register GelContext *context = gel_context_new_with_outer(self->outer);
    g_hash_table_iter_init(&iter, self->variables);
    while(g_hash_table_iter_next(&iter, (gpointer*)&name, (gpointer*)&variable))
        g_hash_table_insert(context->variables,
            g_strdup(name), gel_variable_ref(variable));

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
 * @dest_value: destination #GValue
 *
 * Evaluates @value, stores the result in @dest_value
 *
 * Returns: #TRUE if @dest_value was written, #FALSE otherwise.
 */
gboolean gel_context_eval(GelContext *self, 
                          const GValue *value, GValue *dest_value)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);
    g_return_val_if_fail(dest_value != NULL, FALSE);

    register const GValue *result_value =
        gel_context_eval_value(self, value, dest_value);
    if(GEL_IS_VALUE(result_value))
    {
        if(result_value != dest_value)
            gel_value_copy(result_value, dest_value);
        return TRUE;
    }
    return FALSE;
}


/**
 * gel_context_eval_value:
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
const GValue* gel_context_eval_value(GelContext *self,
                                     const GValue *value, GValue *out_value)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(out_value != NULL, NULL);

    register const GValue *result = NULL;
    register GType type = GEL_VALUE_TYPE(value);

    if(type == GEL_TYPE_SYMBOL)
    {
        register GelSymbol *symbol = (GelSymbol*)gel_value_get_boxed(value);
        result = gel_symbol_get_value(symbol);
        if(result == NULL)
        {
            register const gchar *symbol_name = gel_symbol_get_name(symbol);
            result = gel_context_lookup_symbol(self, symbol_name);
            if(result == NULL)
                gel_warning_unknown_symbol(__FUNCTION__, symbol_name);
        }
    }
    else
    if(type == G_TYPE_VALUE_ARRAY)
    {
        GValueArray *const array = (GValueArray*)gel_value_get_boxed(value);
        const guint array_n_values = array->n_values;
        if(array_n_values > 0)
        {
            const GValue *const array_values = array->values;

            GValue tmp_value = {0};
            const GValue *const first_value =
                gel_context_eval_value(self, array_values + 0, &tmp_value);

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


/**
 * gel_context_eval_params:
 * @self: #GelContext where to evaluate
 * @func: name of the invoker function, usually __FUNCTION__
 * @list: pointer to a list to keep temporary values
 * @format: string describing types to get
 * @n_values: pointer to the number of values in @values
 * @values: pointer to an array of values
 * @...: list of pointers to store the retrieved variables
 *
 * Convenience method to be used in implementation of new closures.
 *
 * Each character in @format indicates the type of variable you are parsing.
 * Posible formats are: asASIOCV
 *
 * <itemizedlist>
 *   <listitem><para>
 *     a: get a literal array (#GValueArray *).
 *   </para></listitem>
 *   <listitem><para>
 *     s: get a literal string (#gchararray).
 *   </para></listitem>
 *   <listitem><para>
 *     A: evaluate and get an array (#GValueArray *).
 *   </para></listitem>
 *   <listitem><para>
 *     S: evaluate and get a string (#gchararray).
 *   </para></listitem>
 *   <listitem><para>
 *     I: evaluate and get an integer (#glong).
 *   </para></listitem>
 *   <listitem><para>
 *     O: evaluate and get an object (#GObject *).
 *   </para></listitem>
 *   <listitem><para>
 *     C: evaluate and get a closure (#GClosure *).
 *   </para></listitem>
 *   <listitem><para>
 *     V: evaluate and get a value (#GValue *).
 *   </para></listitem>
 *   <listitem><para>
 *     *: Do not check for exact number or arguments.
 *   </para></listitem>
 * </itemizedlist>
 *
 * On success, @n_values will be decreased and @values repositioned
 * to match the first remaining argument,
 * @list must be disposed with #gel_value_list_free.
 *
 * On failure , @n_values and @values are left untouched.
 * 
 * Returns: #TRUE on sucess, #FALSE otherwise.
 */
gboolean gel_context_eval_params(GelContext *self, const gchar *func,
                                 GList **list, const gchar *format,
                                 guint *n_values, const GValue **values, ...)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(n_values != NULL, FALSE);
    g_return_val_if_fail(values != NULL, FALSE);

    register guint n_args = 0;
    register guint i;
    for(i = 0; format[i] != 0; i++)
        if(strchr("asASIOCV", format[i]) != NULL)
            n_args++;

    register gboolean exact = (strchr(format, '*') == NULL);

    if(exact && n_args != *n_values)
    {
        gel_warning_needs_n_arguments(func, n_args);
        return FALSE;
    }

    if(!exact && n_args > *n_values)
    {
        gel_warning_needs_at_least_n_arguments(func, n_args);
        return FALSE;
    }

    register gboolean parsed = TRUE;
    register gboolean pending = FALSE;
    va_list args;

    register GList *o_list = *list;
    register guint o_n_values = *n_values;
    register const GValue *o_values = *values;

    va_start(args, values);
    while(*format != 0 && *n_values >= 0 && parsed && !pending)
    {
        register GValue *value = NULL;
        register const GValue *result_value = NULL;

        switch(*format)
        {
            case 'a':
                if(GEL_VALUE_HOLDS(*values, G_TYPE_VALUE_ARRAY))
                {
                    GValueArray **a = va_arg(args, GValueArray **);
                    *a = (GValueArray*)gel_value_get_boxed(*values);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        *values, G_TYPE_VALUE_ARRAY);
                    parsed = FALSE;
                }
                break;
            case 's':
                if(GEL_VALUE_HOLDS(*values, GEL_TYPE_SYMBOL))
                {
                    const gchar **s = va_arg(args, const gchar **);
                    GelSymbol *symbol =
                        (GelSymbol*)gel_value_get_boxed(*values);
                    *s = gel_symbol_get_name(symbol);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        *values, GEL_TYPE_SYMBOL);
                    parsed = FALSE;
                }
                break;
            case 'O':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(GEL_VALUE_HOLDS(result_value, G_TYPE_OBJECT))
                {
                    GObject **obj = va_arg(args, GObject **);
                    *obj = (GObject*)g_value_get_object(result_value);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        result_value, G_TYPE_OBJECT);
                    parsed = FALSE;
                }
                break;
            case 'A':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(GEL_VALUE_HOLDS(result_value, G_TYPE_VALUE_ARRAY))
                {
                    GValueArray **a = va_arg(args, GValueArray **);
                    *a = (GValueArray*)gel_value_get_boxed(result_value);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        result_value, G_TYPE_VALUE_ARRAY);
                    parsed = FALSE;
                }
                break;
            case 'S':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(GEL_VALUE_HOLDS(result_value, G_TYPE_STRING))
                {
                    const gchar **s = va_arg(args, const gchar **);
                    *s = g_value_get_string(result_value);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        result_value, G_TYPE_STRING);
                    parsed = FALSE;
                }
                break;
            case 'I':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(GEL_VALUE_HOLDS(result_value, G_TYPE_LONG))
                {
                    glong *i = va_arg(args, glong *);
                    *i = gel_value_get_long(result_value);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        result_value, G_TYPE_LONG);
                    parsed = FALSE;
                }
                break;
            case 'C':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(GEL_VALUE_HOLDS(result_value, G_TYPE_CLOSURE))
                {
                    GClosure **closure = va_arg(args, GClosure **);
                    *closure = (GClosure*)gel_value_get_boxed(result_value);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        result_value, G_TYPE_CLOSURE);
                    parsed = FALSE;
                }
                break;
            case 'V':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(GEL_IS_VALUE(result_value))
                {
                    const GValue **v = va_arg(args, const GValue **);
                    *v = result_value;
                }
                else
                {
                    g_warning("%s: Not a GValue", func);
                    parsed = FALSE;
                }
                break;
            case '*':
                pending = TRUE;
                break;
        }

        if(value != NULL)
        {
            if(parsed && result_value == value)
                *list = g_list_append(*list, value);
            else
                gel_value_free(value);
        }

        if(parsed && !pending)
        {
            format++;
            (*values)++;
            (*n_values)--;
        }
    }
    va_end(args);

    if(!parsed)
    {
        gel_value_list_free(*list);
        *list = o_list;
        *n_values = o_n_values;
        *values = o_values;
    }
    return parsed;
}


/**
 * gel_context_lookup_symbol:
 * @self: #GelContext where to look for the symbol named @name
 * @name: name of the symbol to lookup
 *
 * If @context has a definition for @name, then returns its value.
 * If not, then it queries the outer context.
 * The returned value is owned by the context so it should not be freed.
 *
 * Returns: The value corresponding to @name, or #NULL if could not find it.
 */
GValue* gel_context_lookup_symbol(const GelContext *self, const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    register GelVariable *variable = NULL;
    register const GelContext *ic;
    for(ic = self; ic != NULL && variable == NULL; ic = ic->outer)
        variable = (GelVariable*)g_hash_table_lookup(ic->variables, name);

    register GValue *value;
    if(variable != NULL)
        value = gel_variable_get_value(variable);
    else
        value = NULL;
    return value;
}


/**
 * gel_context_insert_symbol:
 * @self: #GelContext where to insert the symbol
 * @name: name of the symbol to insert
 * @value: value of the symbol to insert
 *
 * Inserts a new symbol to @context with the name given by @name.
 * @self takes ownership of @value so it should not be freed or unset.
 */
void gel_context_insert_symbol(GelContext *self,
                               const gchar *name, GValue *value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(value != NULL);

    GelVariable *variable = gel_variable_new(value, TRUE);
    g_hash_table_insert(self->variables, g_strdup(name), variable);
}


/**
 * gel_context_insert_object:
 * @self: #GelContext where to insert the object
 * @name: name of the symbol to insert
 * @object: object to insert
 *
 * A wrapper for #gel_context_insert_symbol.
 * @self takes ownership of @object so it should not be unreffed.
 */
void gel_context_insert_object(GelContext *self, const gchar *name,
                               GObject *object)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(object != NULL);
    g_return_if_fail(G_IS_OBJECT(object));

    register GValue *value = gel_value_new_of_type(G_OBJECT_TYPE(object));
    g_value_take_object(value, object);
    gel_context_insert_symbol(self, name, value);
}


static
void gel_context_marshal(GClosure *closure, GValue *return_value,
                         guint n_param_values, const GValue *param_values,
                         gpointer invocation_hint, gpointer marshal_data)
{
    register GCClosure *cc = (GCClosure*)closure;
    register GFunc callback = (GFunc)cc->callback;

    if(n_param_values > 1)
        callback(g_value_peek_pointer(param_values + 0), closure->data);
    else
        callback(NULL, closure->data);
}


/**
 * gel_context_insert_function:
 * @self: #GelContext where to insert the function
 * @name: name of the symbol to insert
 * @function: a #GFunc to invoke
 * @user_data: extra data to pass to @function
 *
 * A wrapper for #gel_context_insert_symbol that calls @function when
 * @self evaluates a call to a function named @name.
 */
void gel_context_insert_function(GelContext *self, const gchar *name,
                                 GFunc callback, gpointer user_data)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(callback != NULL);

    register GValue *value = gel_value_new_of_type(G_TYPE_CLOSURE);
    register GClosure *closure =
        g_cclosure_new(G_CALLBACK(callback), user_data, NULL);
    g_closure_set_marshal(closure, (GClosureMarshal)gel_context_marshal);
    g_value_take_boxed(value, closure);
    gel_context_insert_symbol(self, name, value);
}


/**
 * gel_context_remove_symbol:
 * @self: #GelContext where to remove the symbol
 * @name: name of the symbol to remove
 *
 * Removes the symbol gived by @name.
 *
 * Returns: #TRUE is the symbol was removed, #FALSE otherwise.
 */
gboolean gel_context_remove_symbol(GelContext *self, const gchar *name)
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
 * It stops the current execution loop (for, while)
 * and is used internally by the function break.
 */
void gel_context_set_running(GelContext *self, gboolean running)
{
    g_return_if_fail(self != NULL);
    self->running = running;
}

