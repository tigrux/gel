#include <string.h>

#include <gelcontext.h>
#include <gelvalue.h>
#include <gelclosure.h>
#include <gelvaluelist.h>
#include <geldebug.h>

#define GEL_CONTEXT_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), GEL_TYPE_CONTEXT, GelContextPrivate))


struct _GelContextPrivate
{
    GelContext *outer;
    GHashTable *symbols;
};


enum
{
    GEL_CONTEXT_OUTER = 1
};


GelContext* gel_context_new(void)
{
    return (GelContext*)g_object_new(GEL_TYPE_CONTEXT,  NULL);
}


GelContext* gel_context_new_with_outer(GelContext *outer)
{
    return (GelContext*)g_object_new(GEL_TYPE_CONTEXT, "outer", outer, NULL);
}


void gel_context_unref(GelContext *self)
{
    g_return_if_fail(self != NULL);
    g_object_unref(G_OBJECT(self));
}


static
void gel_context_invoke_closure(GelContext *self, GClosure *closure,
                                guint n_values, GValue *values,
                                GValue *dest_value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(closure != NULL);
    g_return_if_fail(dest_value != NULL);

    if(gel_closure_is_pure(closure))
    {
        GValueArray *array = g_value_array_new(n_values);
        register guint i;
        for(i = 0; i < n_values; i++)
        {
            GValue tmp_value = {0};
            g_value_array_append(array,
                gel_context_eval_value(self, values+i, &tmp_value));
            if(G_IS_VALUE(&tmp_value))
                g_value_unset(&tmp_value);
        }

        g_closure_invoke(
            closure, dest_value, array->n_values, array->values, self);
        g_value_array_free(array);
    }
    else
        g_closure_invoke(closure, dest_value, n_values , values, self);
}


static
void gel_context_invoke_type(GelContext *self, GType type,
                             guint n_values, GValue *values,
                             GValue *dest_value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(type != G_TYPE_INVALID);
    g_return_if_fail(dest_value != NULL);

    register guint i;
    for(i = 0; i < n_values; i++)
    {
        g_return_if_fail(G_VALUE_HOLDS_STRING(values+i));
        const gchar *var_name = g_value_get_string(values+i);
        GValue *var_value = gel_value_new_of_type(type);
        gel_context_add_symbol(self, var_name, var_value);
    }

    GValue count_value = {0};
    g_value_init(&count_value, G_TYPE_GTYPE);
    g_value_set_gtype(&count_value, type);
    gel_value_copy(&count_value, dest_value);
}


static
const GValue* gel_context_eval_array(GelContext *self, const GValueArray *array,
                                     GValue *dest_value)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(dest_value != NULL, NULL);
    g_return_val_if_fail(array != NULL, NULL);
    g_return_val_if_fail(array->n_values > 0, NULL);

    const GValue *result_value = NULL;

    if(G_VALUE_HOLDS_STRING(array->values + 0))
    {
        const gchar *type_name = g_value_get_string(array->values + 0);
        GType type = g_type_from_name(type_name);
        if(type != G_TYPE_INVALID)
        {
            gel_context_invoke_type(self, type,
                array->n_values - 1, array->values + 1, dest_value);
            result_value = dest_value;
        }
    }

    if(result_value == NULL)
    {
        GValue tmp_value = {0};
        const GValue *first_value =
            gel_context_eval_value(self, array->values + 0, &tmp_value);

        if(G_VALUE_HOLDS(first_value, G_TYPE_CLOSURE))
        {
            GClosure *closure = (GClosure*)g_value_get_boxed(first_value);
            gel_context_invoke_closure(self, closure,
                array->n_values - 1, array->values + 1, dest_value);
            result_value = dest_value;
        }
        else
            gel_warning_value_not_of_type(__FUNCTION__,
                first_value, G_TYPE_CLOSURE);

        if(G_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }

    return result_value;
}


