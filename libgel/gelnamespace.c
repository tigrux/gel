#include <gelnamespace.h>


static
void gel_namespace_to_string_transform(const GValue *source, GValue *dest)
{
    GelNamespace *gel_ns = (GelNamespace *)g_value_get_boxed(source);

    const gchar *name = gel_namespace_get_name(gel_ns);
    gchar *buffer = g_strdup_printf("<GelNamespace %s>", name);
    g_value_take_string(dest, buffer);
}


GType gel_namespace_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;
    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelNamespace",
                (GBoxedCopyFunc)gel_namespace_ref,
                (GBoxedFreeFunc)gel_namespace_unref);

        g_value_register_transform_func(type, G_TYPE_STRING,
            gel_namespace_to_string_transform);

        g_once_init_leave(&once, 1);
    }
    return type;
}


struct _GelNamespace
{
    gchar *name;
    GHashTable *infos;
    volatile gint ref_count;
};


GelNamespace* gel_namespace_new(const gchar *namespace_, const gchar *version)
{
    GelNamespace *self = NULL;
    if(g_irepository_require(NULL, namespace_, version, 0, NULL) != NULL)
    {
        GHashTable *infos = g_hash_table_new_full(
            g_str_hash, g_str_equal,
            NULL, (GDestroyNotify)gel_base_info_unref);

        guint n = g_irepository_get_n_infos(NULL, namespace_);
        guint i;
        for(i = 0; i < n; i++)
        {
            GIBaseInfo *info = g_irepository_get_info(NULL, namespace_, i);
            g_hash_table_insert(infos,
                (void*)g_base_info_get_name(info), gel_base_info_new(info));
        }

        self = g_slice_new0(GelNamespace);
        self->ref_count = 1;
        self->name = g_strdup(namespace_);
        self->infos = infos;
    }

    return self;
}


GelNamespace* gel_namespace_ref(GelNamespace *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    g_atomic_int_inc(&self->ref_count);
    return self;
}


void gel_namespace_unref(GelNamespace *self)
{
    g_return_if_fail(self != NULL);

    if(g_atomic_int_dec_and_test(&self->ref_count))
    {
        g_free(self->name);
        g_hash_table_unref(self->infos);
        g_slice_free(GelNamespace, self);
    }
}


const gchar* gel_namespace_get_name(const GelNamespace *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    return self->name;
}


const GelBaseInfo* gel_namespace_lookup(const GelNamespace *self,
                                        const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);

    return (GelBaseInfo *)g_hash_table_lookup(self->infos, name);
}

