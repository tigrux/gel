#include <geltypelib.h>


static
void gel_typelib_to_string_transform(const GValue *source, GValue *dest)
{
    GelTypelib *gel_ns = (GelTypelib *)g_value_get_boxed(source);

    const gchar *name = gel_typelib_get_name(gel_ns);
    gchar *buffer = g_strdup_printf("<GelTypelib %s>", name);
    g_value_take_string(dest, buffer);
}


GType gel_typelib_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;
    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelTypelib",
                (GBoxedCopyFunc)gel_typelib_ref,
                (GBoxedFreeFunc)gel_typelib_unref);

        g_value_register_transform_func(type, G_TYPE_STRING,
            gel_typelib_to_string_transform);

        g_once_init_leave(&once, 1);
    }
    return type;
}


struct _GelTypelib
{
    GITypelib *typelib;
    GHashTable *infos;
    volatile gint ref_count;
};


GelTypelib* gel_typelib_new(const gchar *ns, const gchar *version)
{
    GelTypelib *self = NULL;

    GITypelib *typelib = g_irepository_require(NULL, ns, version, 0, NULL);
    if(typelib != NULL)
    {
        GHashTable *infos = g_hash_table_new_full(
            g_str_hash, g_str_equal,
            NULL, (GDestroyNotify)gel_typeinfo_unref);

        guint n = g_irepository_get_n_infos(NULL, ns);
        guint i;
        for(i = 0; i < n; i++)
        {
            GIBaseInfo *info = g_irepository_get_info(NULL, ns, i);
            g_hash_table_insert(infos,
                (void*)g_base_info_get_name(info), gel_typeinfo_new(info));
        }

        self = g_slice_new0(GelTypelib);
        self->ref_count = 1;
        self->typelib = typelib;
        self->infos = infos;
    }

    return self;
}


GelTypelib* gel_typelib_ref(GelTypelib *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    g_atomic_int_inc(&self->ref_count);
    return self;
}


void gel_typelib_unref(GelTypelib *self)
{
    g_return_if_fail(self != NULL);

    if(g_atomic_int_dec_and_test(&self->ref_count))
    {
        g_hash_table_unref(self->infos);
        g_slice_free(GelTypelib, self);
    }
}


const gchar* gel_typelib_get_name(const GelTypelib *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    return g_typelib_get_namespace(self->typelib);
}


const GelTypeinfo* gel_typelib_lookup(const GelTypelib *self,
                                      const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);

    return (GelTypeinfo *)g_hash_table_lookup(self->infos, name);
}

