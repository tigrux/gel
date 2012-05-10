#include <config.h>

#include <geltypeinfo.h>
#include <gelcontextprivate.h>
#include <gelvalueprivate.h>
#include <gelclosureprivate.h>


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
    GelTypeInfo *info = gel_value_get_boxed(source);
    gchar *name = gel_type_info_to_string(info);
    const gchar *type =
        g_info_type_to_string(g_base_info_get_type(info->info));

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
void gel_type_info_insert_multiple(GelTypeInfo *self, GIBaseInfo *base,
                                   gint (*get_n_nodes)(GIBaseInfo *info),
                                   GIBaseInfo* (*get_node)(GIBaseInfo *info,
                                                           gint n))
{
    guint n = get_n_nodes(base);
    for(guint i = 0; i < n; i++)
    {
        GIBaseInfo *info = get_node(base, i);
        const gchar *base_name = g_base_info_get_name(info);
        gchar *name = g_strdelimit(g_strdup(base_name), "_", '-');
        GelTypeInfo *node = gel_type_info_new(info);
        node->container = self;
        g_hash_table_insert(self->infos, name, node);
    }
}


static
GHashTable *gtypes_HASH = NULL;


static
void gel_type_info_register(GelTypeInfo *self)
{
    if(gtypes_HASH == NULL)
        gtypes_HASH = g_hash_table_new(g_direct_hash, g_direct_equal);

    GType type = g_registered_type_info_get_g_type(self->info);
    g_hash_table_insert(gtypes_HASH, GINT_TO_POINTER(type), self);
}