const GValue* gel_context_eval_value(GelContext *self,
                                     const GValue *value, GValue *dest_value)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(dest_value != NULL, NULL);

    const GValue *result_value = NULL;

    if(G_VALUE_HOLDS(value, G_TYPE_STRING))
    {
        const gchar *symbol_name = g_value_get_string(value);
        GValue *symbol_value = gel_context_find_symbol(self, symbol_name);
        if(symbol_value != NULL)
            result_value = symbol_value;
        else
            gel_warning_unknown_symbol(__FUNCTION__, symbol_name);
    }
    else
    if(G_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY))
    {
        GValueArray *array = (GValueArray*)g_value_get_boxed(value);
        result_value = gel_context_eval_array(self, array, dest_value);
    }

    if(result_value == NULL)
        result_value = value;

    return result_value;
}



gboolean gel_context_eval_params(GelContext *self, const gchar *func,
                                 GList **list, const gchar *format,
                                 guint *n_values, const GValue **values, ...)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(n_values != NULL, FALSE);
    g_return_val_if_fail(values != NULL, FALSE);

    guint n_args = 0;
    register guint i;
    for(i = 0; format[i] != 0; i++)
        if(strchr("asOASCV", format[i]) != NULL)
            n_args++;

    gboolean exact = (strchr(format, '*') == NULL);

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
    gboolean pending = FALSE;
    va_list args;

    GList *o_list = *list;
    guint o_n_values = *n_values;
    const GValue *o_values = *values;

    va_start(args, values);
    while(*format != 0 && *n_values >= 0 && parsed && !pending)
    {
        GValue *value = NULL;
        const GValue *result_value = NULL;

        switch(*format)
        {
            case 'a':
                if(G_VALUE_HOLDS(*values, G_TYPE_VALUE_ARRAY))
                {
                    GValueArray **a = va_arg(args, GValueArray **);
                    *a = (GValueArray*)g_value_get_boxed(*values);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        value, G_TYPE_VALUE_ARRAY);
                    parsed = FALSE;
                }
                break;
            case 's':
                if(G_VALUE_HOLDS_STRING(*values))
                {
                    const gchar **s = va_arg(args, const gchar **);
                    *s = g_value_get_string(*values);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        value, G_TYPE_STRING);
                    parsed = FALSE;
                }
                break;
            case 'O':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(G_VALUE_HOLDS_OBJECT(result_value))
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
                if(G_VALUE_HOLDS(result_value, G_TYPE_VALUE_ARRAY))
                {
                    GValueArray **a = va_arg(args, GValueArray **);
                    *a = (GValueArray*)g_value_get_boxed(result_value);
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
                if(G_VALUE_HOLDS_STRING(result_value))
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
            case 'C':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(G_VALUE_HOLDS(result_value, G_TYPE_CLOSURE))
                {
                    GClosure **closure = va_arg(args, GClosure **);
                    *closure = (GClosure*)g_value_get_boxed(result_value);
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        result_value, G_TYPE_STRING);
                    parsed = FALSE;
                }
                break;
            case 'V':
                value = gel_value_new();
                result_value = gel_context_eval_value(self, *values, value);
                if(G_IS_VALUE(result_value))
                {
                    const GValue **v = va_arg(args, const GValue **);
                    *v = result_value;
                }
                else
                {
                    gel_warning_value_not_of_type(func,
                        result_value, G_TYPE_VALUE);
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


GValue* gel_context_find_symbol(const GelContext *self, const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    GValue *symbol = (GValue*)g_hash_table_lookup(
        self->priv->symbols, name);
    if(symbol == NULL && self->priv->outer != NULL)
        symbol = gel_context_find_symbol(self->priv->outer, name);
    return symbol;
}


void gel_context_add_symbol(GelContext *self, const gchar *name, GValue *value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(value != NULL);
    g_hash_table_insert(self->priv->symbols, g_strdup(name), value);
}


void gel_context_add_object(GelContext *self, const gchar *name, GObject *obj)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_return_if_fail(obj != NULL);
    g_return_if_fail(G_IS_OBJECT(obj));

    GValue *value = gel_value_new_of_type(G_OBJECT_TYPE(obj));
    g_value_take_object(value, obj);
    gel_context_add_symbol(self, name, value);
}


