#include <gelsymbol.h>


G_DEFINE_BOXED_TYPE(GelSymbol, gel_symbol, gel_symbol_dup, gel_symbol_free)


struct _GelSymbol
{
    gchar *name;
    GelVariable *variable;
};


GelSymbol* gel_symbol_new(const gchar *name, GelVariable *variable)
{
    g_return_val_if_fail(name != NULL, NULL);

    GelSymbol *self = g_slice_new0(GelSymbol);
    self->name = g_strdup(name);
    self->variable = variable;

    return self;
}


GelSymbol* gel_symbol_dup(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    GelSymbol *symbol = gel_symbol_new(self->name, NULL);
    if(self->variable != NULL)
        symbol->variable = gel_variable_ref(self->variable);
    else
        symbol->variable = NULL;

    return symbol;
}


void gel_symbol_free(GelSymbol *self)
{
    g_return_if_fail(self != NULL);

    g_free(self->name);
    if(self->variable != NULL)
        gel_variable_unref(self->variable);
    g_slice_free(GelSymbol, self);
}


const gchar* gel_symbol_get_name(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    return self->name;
}


GelVariable* gel_symbol_get_variable(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    return self->variable;
}


void gel_symbol_set_variable(GelSymbol *self, GelVariable *variable)
{
    g_return_if_fail(self != NULL);

    if(self->variable != NULL)
        gel_variable_unref(self->variable);
    if(variable != NULL)
        self->variable = gel_variable_ref(variable);
    else
        self->variable = NULL;
}


GValue* gel_symbol_get_value(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    GValue *value;
    if(self->variable != NULL)
        value = gel_variable_get_value(self->variable);
    else
        value = NULL;

    return value;
}

