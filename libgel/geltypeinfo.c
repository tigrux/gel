#include <config.h>
#include <geltypeinfo.h>
#include <gelvalueprivate.h>


struct _GelTypeInfo
{
    GIBaseInfo *info;
    GelTypeInfo *container;
    GHashTable *infos;
    volatile gint ref_count;
};


#ifndef HAVE_G_INFO_TYPE_TO_STRING

static
const gchar* g_info_type_to_string(GIInfoType type)
{
    switch (type)
    {
        case GI_INFO_TYPE_INVALID:
            return "invalid";
        case GI_INFO_TYPE_FUNCTION:
            return "function";
        case GI_INFO_TYPE_CALLBACK:
            return "callback";
        case GI_INFO_TYPE_STRUCT:
            return "struct";
        case GI_INFO_TYPE_BOXED:
            return "boxed";
        case GI_INFO_TYPE_ENUM:
            return "enum";
        case GI_INFO_TYPE_FLAGS:
            return "flags";
        case GI_INFO_TYPE_OBJECT:
            return "object";
        case GI_INFO_TYPE_INTERFACE:
            return "interface";
        case GI_INFO_TYPE_CONSTANT:
            return "constant";
        case GI_INFO_TYPE_UNION:
            return "union";
        case GI_INFO_TYPE_VALUE:
            return "value";
        case GI_INFO_TYPE_SIGNAL:
            return "signal";
        case GI_INFO_TYPE_VFUNC:
            return "vfunc";
        case GI_INFO_TYPE_PROPERTY:
            return "property";
        case GI_INFO_TYPE_FIELD:
            return "field";
        case GI_INFO_TYPE_ARG:
            return "arg";
        case GI_INFO_TYPE_TYPE:
            return "type";
        case GI_INFO_TYPE_UNRESOLVED:
            return "unresolved";
        default:
            return "unknown";
    }
}

#endif


static
void gel_type_info_to_string_transform(const GValue *source, GValue *dest)
{
    GelTypeInfo *info = (GelTypeInfo *)g_value_get_boxed(source);
    GIBaseInfo *base_info = info->info;

    const gchar *info_namespace = g_base_info_get_namespace(base_info);
    const gchar *type = g_info_type_to_string(g_base_info_get_type(base_info));

    guint n_names = 1;
    GList *name_list = NULL;
    while(info != NULL)
    {
        const gchar *name = g_base_info_get_name(info->info);
        name_list = g_list_prepend(name_list, (void *)name);
        info = info->container;
        n_names++;
    }
    name_list = g_list_prepend(name_list, (void *)info_namespace);

    gchar **name_array = g_new0(gchar*, n_names+1);
    GList *name_iter = name_list;
    for(guint i = 0; i < n_names; i++)
    {
        name_array[i] = (gchar*)(name_iter->data);
        name_iter = g_list_next(name_iter);
    }
    g_list_free(name_list);
    gchar *name = g_strjoinv(".", name_array);
    g_free(name_array);

    g_value_take_string(dest,
        g_strdup_printf("<GelTypeInfo %s %s>", type, name));
    g_free(name);
}


GType gel_type_info_get_type(void)
{
    static volatile gsize once = 0;
    static GType type = G_TYPE_INVALID;
    if(g_once_init_enter(&once))
    {
        type = g_boxed_type_register_static("GelTypeInfo",
                (GBoxedCopyFunc)gel_type_info_ref,
                (GBoxedFreeFunc)gel_type_info_unref);

        g_value_register_transform_func(type, G_TYPE_STRING,
            gel_type_info_to_string_transform);

        g_once_init_leave(&once, 1);
    }

    return type;
}


static
void gel_type_info_insert_multiple(GelTypeInfo *self,
                                   gint (*get_n_nodes)(GIBaseInfo *info),
                                   GIBaseInfo* (*get_node)(GIBaseInfo *info,
                                                           gint n))
{
    guint n = get_n_nodes(self->info);
    for(guint i = 0; i < n; i++)
    {
        GIBaseInfo *info = get_node(self->info, i);
        const gchar *base_info_name = g_base_info_get_name(info);
        gchar *name = g_strdelimit(g_strdup(base_info_name), "_", '-');
        GelTypeInfo *node = gel_type_info_new(info);
        node->container = self;
        g_hash_table_insert(self->infos, name, node);
    }
}


