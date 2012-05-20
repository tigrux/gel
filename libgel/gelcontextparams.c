#include <gelcontext.h>
#include <gelcontextprivate.h>
#include <gelerrors.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelsymbol.h>

#define g_node_get_int(node) \
    (GPOINTER_TO_INT((node)->data))

#define g_node_add_int(node, data) \
    g_node_append_data(node, GINT_TO_POINTER(data))

#define gel_args_pop(args, type) \
    (**((type **)(*(args))++))

static
GNode *gel_params_format_to_node(const gchar *format, guint *pos, guint *count)
{
    GNode *root = g_node_new(NULL);
    while(format[*pos] != 0)
    {
        if(format[*pos] == '(')
        {
            (*pos)++;
            GNode *child = gel_params_format_to_node(format, pos, count);
            g_node_append(root, child);
        }
        else
        if(format[*pos] == ')')
            break;
        else
        {
            gint data = format[*pos];
            g_node_add_int(root, data);
            if(data != '*')
                (*count)++;
        }
        (*pos)++;
    }

    return root;
}


static
GNode* gel_params_parse_format(const gchar *format, guint *count)
{
    g_return_val_if_fail(count != NULL, NULL);

    guint pos = 0;
    *count = 0;

    return gel_params_format_to_node(format, &pos, count);
}


static
gboolean gel_context_eval_param(GelContext *self, const gchar *func,
                                guint *n_values, const GValue **values,
                                GList **list, gchar format, void ***args)
{
    GValue *value = NULL;
    const GValue *result = NULL;
    gboolean parsed = TRUE;

    switch(format)
    {
        case 'v':
            gel_args_pop(args, const GValue *) = *values;
            break;
        case 'V':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
                gel_args_pop(args, const GValue *) = result;
            break;
        case 'a':
            if(G_VALUE_HOLDS(*values, GEL_TYPE_ARRAY))
                gel_args_pop(args, void *) = g_value_get_boxed(*values);
            else
            {
                gel_error_value_not_of_type(self,
                    func, *values, GEL_TYPE_ARRAY);
                parsed = FALSE;
            }
            break;
        case 'A':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, GEL_TYPE_ARRAY))
                gel_args_pop(args, void *) = g_value_get_boxed(result);
            else
            {
                gel_error_value_not_of_type(self,
                    func, result, GEL_TYPE_ARRAY);
                parsed = FALSE;
            }
            break;
        case 'H':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_HASH_TABLE))
                gel_args_pop(args, void *) = g_value_get_boxed(result);
            else
            {
                gel_error_value_not_of_type(self,
                    func, result, G_TYPE_HASH_TABLE);
                parsed = FALSE;
            }
            break;
        case 's':
            if(G_VALUE_HOLDS(*values, GEL_TYPE_SYMBOL))
            {
                GelSymbol *symbol = g_value_get_boxed(*values);
                gel_args_pop(args, const gchar *) = gel_symbol_get_name(symbol);
            }
            else
            {
                gel_error_value_not_of_type(self,
                    func, *values, GEL_TYPE_SYMBOL);
                parsed = FALSE;
            }
            break;
        case 'S':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_STRING))
                gel_args_pop(args, const gchar *) = g_value_get_string(result);
            else
            {
                gel_error_value_not_of_type(self,
                    func, result, G_TYPE_STRING);
                parsed = FALSE;
            }
            break;
        case 'B':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
                gel_args_pop(args, gboolean) = gel_value_to_boolean(result);
            break;
        case 'I':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_INT64))
                gel_args_pop(args, gint64) = g_value_get_int64(result);
            else
            {
                GValue tmp = {0};
                g_value_init(&tmp, G_TYPE_INT64);
                if(g_value_transform(result, &tmp))
                    gel_args_pop(args, gint64) = g_value_get_int64(&tmp);
                else
                {
                    gel_error_value_not_of_type(self,
                        func, result, G_TYPE_INT64);
                    parsed = FALSE;
                }
                g_value_unset(&tmp);
            }
            break;
        case 'F':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_DOUBLE))
                gel_args_pop(args, gdouble) = g_value_get_double(result);
            else
            {
                GValue tmp = {0};
                g_value_init(&tmp, G_TYPE_DOUBLE);
                if(g_value_transform(result, &tmp))
                    gel_args_pop(args, gdouble) = g_value_get_double(&tmp);
                else
                {
                    gel_error_value_not_of_type(self, 
                        func, result, G_TYPE_DOUBLE);
                    parsed = FALSE;
                }
                g_value_unset(&tmp);
            }
            break;
        case 'G':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_GTYPE))
                gel_args_pop(args, GType) = g_value_get_gtype(result);
            else
            {
                gel_error_value_not_of_type(self,
                    func, result, G_TYPE_GTYPE);
                parsed = FALSE;
            }
            break;
        case 'O':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_OBJECT))
                gel_args_pop(args, GObject *) = g_value_get_object(result);
            else
            {
                gel_error_value_not_of_type(self,
                    func, result, G_TYPE_OBJECT);
                parsed = FALSE;
            }
            break;
        case 'X':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_BOXED))
                gel_args_pop(args, void *) = g_value_get_boxed(result);
            else
            {
                gel_error_value_not_of_type(self,
                    func, result, G_TYPE_BOXED);
                parsed = FALSE;
            }
            break;
        case 'C':
            value = gel_value_new();
            result =
                gel_context_eval_param_into_value(self, *values, value);
            if(gel_context_error(self))
                parsed = FALSE;
            else
            if(G_VALUE_HOLDS(result, G_TYPE_CLOSURE))
                gel_args_pop(args, GClosure *) = g_value_get_boxed(result);
            else
            {
                gel_error_value_not_of_type(self,
                    func, result, G_TYPE_CLOSURE);
                parsed = FALSE;
            }
            break;
    }

    if(value != NULL)
    {
        if(parsed && result == value)
            *list = g_list_append(*list, value);
        else
            gel_value_free(value);
    }

    return parsed;
}