void gel_context_remove_symbol(GelContext *self, const gchar *name)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(name != NULL);
    g_hash_table_remove(self->priv->symbols, name);
}


GelContext* gel_context_get_outer(const GelContext* self)
{
    g_return_val_if_fail(self != NULL, NULL);
    return self->priv->outer;
}


static
void gel_context_set_outer(GelContext* self, GelContext* outer)
{
    g_return_if_fail(self != NULL);
    self->priv->outer = outer;
    g_object_notify(G_OBJECT(self), "outer");
}


static
void gel_context_get_property(GObject * object, guint property_id,
                              GValue * value, GParamSpec * pspec)
{
    GelContext *self = GEL_CONTEXT(object);
    switch(property_id)
    {
        case GEL_CONTEXT_OUTER:
            g_value_set_object(value, gel_context_get_outer(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}


static void gel_context_set_property(GObject *object, guint property_id,
                                     const GValue *value, GParamSpec *pspec)
{
    GelContext *self;
    self = GEL_CONTEXT(object);
    switch(property_id)
    {
        case GEL_CONTEXT_OUTER:
            gel_context_set_outer(self, (GelContext*)g_value_get_object(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}


static
gpointer gel_context_parent_class = NULL;


static
GObject *gel_context_constructor(GType type,
                                 guint n_properties,
                                 GObjectConstructParam *properties)
{
    GObjectClass *parent_class = G_OBJECT_CLASS(gel_context_parent_class);
    GObject *obj = parent_class->constructor(type, n_properties, properties);
    GelContext *self = GEL_CONTEXT(obj);

    self->priv->symbols = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        (GDestroyNotify)g_free, (GDestroyNotify)gel_value_free);

    if(self->priv->outer == NULL)
        gel_context_add_default_symbols(self);
    return obj;
}


static
void gel_context_finalize(GObject *obj)
{
    GelContext *self = GEL_CONTEXT(obj);
    g_hash_table_destroy(self->priv->symbols);
    G_OBJECT_CLASS(gel_context_parent_class)->finalize(obj);
}


static
void gel_context_class_init(GelContextClass * klass)
{
    gel_context_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(klass, sizeof(GelContextPrivate));

    GObjectClass *g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->get_property = gel_context_get_property;
    g_object_class->set_property = gel_context_set_property;
    g_object_class->constructor = gel_context_constructor;
    g_object_class->finalize = gel_context_finalize;

    g_object_class_install_property(g_object_class,
        GEL_CONTEXT_OUTER,
        g_param_spec_object(
            "outer",
            "Outer",
            "Outer context",
            GEL_TYPE_CONTEXT,
            (GParamFlags)(
                G_PARAM_STATIC_NAME
                | G_PARAM_STATIC_NICK
                | G_PARAM_STATIC_BLURB
                | G_PARAM_READABLE
                | G_PARAM_WRITABLE
                | G_PARAM_CONSTRUCT_ONLY)));

    g_signal_new("quit",
        GEL_TYPE_CONTEXT,
        G_SIGNAL_RUN_LAST, 0,
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static
void gel_context_init(GelContext *self)
{
    self->priv = GEL_CONTEXT_GET_PRIVATE(self);
}


GType gel_context_get_type(void)
{
    static volatile gsize gel_context_type = 0;

    if(g_once_init_enter(&gel_context_type))
    {
        static const GTypeInfo type_info =
        {
            sizeof(GelContextClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) gel_context_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,
            sizeof(GelContext),
            0,
            (GInstanceInitFunc) gel_context_init,
            NULL
        };

        GType type = g_type_register_static(
            G_TYPE_OBJECT, "GelContext", &type_info, (GTypeFlags)0);
        g_once_init_leave(&gel_context_type, type);
    }

    return (GType)gel_context_type;
}