static
GHashTable *gel_type_info_gtypes = NULL;


static
void gel_type_info_register(GelTypeInfo *self)
{
    if(gel_type_info_gtypes == NULL)
        gel_type_info_gtypes = g_hash_table_new(g_direct_hash, g_direct_equal);
    GType type = g_registered_type_info_get_g_type(self->info);
    g_hash_table_insert(gel_type_info_gtypes, GINT_TO_POINTER(type), self);
}


GelTypeInfo* gel_type_info_from_gtype(GType type)
{
    GelTypeInfo *info = NULL;
    if(gel_type_info_gtypes != NULL)
    {
        info = (GelTypeInfo *)g_hash_table_lookup(
                    gel_type_info_gtypes, GINT_TO_POINTER(type));
    }
    return info;
}


#ifndef GI_IS_REGISTERED_TYPE_INFO
#define GI_IS_REGISTERED_TYPE_INFO(info) \
    ((g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_BOXED) || \
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_ENUM) || \
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_FLAGS) ||	\
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_INTERFACE) || \
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_OBJECT) || \
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_STRUCT) || \
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_UNION) || \
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_BOXED))
#endif


GelTypeInfo* gel_type_info_new(GIBaseInfo *info)
{
    GelTypeInfo *self = g_slice_new0(GelTypeInfo);
    self->ref_count = 1;
    self->info = info;
    self->infos = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        (GDestroyNotify)g_free, (GDestroyNotify)gel_type_info_unref);

    if(GI_IS_REGISTERED_TYPE_INFO(info))
        gel_type_info_register(self);

    switch(g_base_info_get_type(info))
    {
        case GI_INFO_TYPE_INTERFACE:
            gel_type_info_insert_multiple(self,
                g_interface_info_get_n_methods,
                g_interface_info_get_method);

            gel_type_info_insert_multiple(self,
                g_interface_info_get_n_constants,
                g_interface_info_get_constant);

            gel_type_info_insert_multiple(self,
                g_interface_info_get_n_properties,
                g_interface_info_get_property);
            break;

        case GI_INFO_TYPE_OBJECT:
            gel_type_info_insert_multiple(self,
                g_object_info_get_n_methods,
                g_object_info_get_method);

            gel_type_info_insert_multiple(self,
                g_object_info_get_n_constants,
                g_object_info_get_constant);

            gel_type_info_insert_multiple(self,
                g_object_info_get_n_properties,
                g_object_info_get_property);
            break;

        case GI_INFO_TYPE_STRUCT:
            gel_type_info_insert_multiple(self,
                g_struct_info_get_n_methods,
                g_struct_info_get_method);
            break;

        case GI_INFO_TYPE_FLAGS:
        case GI_INFO_TYPE_ENUM:
            gel_type_info_insert_multiple(self,
                g_enum_info_get_n_values,
                g_enum_info_get_value);
            break;

        case GI_INFO_TYPE_UNION:
            gel_type_info_insert_multiple(self,
                g_union_info_get_n_methods,
                g_union_info_get_method);
            break;

        default:
            break;
    }

    return self;
}


GelTypeInfo* gel_type_info_ref(GelTypeInfo *self)
{
    g_return_val_if_fail(self != NULL, NULL);

    g_atomic_int_inc(&self->ref_count);
    return self;
}


void gel_type_info_unref(GelTypeInfo *self)
{
    g_return_if_fail(self != NULL);

    if(g_atomic_int_dec_and_test(&self->ref_count))
    {
        g_hash_table_unref(self->infos);
        g_base_info_unref(self->info);
        g_slice_free(GelTypeInfo, self);
    }
}


const GelTypeInfo* gel_type_info_lookup(const GelTypeInfo *self,
                                        const gchar *name)
{
    g_return_val_if_fail(self != NULL, NULL);

    return (GelTypeInfo *)g_hash_table_lookup(self->infos, name);
}