static
gboolean gel_context_eval_params_args(GelContext *self, const gchar *func,
                                      guint *n_values, const GValue **values,
                                      GList **list, GNode *format_node,
                                      void ***args)

{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(n_values != NULL, FALSE);
    g_return_val_if_fail(values != NULL, FALSE);

    gboolean exact = TRUE;
    
    guint n_args = g_node_n_children(format_node);

    if(n_args > 0)
    {
        GNode *last_child = g_node_last_child(format_node);
        if(g_node_get_int(last_child) == '*')
        {
            exact = FALSE;
            g_node_destroy(last_child);
            n_args--;
        }
    }


    if(exact && n_args != *n_values)
    {
        gel_error_needs_n_arguments(self, func, n_args);
        return FALSE;
    }

    if(!exact && n_args > *n_values)
    {
        gel_error_needs_at_least_n_arguments(self, func, n_args);
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
            gchar format = g_node_get_int(node);
            parsed = gel_context_eval_param(self,
                func, n_values, values, list, format, args);
        }
        else
            if(G_VALUE_HOLDS(*values, GEL_TYPE_ARRAY))
            {
                GelArray *array = g_value_get_boxed(*values);
                guint n_values = gel_array_get_n_values(array);
                const GValue *values = gel_array_get_values(array);

                gel_context_eval_params_args(self,
                    func, &n_values, &values, list, node,  args);
            }
            else
            {
                gel_error_value_not_of_type(self,
                    func, *values, GEL_TYPE_ARRAY);
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
 * @self: a #GelContext to evaluate
 * @func: the name of function that is evaluating, for debug
 * @n_values: the number of #GValue to evaluate
 * @values: the array of #GValue to evaluate
 * @list: the location of a #GList to keep the temporary values
 * @format: a string that indicates the types of the values
 * @...: The return location for the respective variables
 * 
 * Evaluates an array of #GValue and store the results.
 * Any temporary values will be stored in @list.
 * The list must be freed with @gel_list_free.
 *
 * Returns: #TRUE if all the values were evaluated, #FALSE otherwise
 */

gboolean gel_context_eval_params(GelContext *self,const gchar *func,
                                 guint *n_values, const GValue **values,
                                 GList **list, const gchar *format, ...)
{
    guint n_args;
    GNode *format_node = gel_params_parse_format(format, &n_args);
    void **args = g_new0(void *, n_args);

    va_list args_va;
    va_start(args_va, format);
    for(guint i = 0; i < n_args; i++)
        args[i] = va_arg(args_va, void *);
    va_end(args_va);

    void **args_iter = args;
    gboolean parsed = gel_context_eval_params_args(self,
        func, n_values, values, list, format_node, &args_iter);

    g_free(args);
    g_node_destroy(format_node);
 
    return parsed;
}

