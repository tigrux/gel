#include <gelsymbol.h>
#include <gelvariable.h>


G_DEFINE_BOXED_TYPE(GelSymbol, gel_symbol, gel_symbol_dup, gel_symbol_free)


struct _GelSymbol
{
    gchar *name;
    GelVariable *variable;
};


GelSymbol* gel_symbol_new(const gchar *name)
{
    g_return_val_if_fail(name != NULL, NULL);

    GelSymbol *self = g_slice_new0(GelSymbol);
    self->name = g_strdup(name);

    return self;
}


GelSymbol* gel_symbol_dup(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    GelSymbol *symbol = gel_symbol_new(self->name);
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


const GValue* gel_symbol_get_value(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    register GValue *value;
    if(self->variable != NULL)
        value = gel_variable_get_value(self->variable);
    else
        value = NULL;
    return value;
}