gboolean gel_argument_to_value(const GIArgument *arg, GITypeTag arg_tag,
                           GValue *value)
{
    switch(arg_tag)
    {
        case GI_TYPE_TAG_BOOLEAN:
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, arg->v_boolean);
            return TRUE;
        case GI_TYPE_TAG_INT8:
            g_value_init(value, G_TYPE_LONG);
            g_value_set_long(value, arg->v_int8);
            return TRUE;
        case GI_TYPE_TAG_UINT8:
            g_value_init(value, G_TYPE_LONG);
            g_value_set_long(value, arg->v_uint8);
            return TRUE;
        case GI_TYPE_TAG_INT16:
            g_value_init(value, G_TYPE_LONG);
            g_value_set_long(value, arg->v_int16);
            return TRUE;
        case GI_TYPE_TAG_UINT16:
            g_value_init(value, G_TYPE_LONG);
            g_value_set_long(value, arg->v_uint16);
            return TRUE;
        case GI_TYPE_TAG_INT32:
            g_value_init(value, G_TYPE_LONG);
            g_value_set_long(value, arg->v_int32);
            return TRUE;
        case GI_TYPE_TAG_UINT32:
            g_value_init(value, G_TYPE_LONG);
            g_value_set_long(value, arg->v_uint32);
            return TRUE;
        case GI_TYPE_TAG_INT64:
            g_value_init(value, G_TYPE_INT64);
            g_value_set_int64(value, arg->v_int64);
            return TRUE;
        case GI_TYPE_TAG_UINT64:
            g_value_init(value, G_TYPE_UINT64);
            g_value_set_uint64(value, arg->v_uint64);
            return TRUE;
        case GI_TYPE_TAG_FLOAT:
            g_value_init(value, G_TYPE_DOUBLE);
            g_value_set_double(value, arg->v_float);
            return TRUE;
        case GI_TYPE_TAG_DOUBLE:
            g_value_init(value, G_TYPE_DOUBLE);
            g_value_set_double(value, arg->v_double);
            return TRUE;
        case GI_TYPE_TAG_GTYPE:
            g_value_init(value, G_TYPE_GTYPE);
            g_value_set_gtype(value, arg->v_uint32);
            return TRUE;
        case GI_TYPE_TAG_UTF8:
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, arg->v_pointer);
            return TRUE;
        default:
            return FALSE;
    }
}


gboolean gel_type_info_eval_into_value(const GelTypeInfo *self,
                                       GObject *object,
                                       GValue *return_value)
{
    g_return_val_if_fail(self != NULL, FALSE);

    GIBaseInfo *info = self->info;

    if(GI_IS_REGISTERED_TYPE_INFO(info))
    {
        g_value_init(return_value, G_TYPE_GTYPE);
        GType type = g_registered_type_info_get_g_type(info);
        g_value_set_gtype(return_value, type);
        return TRUE;
    }

    switch(g_base_info_get_type(info))
    {
        case GI_INFO_TYPE_CONSTANT:
        {
            GITypeInfo *arg_info = g_constant_info_get_type(info);
            GITypeTag arg_tag = g_type_info_get_tag(arg_info);
            GIArgument argument = {0};
            g_constant_info_get_value(info, &argument);
            gboolean converted =
                gel_argument_to_value(&argument, arg_tag, return_value);
            g_constant_info_free_value(info, &argument);
            g_base_info_unref(arg_info);
            if(converted)
                return TRUE;
            break;
        }
        case GI_INFO_TYPE_VALUE:
        {
            GType enum_type =
                g_registered_type_info_get_g_type(self->container->info);
            glong enum_value = g_value_info_get_value(info);
            if(G_TYPE_IS_ENUM(enum_type))
            {
                g_value_init(return_value, enum_type);
                g_value_set_enum(return_value, enum_value);
            }
            else
            {
                g_value_init(return_value, G_TYPE_LONG);
                gel_value_set_long(return_value, enum_value);
            }
            return TRUE;
        }
        case GI_INFO_TYPE_PROPERTY:
            if(object != NULL)
            {
                const gchar *name = g_base_info_get_name(info);
                GObjectClass *gclass = G_OBJECT_GET_CLASS(object);
                GParamSpec *spec = g_object_class_find_property(gclass, name);
                if(spec != NULL)
                {
                    g_value_init(return_value, spec->value_type);
                    g_object_get_property(object, name, return_value);
                    return TRUE;
                }
            }
            break;
        default:
            break;
    }

    g_value_init(return_value, GEL_TYPE_TYPEINFO);
    g_value_set_boxed(return_value, self);
    return FALSE;
}

