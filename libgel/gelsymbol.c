#include <gelsymbol.h>


struct _GelSymbol
{
    gchar *name;
    GelVariable *variable;
};


GType gel_symbol_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;

    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelSymbol",
            (GBoxedCopyFunc)gel_symbol_dup, (GBoxedFreeFunc)gel_symbol_free);
        g_once_init_leave(&once, 1);
    }

    return type;
}


GelSymbol* gel_symbol_new(const gchar *name, GelVariable *variable)
{
    g_return_val_if_fail(name != NULL, NULL);

    GelSymbol *self = g_slice_new0(GelSymbol);
    self->name = g_strdup(name);

    if(variable != NULL)
        self->variable = gel_variable_ref(variable);
    else
        self->variable = NULL;

    return self;
}


GelSymbol* gel_symbol_dup(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    return gel_symbol_new(self->name, self->variable);
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
    self->variable = variable != NULL ? gel_variable_ref(variable) : NULL;
}


GValue* gel_symbol_get_value(const GelSymbol *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    GValue *value = NULL;
    if(self->variable != NULL)
        value = gel_variable_get_value(self->variable);

    return value;
}

