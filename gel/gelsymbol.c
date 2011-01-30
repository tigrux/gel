#include <gelsymbol.h>


G_DEFINE_BOXED_TYPE(GelSymbol, gel_symbol, gel_symbol_dup, gel_symbol_free)


GelSymbol *gel_symbol_new(const gchar *name)
{
    g_return_val_if_fail(name != NULL, NULL);

    GelSymbol *self = g_slice_new0(GelSymbol);
    self->name = g_strdup(name);

    return self;
}


GelSymbol *gel_symbol_dup(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    GelSymbol *symbol = gel_symbol_new(self->name);
    symbol->value = self->value;

    return symbol;
}


void gel_symbol_free(GelSymbol *self)
{
    g_return_if_fail(self != NULL);
    g_free(self->name);
    g_slice_free(GelSymbol, self);
}

