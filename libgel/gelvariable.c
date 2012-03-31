#include <gelvariable.h>
#include <gelvalueprivate.h>


GType gel_variable_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;
    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelVariable",
                (GBoxedCopyFunc)gel_variable_ref,
                (GBoxedFreeFunc)gel_variable_unref);
        g_once_init_leave(&once, 1);
    }
    return type;
}


struct _GelVariable
{
    GValue *value;
    gboolean owned;
    volatile gint ref_count;
};


GelVariable* gel_variable_new(GValue *value, gboolean owned)
{
    g_return_val_if_fail(value != NULL, NULL);

    GelVariable *self = g_slice_new0(GelVariable);
    self->value = value;
    self->owned = owned;
    self->ref_count = 1;

    return self;
}


GelVariable* gel_variable_ref(GelVariable *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    g_atomic_int_inc(&self->ref_count);
    return self;
}


void gel_variable_unref(GelVariable *self)
{
    g_return_if_fail(self != NULL);

    if(g_atomic_int_dec_and_test(&self->ref_count))
    {
        if(self->owned)
            gel_value_free(self->value);
        g_slice_free(GelVariable, self);
    }
}


GValue* gel_variable_get_value(const GelVariable *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    return self->value;
}


gboolean gel_variable_get_owned(const GelVariable *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    return self->owned;
}

