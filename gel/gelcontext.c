#include <string.h>

#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>


/**
 * SECTION:gelcontext
 * @short_description: Class used to keep symbols and evaluate values.
 * @title: GelContext
 * @include: gel.h
 *
 * #GelContext is a class where symbols are stored and evaluated.
 */


G_DEFINE_BOXED_TYPE(GelContext, gel_context, gel_context_copy, gel_context_free)


static guint contexts_COUNT;
static GList *contexts_POOL;
static GStaticMutex contexts_MUTEX;


static
GelContext *gel_context_alloc(void)
{
    register GelContext *self = g_slice_new0(GelContext);
    self->symbols = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        (GDestroyNotify)g_free, (GDestroyNotify)gel_value_free);
    return self;
}


static
void gel_context_dispose(GelContext *self)
{
    g_hash_table_unref(self->symbols);
    g_slice_free(GelContext, self);
}


/**
 * gel_context_new:
 *
 * Creates a #GelContext, no outer context is assigned and the
 * default symbols are added with #gel_context_add_default_symbols.
 *
 * Returns: A new #GelContext, with no outer context assigned.
 */
GelContext* gel_context_new(void)
{
    register GelContext *self = gel_context_new_with_outer(NULL);
    gel_context_add_default_symbols(self);
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
    g_static_mutex_lock(&contexts_MUTEX);
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
    g_static_mutex_unlock(&contexts_MUTEX);

    return self;
}


/**
 * gel_context_copy:
 * @self: #GelContext to copy
 *
 * Constructs a copy of @self
 *
 * Returns: a #GelContext that is a copy of @self
 */
GelContext* gel_context_copy(GelContext *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    GHashTableIter iter;
    const gchar *name;
    GValue *value;

    register GelContext *context = gel_context_new_with_outer(self->outer);
    g_hash_table_iter_init(&iter, self->symbols);
    while(g_hash_table_iter_next(&iter, (gpointer*)&name, (gpointer*)&value))
        gel_context_add_value(context, name, gel_value_dup(value));

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
    g_hash_table_remove_all(self->symbols);

    g_static_mutex_lock(&contexts_MUTEX);
    contexts_POOL = g_list_append(contexts_POOL, self);
    if(--contexts_COUNT == 0)
    {
        g_list_foreach(contexts_POOL, (GFunc)gel_context_dispose, NULL);
        g_list_free(contexts_POOL);
    }
    g_static_mutex_unlock(&contexts_MUTEX);
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
        const gchar *symbol_name = symbol->name;
        if((result = gel_context_find_symbol(self, symbol_name)) != NULL)
            symbol->value = result;
        else
            gel_warning_unknown_symbol(__FUNCTION__, symbol_name);
    }
    else
    if(type == G_TYPE_VALUE_ARRAY)
    {
        const GValueArray *const array =
            (GValueArray*)gel_value_get_boxed(value);
        const guint array_n_values = array->n_values;
        const GValue *const array_values = array->values;

        GValue tmp_value = {0};
        register const GValue *first_value =
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
                    GelSymbol *symbol = (GelSymbol*)gel_value_get_boxed(*values);
                    *s = symbol->name;
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
 * gel_context_find_symbol:
 * @self: #GelContext where to look for the symbol named @name
 * @name: name of the symbol to lookup
 *
 * If @context has a definition for @name, then returns its value.
 * If not, then it queries the outer context.
 * The returned value is owned by the context so it should not be freed.
 *
 * Returns: The value corresponding to @name, or #NULL if could not find it.
 */
GValue* gel_context_find_symbol(const GelContext *self, const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    register GValue *symbol = NULL;
    register const GelContext *ic;
    for(ic = self; ic != NULL && symbol == NULL; ic = ic->outer)
        symbol = (GValue*)g_hash_table_lookup(ic->symbols, name);
    return symbol;
}


/**
 * gel_context_add_value:
 * @self: #GelContext where to add the symbol
 * @name: name of the symbol to add
 * @value: value of the symbol to add
 *
 * Adds a new symbol to @context with the name given by @name.
 * @self takes ownership of @value so it should not be freed or unset.
 */
void gel_context_add_value(GelContext *self, const gchar *name, GValue *value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(value != NULL);
    g_hash_table_insert(self->symbols, g_strdup(name), value);
}


/**
 * gel_context_add_object:
 * @self: #GelContext where to add the object
 * @name: name of the symbol to add
 * @object: object to add
 *
 * A wrapper for #gel_context_add_value.
 * @self takes ownership of @object so it should not be unreffed.
 */
void gel_context_add_object(GelContext *self, const gchar *name,
                            GObject *object)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(object != NULL);
    g_return_if_fail(G_IS_OBJECT(object));

    register GValue *value = gel_value_new_of_type(G_OBJECT_TYPE(object));
    g_value_take_object(value, object);
    gel_context_add_value(self, name, value);
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
 * gel_context_add_function:
 * @self: #GelContext where to add the function
 * @name: name of the symbol to add
 * @function: a #GFunc to invoke
 * @user_data: extra data to pass to @function
 *
 * A wrapper for #gel_context_add_value that calls @function when
 * @self evaluates a call to a function named @name.
 */
void gel_context_add_function(GelContext *self, const gchar *name,
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
    gel_context_add_value(self, name, value);
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
    return g_hash_table_remove(self->symbols, name);
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

