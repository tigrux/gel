#include <geldebug.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>


static
const gchar* plural(guint n)
{
    return (n == 1 ? "" : "s");
}


void gel_warning_needs_at_least_n_arguments(GelContext *context,
                                            const gchar *func, guint n)
{
    g_warning("%s: Needs at least %u argument%s", func, n, plural(n));
}


void gel_warning_needs_n_arguments(GelContext *context,
                                   const gchar *func, guint n)
{
    g_warning("%s: Needs %u argument%s", func, n, plural(n));
}


void gel_warning_no_such_property(GelContext *context,
                                  const gchar *func, const gchar *prop_name)
{
    g_warning("%s: No such property '%s'", func, prop_name);
}


void gel_warning_value_not_of_type(GelContext *context, const gchar *func,
                                   const GValue *value, GType type)
{
    gchar *value_string = gel_value_to_string(value);

    g_warning("%s: '%s' is not of type '%s'", func,
        value_string, g_type_name(type));

    g_free(value_string);
}


void gel_warning_unknown_symbol(GelContext *context,
                                const gchar *func, const gchar *symbol)
{
    g_warning("%s: Unknown symbol '%s'", func, symbol);
}


void gel_warning_type_not_instantiatable(GelContext *context,
                                         const gchar *func, GType type)
{
    g_warning("%s: Type %s is not instantiatable", func, g_type_name(type));
}


void gel_warning_invalid_value_for_property(GelContext *context,
                                            const gchar *func,
                                            const GValue *value,
                                            const GParamSpec *pspec)
{
    gchar *value_string = gel_value_to_string(value);

    g_warning("%s: '%s' of type '%s' is invalid for property '%s' of type '%s'",
        func, value_string, GEL_VALUE_TYPE_NAME(value),
        pspec->name, g_type_name(pspec->value_type));

    g_free(value_string);
}


void gel_warning_type_name_invalid(GelContext *context,
                                   const gchar *func, const gchar *name)
{
    g_warning("%s: '%s' is not a registered type", func, name);
}


void gel_warning_invalid_argument_name(GelContext *context,
                                       const gchar *func, const gchar *name)
{
    g_warning("%s: '%s' is an invalid argument name", func, name);
}


void gel_warning_index_out_of_bounds(GelContext *context,
                                     const gchar *func)
{
    g_warning("%s: Index out of bounds", func);
}


void gel_warning_symbol_exists(GelContext *context,
                               const gchar *func, const gchar *name)
{
    g_warning("%s: Symbol '%s' already exists", func, name);
}


void gel_warning_expected(GelContext *context,
                          const gchar *func, const gchar *s)
{
    gel_context_set_error(context, g_error_new(
        GEL_CONTEXT_ERROR, GEL_CONTEXT_ERROR_EXPECTED,
        "%s: Expected %s", func, s));
}

