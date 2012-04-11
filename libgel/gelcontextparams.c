#include <string.h>

#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>

#define g_node_get_data(node) (GPOINTER_TO_INT((node)->data))


static
GNode *gel_params_format_to_node(const gchar *format, guint *pos)
{
    GNode *root = g_node_new(NULL);
    while(format[*pos] != 0)
    {
        if(format[*pos] == '(')
        {
            (*pos)++;
            GNode *child = gel_params_format_to_node(format, pos);
            g_node_append(root, child);
        }
        else
        if(format[*pos] == ')')
            break;
        else
            g_node_append_data(root, GINT_TO_POINTER((gint)format[*pos]));
        (*pos)++;
    }
    return root;
}


static
gboolean gel_context_eval_param(GelContext *self, const gchar *func,
                                GList **list, gchar format,
                                guint *n_values, const GValue **values,
                                va_list args)
{
    GValue *value = NULL;
    const GValue *result_value = NULL;
    gboolean parsed = TRUE;

    switch(format)
    {
        case 'v':
            {
                const GValue **v = va_arg(args, const GValue **);
                *v = *values;
            }
            break;
        case 'V':
            value = gel_value_new();
            result_value =
                gel_context_eval_param_into_value(self, *values, value);
            if(!G_VALUE_HOLDS(result_value, GEL_TYPE_SYMBOL))
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
        case 'A':
            value = gel_value_new();
            result_value =
                gel_context_eval_param_into_value(self, *values, value);
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
        case 'H':
            value = gel_value_new();
            result_value =
                gel_context_eval_param_into_value(self, *values, value);
            if(GEL_VALUE_HOLDS(result_value, G_TYPE_HASH_TABLE))
            {
                GHashTable **a = va_arg(args, GHashTable **);
                *a = (GHashTable*)gel_value_get_boxed(result_value);
            }
            else
            {
                gel_warning_value_not_of_type(func,
                    result_value, G_TYPE_HASH_TABLE);
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
        case 'S':
            value = gel_value_new();
            result_value =
                gel_context_eval_param_into_value(self, *values, value);
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
            result_value =
                gel_context_eval_param_into_value(self, *values, value);
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
        case 'O':
            value = gel_value_new();
            result_value =
                gel_context_eval_param_into_value(self, *values, value);
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
        case 'C':
            value = gel_value_new();
            result_value =
                gel_context_eval_param_into_value(self, *values, value);
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
    }

    if(value != NULL)
    {
        if(parsed && result_value == value)
            *list = g_list_append(*list, value);
        else
            gel_value_free(value);
    }
    return parsed;
}


static
gboolean gel_context_eval_params_va(GelContext *self, const gchar *func,
                                    GList **list, GNode *format_node,
                                    guint *n_values, const GValue **values,
                                    va_list args)

{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(n_values != NULL, FALSE);
    g_return_val_if_fail(values != NULL, FALSE);

    gboolean exact = TRUE;

    GNode *last_child = g_node_last_child(format_node);
    if(g_node_get_data(last_child) == '*')
    {
        exact = FALSE;
        g_node_destroy(last_child);
    }

    guint n_args = g_node_n_children(format_node);

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

    gboolean parsed = TRUE;

    GList *o_list = *list;
    guint o_n_values = *n_values;
    const GValue *o_values = *values;

    GNode *node = g_node_first_child(format_node);
    while(node != 0 && *n_values >= 0 && parsed)
    {
        if(G_NODE_IS_LEAF(node))
        {
            gchar format = g_node_get_data(node);
            parsed = gel_context_eval_param(self,
                func, list, format, n_values, values, args);
        }
        else
            if(GEL_VALUE_HOLDS(*values, G_TYPE_VALUE_ARRAY))
            {
                GValueArray *array =
                    (GValueArray*)gel_value_get_boxed(*values);
                guint n_values = array->n_values;
                const GValue *values = array->values;
                gel_context_eval_params_va(self,
                    func, list, node, &n_values, &values, args);
            }
            else
            {
                gel_warning_value_not_of_type(func,
                    *values, G_TYPE_VALUE_ARRAY);
                parsed = FALSE;
            }

        if(parsed)
        {
            node = g_node_next_sibling(node);
            (*values)++;
            (*n_values)--;
        }
    }

    if(!parsed)
    {
        *list = o_list;
        *n_values = o_n_values;
        *values = o_values;
    }
    return parsed;
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
 * Posible formats are: vVaAHsSIOC
 *
 * <itemizedlist>
  *   <listitem><para>
 *     v: get a literal value (#GValue *).
 *   </para></listitem>
 *   <listitem><para>
 *     V: evaluate and get a value (#GValue *).
 *   </para></listitem>
 *   <listitem><para>
 *     a: get a literal array (#GValueArray *).
 *   </para></listitem>
 *   <listitem><para>
 *     A: evaluate and get an array (#GValueArray *).
 *   </para></listitem>
 *   <listitem><para>
 *     H: evaluate and get a hash (#GHashTable *).
 *   </para></listitem>
 *   <listitem><para>
 *     s: get a literal string (#gchararray).
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
    va_list args;
    va_start(args, values);

    guint format_pos = 0;
    GNode *format_node = gel_params_format_to_node(format, &format_pos);

    gboolean parsed = gel_context_eval_params_va(self,
        func, list, format_node, n_values, values, args);

    g_node_destroy(format_node);
    va_end(args);
    return parsed;
}

