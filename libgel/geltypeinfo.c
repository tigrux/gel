#include <geltypeinfo.h>


static
GHashTable *gel_typeinfo_gtypes = NULL;


struct _GelTypeinfo
{
    GIBaseInfo *info;
    GelTypeinfo *container;
    GHashTable *infos;
    volatile gint ref_count;
};


static
void gel_typeinfo_to_string_transform(const GValue *source, GValue *dest)
{
    GelTypeinfo *info = (GelTypeinfo *)g_value_get_boxed(source);

    const gchar *type = g_info_type_to_string(g_base_info_get_type(info->info));
    const gchar *name = g_base_info_get_name(info->info);
    gchar *buffer = g_strdup_printf("<GelTypeinfo %s %s>", type, name);
    g_value_take_string(dest, buffer);
}


GType gel_typeinfo_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;
    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelTypeinfo",
                (GBoxedCopyFunc)gel_typeinfo_ref,
                (GBoxedFreeFunc)gel_typeinfo_unref);

        g_value_register_transform_func(type, G_TYPE_STRING,
            gel_typeinfo_to_string_transform);

        g_once_init_leave(&once, 1);
    }
    return type;
}


static
void gel_typeinfo_insert_multiple(GelTypeinfo *self,
                              gint (*get_n_nodes)(GIBaseInfo *info),
                              GIBaseInfo* (*get_node)(GIBaseInfo *info, gint n))
{
    guint n = get_n_nodes(self->info);
    guint i;
    for(i = 0; i < n; i++)
    {
        GIBaseInfo *info = get_node(self->info, i);
        const gchar *name = g_base_info_get_name(info);
        GelTypeinfo *node = gel_typeinfo_new(info);
        node->container = self;
        g_hash_table_insert(self->infos, (void* )name, node);
    }
}


static
void gel_typeinfo_register(GelTypeinfo *self)
{
    if(gel_typeinfo_gtypes == NULL)
        gel_typeinfo_gtypes = g_hash_table_new(g_direct_hash, g_direct_equal);
    GType type = g_registered_type_info_get_g_type(self->info);
    g_hash_table_insert(gel_typeinfo_gtypes, GINT_TO_POINTER(type), self);
}


GelTypeinfo* gel_typeinfo_from_gtype(GType type)
{
    GelTypeinfo *info = NULL;
    if(gel_typeinfo_gtypes != NULL)
    {
        info = (GelTypeinfo *)g_hash_table_lookup(
                    gel_typeinfo_gtypes, GINT_TO_POINTER(type));
    }
    return info;
}


GelTypeinfo* gel_typeinfo_new(GIBaseInfo *info)
{
    GelTypeinfo *self = g_slice_new0(GelTypeinfo);
    self->ref_count = 1;
    self->info = info;
    self->infos = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        NULL, (GDestroyNotify)gel_typeinfo_unref);

    if(GI_IS_REGISTERED_TYPE_INFO(info))
        gel_typeinfo_register(self);

    switch(g_base_info_get_type(info))
    {
        case GI_INFO_TYPE_INTERFACE:
        {
            gel_typeinfo_insert_multiple(self,
                g_interface_info_get_n_methods,
                g_interface_info_get_method);

            gel_typeinfo_insert_multiple(self,
                g_interface_info_get_n_constants,
                g_interface_info_get_constant);

            break;
        }

        case GI_INFO_TYPE_OBJECT:
        {
            gel_typeinfo_insert_multiple(self,
                g_object_info_get_n_methods,
                g_object_info_get_method);

            gel_typeinfo_insert_multiple(self,
                g_object_info_get_n_constants,
                g_object_info_get_constant);

            break;
        }

        case GI_INFO_TYPE_STRUCT:
        {
            gel_typeinfo_insert_multiple(self,
                g_struct_info_get_n_methods,
                g_struct_info_get_method);

            gel_typeinfo_insert_multiple(self,
                g_struct_info_get_n_fields,
                g_struct_info_get_field);

            break;
        }

        case GI_INFO_TYPE_FLAGS:
        case GI_INFO_TYPE_ENUM:
        {
            gel_typeinfo_insert_multiple(self,
                g_enum_info_get_n_values,
                g_enum_info_get_value);

            break;
        }

        case GI_INFO_TYPE_UNION:
        {
            gel_typeinfo_insert_multiple(self,
                g_union_info_get_n_methods,
                g_union_info_get_method);

            gel_typeinfo_insert_multiple(self,
                g_union_info_get_n_fields,
                g_union_info_get_field);

            break;
        }

        default:
            break;
    }

    return self;
}


GelTypeinfo* gel_typeinfo_ref(GelTypeinfo *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    g_atomic_int_inc(&self->ref_count);
    return self;
}


void gel_typeinfo_unref(GelTypeinfo *self)
{
    g_return_if_fail(self != NULL);

    if(g_atomic_int_dec_and_test(&self->ref_count))
    {
        g_hash_table_unref(self->infos);
        g_base_info_unref(self->info);
        g_slice_free(GelTypeinfo, self);
    }
}


const gchar* gel_typeinfo_get_name(const GelTypeinfo *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    return g_base_info_get_name(self->info);
}


const GelTypeinfo* gel_typeinfo_lookup(const GelTypeinfo *self,
                                       const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);

    return (GelTypeinfo *)g_hash_table_lookup(self->infos, name);
}

