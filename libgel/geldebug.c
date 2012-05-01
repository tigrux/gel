#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>


static
const gchar* plural(guint n)
{
    return (n == 1 ? "" : "s");
}


void gel_warning_needs_at_least_n_arguments(GelContext *context,const gchar *f,
                                            guint n)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_ARGUMENTS,
        "%s: Needs at least %u argument%s", f, n, plural(n)));
}


void gel_warning_needs_n_arguments(GelContext *context, const gchar *f,
                                   guint n)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_ARGUMENTS,
        "%s: Needs %u argument%s", f, n, plural(n)));
}


void gel_warning_no_such_property(GelContext *context, const gchar *f,
                                  const gchar *prop_name)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_PROPERTY,
        "%s: No such property '%s'", f, prop_name));
}


void gel_warning_value_not_of_type(GelContext *context, const gchar *f,
                                   const GValue *value, GType type)
{
    gchar *value_string = gel_value_to_string(value);

    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_TYPE,
        "%s: '%s' is not of type '%s'", f,
        value_string, g_type_name(type)));

    g_free(value_string);
}


void gel_warning_unknown_symbol(GelContext *context, const gchar *f,
                                const gchar *symbol)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_SYMBOL,
        "%s: Unknown symbol '%s'", f, symbol));
}


void gel_warning_type_not_instantiatable(GelContext *context, const gchar *f,
                                         GType type)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_TYPE,
        "%s: Type %s is not instantiatable", f, g_type_name(type)));
}


void gel_warning_invalid_value_for_property(GelContext *context, const gchar *f,
                                            const GValue *value,
                                            const GParamSpec *pspec)
{
    gchar *value_string = gel_value_to_string(value);

    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_PROPERTY,
        "%s: '%s' of type '%s' is invalid for property '%s' of type '%s'",
        f, value_string, GEL_VALUE_TYPE_NAME(value),
        pspec->name, g_type_name(pspec->value_type)));

    g_free(value_string);
}


void gel_warning_type_name_invalid(GelContext *context, const gchar *f,
                                   const gchar *name)
{

    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_TYPE,
        "%s: '%s' is not a registered type", f, name));
}


void gel_warning_invalid_argument_name(GelContext *context, const gchar *f,
                                       const gchar *name)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_ARGUMENTS,
        "%s: '%s' is an invalid argument name", f, name));
}


void gel_warning_index_out_of_bounds(GelContext *context, const gchar *f,
                                     gint index)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_INDEX,
        "%s: Index %d out of bounds", f, index));
}


void gel_warning_invalid_key(GelContext *context, const gchar *f,
                             GValue *key)
{
    gchar *value_string = gel_value_to_string(key);

    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_KEY,
        "%s: Invalid key %s", f, value_string));

    g_free(value_string);
}


void gel_warning_symbol_exists(GelContext *context, const gchar *f,
                               const gchar *name)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_SYMBOL,
        "%s: Symbol '%s' already exists", f, name));
}


void gel_warning_expected(GelContext *context, const gchar *f,
                          const gchar *s)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_ARGUMENTS,
        "%s: Expected %s", f, s));
}


void gel_warning_incompatible(GelContext *context, const gchar *f,
                              const GValue *v1, const GValue *v2)
{
    gchar *s1 = gel_value_to_string(v1);
    gchar *s2 = gel_value_to_string(v2);

    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_ARGUMENTS,
        "%s: Incompatible values: %s and %s", f, s1, s2));

    g_free(s1);
    g_free(s2);
}