GelTypeInfo* gel_type_info_from_gtype(GType type)
{
    GelTypeInfo *info = NULL;
    if(gtypes_HASH != NULL)
        info = g_hash_table_lookup(gtypes_HASH, GINT_TO_POINTER(type));

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

#ifndef GI_IS_OBJECT_INFO
#define GI_IS_OBJECT_INFO(info) \
     (g_base_info_get_type((GIBaseInfo*)info) == GI_INFO_TYPE_OBJECT)
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
        case GI_INFO_TYPE_OBJECT:
            gel_type_info_insert_multiple(self, info,
                g_object_info_get_n_methods,
                g_object_info_get_method);

            gel_type_info_insert_multiple(self, info,
                g_object_info_get_n_constants,
                g_object_info_get_constant);

            gel_type_info_insert_multiple(self, info,
                g_object_info_get_n_properties,
                g_object_info_get_property);

            guint n_ifaces = g_object_info_get_n_interfaces(info);
            for(guint i = 0; i < n_ifaces; i++)
            {
                GIInterfaceInfo *iface_info =
                    g_object_info_get_interface(info, i);

                gel_type_info_insert_multiple(self, iface_info,
                    g_interface_info_get_n_methods,
                    g_interface_info_get_method);

                gel_type_info_insert_multiple(self, iface_info,
                    g_interface_info_get_n_properties,
                    g_interface_info_get_property);

                gel_type_info_insert_multiple(self, iface_info,
                    g_interface_info_get_n_constants,
                    g_interface_info_get_constant);

                g_base_info_unref(iface_info);
            }

            break;

        case GI_INFO_TYPE_STRUCT:
            gel_type_info_insert_multiple(self, info,
                g_struct_info_get_n_methods,
                g_struct_info_get_method);
            break;

        case GI_INFO_TYPE_FLAGS:
        case GI_INFO_TYPE_ENUM:
            gel_type_info_insert_multiple(self, info,
                g_enum_info_get_n_values,
                g_enum_info_get_value);
            break;

        case GI_INFO_TYPE_UNION:
            gel_type_info_insert_multiple(self, info,
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

    const GelTypeInfo *container_info = self;
    const GelTypeInfo *child_info = NULL;

    while(container_info != NULL)
    {
        child_info = g_hash_table_lookup(container_info->infos, name);
        if(child_info != NULL)
            break;

        GIBaseInfo *base_info = container_info->info;
        if(GI_IS_OBJECT_INFO(base_info))
        {
            GIObjectInfo *parent_info = g_object_info_get_parent(base_info);
            if(parent_info == NULL)
                break;

            GType parent_type = g_registered_type_info_get_g_type(parent_info);
            container_info = gel_type_info_from_gtype(parent_type);
            g_base_info_unref(parent_info);
        }
        else
            break;
    }

    return child_info;
}


gchar* gel_type_info_to_string(const GelTypeInfo *self)
{
    GIBaseInfo *base_info = self->info;

    const gchar *info_namespace = g_base_info_get_namespace(base_info);

    guint n_names = 1;
    GList *name_list = NULL;
    const GelTypeInfo *info = self;

    while(info != NULL)
    {
        const gchar *name = g_base_info_get_name(info->info);
        name_list = g_list_prepend(name_list, (void *)name);
        info = info->container;
        n_names++;
    }
    name_list = g_list_prepend(name_list, (void *)info_namespace);

    gchar **name_array = g_new0(gchar *, n_names + 1);
    GList *name_iter = name_list;

    for(guint i = 0; i < n_names; i++)
    {
        name_array[i] = name_iter->data;
        name_iter = g_list_next(name_iter);
    }

    g_list_free(name_list);
    gchar *name = g_strjoinv(".", name_array);
    g_free(name_array);

    return name;
}


static
gboolean gel_argument_to_value(const GArgument *arg, GITypeInfo *info,
                               GITransfer transfer, GValue *value)
{
    GITypeTag tag = g_type_info_get_tag(info);
    switch(tag)
    {
        case GI_TYPE_TAG_BOOLEAN:
            g_value_init(value, G_TYPE_BOOLEAN);
            gel_value_set_boolean(value, arg->v_boolean);
            return TRUE;
        case GI_TYPE_TAG_INT8:
            g_value_init(value, G_TYPE_INT64);
            gel_value_set_int64(value, arg->v_int8);
            return TRUE;
        case GI_TYPE_TAG_UINT8:
            g_value_init(value, G_TYPE_INT64);
            gel_value_set_int64(value, arg->v_uint8);
            return TRUE;
        case GI_TYPE_TAG_INT16:
            g_value_init(value, G_TYPE_INT64);
            gel_value_set_int64(value, arg->v_int16);
            return TRUE;
        case GI_TYPE_TAG_UINT16:
            g_value_init(value, G_TYPE_INT64);
            gel_value_set_int64(value, arg->v_uint16);
            return TRUE;
        case GI_TYPE_TAG_INT32:
            g_value_init(value, G_TYPE_INT64);
            gel_value_set_int64(value, arg->v_int32);
            return TRUE;
        case GI_TYPE_TAG_UINT32:
            g_value_init(value, G_TYPE_INT64);
            gel_value_set_int64(value, arg->v_uint32);
            return TRUE;
        case GI_TYPE_TAG_INT64:
            g_value_init(value, G_TYPE_INT64);
            gel_value_set_int64(value, arg->v_int64);
            return TRUE;
        case GI_TYPE_TAG_UINT64:
            g_value_init(value, G_TYPE_UINT64);
            gel_value_set_int64(value, arg->v_uint64);
            return TRUE;
        case GI_TYPE_TAG_FLOAT:
            g_value_init(value, G_TYPE_DOUBLE);
            gel_value_set_double(value, arg->v_float);
            return TRUE;
        case GI_TYPE_TAG_DOUBLE:
            g_value_init(value, G_TYPE_DOUBLE);
            gel_value_set_double(value, arg->v_double);
            return TRUE;
        case GI_TYPE_TAG_GTYPE:
            g_value_init(value, G_TYPE_GTYPE);
            gel_value_set_gtype(value, arg->v_uint32);
            return TRUE;
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_UTF8:
            g_value_init(value, G_TYPE_STRING);
            if(transfer == GI_TRANSFER_EVERYTHING)
                g_value_take_string(value, arg->v_pointer);
            else
                g_value_set_string(value, arg->v_pointer);
            return TRUE;
        case GI_TYPE_TAG_INTERFACE:
        {
            gboolean result = FALSE;
            GIBaseInfo *base = g_type_info_get_interface(info);
            GIInfoType type = g_base_info_get_type(base);

            switch(type)
            {
                case GI_INFO_TYPE_OBJECT:
                    g_value_init(value,
                        g_registered_type_info_get_g_type(base));
                    if(transfer == GI_TRANSFER_EVERYTHING)
                        g_value_take_object(value, arg->v_pointer);
                    else
                        g_value_set_object(value, arg->v_pointer);
                    result = TRUE;
                    break;
                case GI_INFO_TYPE_STRUCT:
                    g_value_init(value,
                        g_registered_type_info_get_g_type(base));
                    if(transfer == GI_TRANSFER_EVERYTHING)
                        g_value_take_boxed(value, arg->v_pointer);
                    else
                        g_value_set_boxed(value, arg->v_pointer);
                    result = TRUE;
                    break;
                default:
                    break;
            }
            g_base_info_unref(base);
            return result;
        }
        default:
            return FALSE;
    }
}


static
gboolean gel_type_info_registered_type_to_value(const GelTypeInfo *self,
                                                GValue *return_value)
{
    gboolean result = FALSE;
    GIBaseInfo *info = self->info;

    if(GI_IS_REGISTERED_TYPE_INFO(info))
    {
        g_value_init(return_value, G_TYPE_GTYPE);
        GType type = g_registered_type_info_get_g_type(info);
        g_value_set_gtype(return_value, type);
        result = TRUE;
    }

    return result;
}


static
gboolean gel_type_info_constant_to_value(const GelTypeInfo *self,
                                         GValue *return_value)
{
    GIBaseInfo *info = self->info;
    GITypeInfo *arg_info = g_constant_info_get_type(info);

    GArgument argument = {0};
    g_constant_info_get_value(info, &argument);

    gboolean converted =
        gel_argument_to_value(&argument,
            arg_info, GI_TRANSFER_NOTHING, return_value);

    #if HAVE_G_CONSTANT_INFO_FREE_VALUE
    g_constant_info_free_value(info, &argument);
    #endif
    g_base_info_unref(arg_info);

    return converted;
}


static
gboolean gel_type_info_enum_to_value(const GelTypeInfo *self,
                                     GValue *return_value)
{
    GType enum_type =
        g_registered_type_info_get_g_type(self->container->info);
    GIBaseInfo *info = self->info;
    glong enum_value = g_value_info_get_value(info);

    if(G_TYPE_IS_ENUM(enum_type))
    {
        g_value_init(return_value, enum_type);
        gel_value_set_enum(return_value, enum_value);
    }
    else
    {
        g_value_init(return_value, G_TYPE_INT64);
        gel_value_set_int64(return_value, enum_value);
    }

    return TRUE;
}


static
gboolean gel_type_info_property_to_value(const GelTypeInfo *self,
                                         GObject *object, GValue *return_value)
{
    GIBaseInfo *info = self->info;
    const gchar *name = g_base_info_get_name(info);
    GObjectClass *gclass = G_OBJECT_GET_CLASS(object);
    GParamSpec *spec = g_object_class_find_property(gclass, name);
    gboolean result = FALSE;

    if(spec != NULL)
    {
        g_value_init(return_value, spec->value_type);
        g_object_get_property(object, name, return_value);
        result = TRUE;
    }

    return result;
}


void gel_type_info_closure_marshal(GClosure *gclosure,
                                   GValue *return_value,
                                   guint n_values, const GValue *values,
                                   GelContext *context)
{
    GelIntrospectionClosure *closure = (GelIntrospectionClosure *)gclosure;
    const GelTypeInfo *info = gel_introspection_closure_get_info(closure);
    GObject *object = gel_introspection_closure_get_instance(closure);
    GIBaseInfo *function_info = info->info;

    guint n_args = g_callable_info_get_n_args(function_info);
    GIArgInfo **infos = g_new0(GIArgInfo *, n_args);
    GITypeInfo **types = g_new0(GITypeInfo *, n_args);
    gboolean *indirect_args = g_new0(gboolean, n_args);
    GArgument *inputs = g_new0(GArgument, n_args + 1);
    GArgument *outputs = g_new0(GArgument, n_args);

    guint n_inputs = 0;
    guint n_outputs = 0;

    if(g_function_info_get_flags(function_info) & GI_FUNCTION_IS_METHOD)
    {
        inputs[0].v_pointer = object;
        n_inputs++;
    }

    for(guint i = 0; i < n_args; i++)
    {
        GIArgInfo *info = g_callable_info_get_arg(function_info, i);
        GITypeInfo *type = g_arg_info_get_type(info);

        switch(g_type_info_get_tag(type))
        {
            case GI_TYPE_TAG_ARRAY:
            {
                gint length_index = g_type_info_get_array_length(type);
                if(length_index != -1)
                    indirect_args[length_index] = TRUE;
                break;
            }
            case GI_TYPE_TAG_INTERFACE:
            {
                GIBaseInfo *iface_info = g_type_info_get_interface(type);
                GIInfoType iface_type = g_base_info_get_type(iface_info);

                switch(iface_type)
                {
                    case GI_INFO_TYPE_CALLBACK:
                        indirect_args[g_arg_info_get_closure(info)] = TRUE;
                        indirect_args[g_arg_info_get_destroy(info)] = TRUE;
                        break;
                    default:
                        break;
                }

                g_base_info_unref(iface_info);
                break;
            }
            default:
                break;
        }

        infos[i] = info;
        types[i] = type;
    }

    GList *tmp_list = NULL;

    for(guint i = 0; i < n_args; i++)
    {
        GITypeInfo *type = types[i];
        gboolean is_input = FALSE;
        gboolean is_output = FALSE;

        GIDirection dir = g_arg_info_get_direction(infos[i]);
        switch(dir)
        {
            case GI_DIRECTION_IN:
                is_input = TRUE;
                break;
            case GI_DIRECTION_OUT:
                is_output = TRUE;
                break;
            case GI_DIRECTION_INOUT:
                is_input = TRUE;
                is_output = TRUE;
                break;
        }

        if(!indirect_args[i])
        {
            GITypeTag tag = g_type_info_get_tag(type);

            switch(tag)
            {
                case GI_TYPE_TAG_BOOLEAN:
                {
                    gboolean number = FALSE;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "B*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_boolean = number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_INT8:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_int8 = (gint8)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_UINT8:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_uint8 = (guint8)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_INT16:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_int16 = (gint16)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_UINT16:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_uint16 = (guint16)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_INT32:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_int32 = (gint32)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_UINT32:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_uint32 = (guint32)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_INT64:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_int8 = number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_UINT64:
                {
                    gint64 number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "I*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_uint64 = (guint64)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_FLOAT:
                {
                    gdouble number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "F*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_float = (gfloat)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_DOUBLE:
                {
                    gdouble number = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "F*", &number))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &number;
                                outputs[n_outputs].v_pointer = &number;
                            }
                            else
                                inputs[n_inputs].v_double = (gdouble)number;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_GTYPE:
                {
                    GType type = G_TYPE_INVALID;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "G*", &type))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &type;
                                outputs[n_outputs].v_pointer = &type;
                            }
                            else
                                inputs[n_inputs].v_size = type;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_UTF8:
                {
                    gchar *string = 0;
                    if(gel_context_eval_params(context, __FUNCTION__,
                        &n_values, &values, &tmp_list, "S*", &string))
                    {
                        if(is_input)
                        {
                            if(is_output)
                            {
                                inputs[n_inputs].v_pointer = &string;
                                outputs[n_outputs].v_pointer = &string;
                            }
                            else
                                inputs[n_inputs].v_pointer = string;
                        }
                    }
                    else
                        goto end;
                    break;
                }
                case GI_TYPE_TAG_INTERFACE:
                {
                    GIBaseInfo *iface_info = g_type_info_get_interface(type);
                    GIInfoType iface_type = g_base_info_get_type(iface_info);

                    switch(iface_type)
                    {
                        case GI_INFO_TYPE_OBJECT:
                        {
                            GObject *object;
                            if(gel_context_eval_params(context, __FUNCTION__,
                                &n_values, &values, &tmp_list, "O*", &object))
                            {
                                if(is_input)
                                {
                                    if(is_output)
                                    {
                                        inputs[n_inputs].v_pointer = &object;
                                        outputs[n_outputs].v_pointer = &object;
                                    }
                                    else
                                        inputs[n_inputs].v_pointer = object;
                                }
                            }
                            else
                                goto end;
                            break;
                        }
                        case GI_INFO_TYPE_BOXED:
                        {
                            void *boxed;
                            if(gel_context_eval_params(context, __FUNCTION__,
                                &n_values, &values, &tmp_list, "X*", &boxed))
                            {
                                if(is_input)
                                {
                                    if(is_output)
                                    {
                                        inputs[n_inputs].v_pointer = &boxed;
                                        outputs[n_outputs].v_pointer = &boxed;
                                    }
                                    else
                                        inputs[n_inputs].v_pointer = boxed;
                                }
                            }
                            else
                                goto end;
                            break;
                        }
                        case GI_INFO_TYPE_ENUM:
                        {
                            gint64 number;
                            if(gel_context_eval_params(context, __FUNCTION__,
                                &n_values, &values, &tmp_list, "I*", &number))
                            {
                                if(is_input)
                                {
                                    if(is_output)
                                    {
                                        inputs[n_inputs].v_pointer = &number;
                                        outputs[n_outputs].v_pointer = &number;
                                    }
                                    else
                                        inputs[n_inputs].v_int64 = number;
                                }
                            }
                            else
                                goto end;
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if(is_input)
            n_inputs++;
        if(is_output)
            n_outputs++;
    }

    GArgument return_arg = {0};
    g_function_info_invoke(function_info,
        inputs, n_inputs,
        outputs, n_outputs,
        &return_arg, FALSE);

    GITypeInfo *return_info = g_callable_info_get_return_type(function_info);
    GITransfer transfer = g_callable_info_get_caller_owns(function_info);
    gel_argument_to_value(&return_arg, return_info, transfer, return_value);
    g_base_info_unref(return_info);

    end:
    for(guint i = 0; i < n_args; i++)
    {
        g_base_info_unref(types[i]);
        g_base_info_unref(infos[i]);
    }

    gel_value_list_free(tmp_list);
    g_free(outputs);
    g_free(inputs);
    g_free(indirect_args);
    g_free(types);
    g_free(infos);
}


static
gboolean gel_type_info_function_to_value(const GelTypeInfo *self,
                                         void *instance, GValue *return_value)
{
    GClosure *closure = gel_closure_new_introspection(self, instance);

    g_value_init(return_value, G_TYPE_CLOSURE);
    g_value_take_boxed(return_value, closure);

    return TRUE;
}


gboolean gel_type_info_to_value(const GelTypeInfo *self,
                                void *instance, GValue *return_value)
{
    g_return_val_if_fail(self != NULL, FALSE);

    switch(g_base_info_get_type(self->info))
    {
        case GI_INFO_TYPE_CONSTANT:
            if(gel_type_info_constant_to_value(self, return_value))
                return TRUE;
            break;
        case GI_INFO_TYPE_VALUE:
            if(gel_type_info_enum_to_value(self, return_value))
                return TRUE;
            break;
        case GI_INFO_TYPE_PROPERTY:
            if(instance != NULL)
                if(gel_type_info_property_to_value(self, instance, return_value))
                    return TRUE;
            break;
        case GI_INFO_TYPE_FUNCTION:
            if(gel_type_info_function_to_value(self, instance, return_value))
                return TRUE;
            break;
        default:
            if(gel_type_info_registered_type_to_value(self, return_value))
                return TRUE;
            break;
    }

    g_value_init(return_value, GEL_TYPE_TYPEINFO);
    g_value_set_boxed(return_value, self);

    return FALSE;
}

