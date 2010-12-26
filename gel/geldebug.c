#include <geldebug.h>
#include <gelvalue.h>


static
const gchar* plural(guint n)
{
    return (n == 1 ? "" : "s");
}


void gel_warning_needs_at_least_n_arguments(guint n)
{
    g_warning("Needs at least %u argument%s", n, plural(n));
}


void gel_warning_needs_n_arguments(guint n)
{
    g_warning("Needs %u argument%s", n, plural(n));
}


void gel_warning_no_such_property(const gchar *prop_name)
{
    g_warning("No such property '%s'", prop_name);
}


void gel_warning_value_not_of_type(const GValue *value, GType type)
{
    gchar *value_string = gel_value_to_string(value);
    g_warning("'%s' is not of type '%s'", value_string, g_type_name(type));
    g_free(value_string);
}


void gel_warning_unknown_symbol(const gchar *symbol)
{
    g_warning("Unknown symbol '%s'", symbol);
}


void gel_warning_type_not_instantiatable(GType type)
{
    g_print("Type %s is not instantiatable", g_type_name(type));
}


void gel_warning_invalid_value_for_property(const GValue *value,
                                            const GParamSpec *pspec)
{
    gchar *value_string = gel_value_to_string(value);
    g_print("'%s' of type '%s' is invalid for property '%s' of type '%s'",
        value_string, G_VALUE_TYPE_NAME(value),
        pspec->name, g_type_name(pspec->value_type));
    g_free(value_string);
}


void gel_warning_type_name_invalid(const gchar *name)
{
    g_warning("'%s' is not a registered type", name);
}


void gel_warning_invalid_argument_name(const gchar *name)
{
    g_warning("'%s' is an invalid argument name", name);
}

