#include <gelbaseinfo.h>


static
void gel_base_info_to_string_transform(const GValue *source, GValue *dest)
{
    GelBaseInfo *gel_info = (GelBaseInfo *)g_value_get_boxed(source);
    GIBaseInfo *base_info = gel_base_info_get_info(gel_info);

    const gchar *type = g_info_type_to_string(g_base_info_get_type(base_info));
    const gchar *name = g_base_info_get_name(base_info);
    gchar *buffer = g_strdup_printf("<GelBaseInfo %s %s>", type, name);
    g_value_take_string(dest, buffer);
}


GType gel_base_info_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;
    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelBaseInfo",
                (GBoxedCopyFunc)gel_base_info_ref,
                (GBoxedFreeFunc)gel_base_info_unref);

        g_value_register_transform_func(type, G_TYPE_STRING,
            gel_base_info_to_string_transform);

        g_once_init_leave(&once, 1);
    }
    return type;
}


struct _GelBaseInfo
{
    GIBaseInfo *info;
    GHashTable *nodes;
    volatile gint ref_count;
};


GelBaseInfo* gel_base_info_new(GIBaseInfo *info)
{
    GelBaseInfo *self = g_slice_new0(GelBaseInfo);
    self->info = info;
    self->nodes = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        NULL, (GDestroyNotify)g_base_info_unref);
    self->ref_count = 1;

    return self;
}


GelBaseInfo* gel_base_info_ref(GelBaseInfo *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    g_atomic_int_inc(&self->ref_count);
    return self;
}


void gel_base_info_unref(GelBaseInfo *self)
{
    g_return_if_fail(self != NULL);

    if(g_atomic_int_dec_and_test(&self->ref_count))
    {
        g_base_info_unref(self->info);
        g_hash_table_unref(self->nodes);
        g_slice_free(GelBaseInfo, self);
    }
}


const gchar* gel_base_info_get_name(const GelBaseInfo *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    return g_base_info_get_name(self->info);
}


GIBaseInfo* gel_base_info_get_info(const GelBaseInfo *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    return self->info;
}


const GelBaseInfo* gel_base_info_get_node(const GelBaseInfo *self,
                                          const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);

    return (GelBaseInfo *)g_hash_table_lookup(self->nodes, name);
}

