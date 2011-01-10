#include <girepository.h>
#include <gitypelib.h>

void handle_arg(GITypeInfo *atype)
{
    GITypeTag tag = g_type_info_get_tag(atype);
    g_print(": %s", g_type_tag_to_string(tag));
    if(tag == GI_TYPE_TAG_ARRAY)
    {
        GITypeInfo *param = g_type_info_get_param_type(atype, 0);
        g_print(" of %s\n",
            g_type_tag_to_string(
                g_type_info_get_tag(param)));
        g_base_info_unref((GIBaseInfo*)param);
    }
    else
    if(tag == GI_TYPE_TAG_INTERFACE)
    {
        GIBaseInfo *info = g_type_info_get_interface(atype);
        g_print(" for %s\n",
            g_base_info_get_name(info));
        g_base_info_unref((GIBaseInfo*)info);
    }
    else
        g_print("\n");
}

void handle_callable(GIFunctionInfo *cinfo)
{
    GITypeInfo *rtype = g_callable_info_get_return_type(cinfo);
    g_print("\t\tReturns");
    handle_arg(rtype);
    g_base_info_unref((GIBaseInfo*)rtype);

    guint n = g_callable_info_get_n_args(cinfo);
    guint i;
    for (i = 0; i < n; i++)
    {
        GIArgInfo *ainfo = g_callable_info_get_arg(cinfo, i);
        GITypeInfo *atype = g_arg_info_get_type(ainfo);
        g_print("\t\t%s", g_base_info_get_name((GIBaseInfo*)ainfo));
        handle_arg(atype);
        g_base_info_unref((GIBaseInfo*)atype);
        g_base_info_unref((GIBaseInfo*)ainfo);
    }
}


int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        g_printerr("Need a typelib name\n");
        return 1;
    }

    g_type_init();
    const gchar *namespace_ = argv[1];
    const gchar *version = NULL;
    GIRepository *repo = g_irepository_get_default();
    if(argc > 2)
        version = argv[2];
    GTypelib *typelib = g_irepository_require(repo,
        namespace_, version, (GIRepositoryLoadFlags)0, NULL);

    if(typelib == NULL)
    {
        g_print("Could not get a typelib for %s %s\n",
            namespace_, version != NULL ? version : "");
        return 1;
    }

    guint n = g_irepository_get_n_infos (repo, namespace_);
    guint i;

    for(i = 0; i < n; i++)
    {
        GIBaseInfo *info = g_irepository_get_info (repo, namespace_, i);
        GIInfoType info_type = g_base_info_get_type(info);

        g_print("%s %s\n",
            g_info_type_to_string(info_type),
            g_base_info_get_name(info));

        switch(info_type)
        {
            guint i, n;

            case GI_INFO_TYPE_FUNCTION:
                handle_callable(info);
                break;

            case GI_INFO_TYPE_OBJECT:
            {
                GIRegisteredTypeInfo *type_info = (GIRegisteredTypeInfo*)info;
                GType type = g_registered_type_info_get_g_type(type_info);
                g_print("\ttypename = %s\n", g_type_name(type));
                n = g_object_info_get_n_methods(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_object_info_get_method(info, i);
                    g_print("\tmethod %s()\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    handle_callable(finfo);
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                n = g_object_info_get_n_vfuncs(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_object_info_get_vfunc(info, i);
                    g_print("\tvfunc (*%s)()\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    handle_callable(finfo);
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                n = g_object_info_get_n_properties(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *pinfo = g_object_info_get_property(info, i);
                    g_print("\tproperty :%s",
                        g_base_info_get_name((GIBaseInfo*)pinfo));
                    GITypeInfo *ptype = g_property_info_get_type(pinfo);
                    handle_arg(ptype);
                    g_base_info_unref((GIBaseInfo*)ptype);
                    g_base_info_unref((GIBaseInfo*)pinfo);
                }
                n = g_object_info_get_n_signals(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_object_info_get_signal(info, i);
                    g_print("\tsignal ::%s\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    handle_callable(finfo);
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                n = g_object_info_get_n_constants(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_object_info_get_constant(info, i);
                    g_print("\tconstant %s\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                n = g_object_info_get_n_fields(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_object_info_get_field(info, i);
                    g_print("\tfield .%s\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                break;
            }
            case GI_INFO_TYPE_STRUCT:
            {
                GIRegisteredTypeInfo *type_info = (GIRegisteredTypeInfo*)info;
                GType type = g_registered_type_info_get_g_type(type_info);
                if(type != G_TYPE_NONE)
                    g_print("\ttypename = %s\n", g_type_name(type));
                n = g_struct_info_get_n_methods(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_struct_info_get_method(info, i);
                    g_print("\tmethod %s()\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    handle_callable(finfo);
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                n = g_struct_info_get_n_fields(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_struct_info_get_field(info, i);
                    g_print("\tfield .%s\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                break;
            }
            case GI_INFO_TYPE_FLAGS:
            case GI_INFO_TYPE_ENUM:
            {
                GIRegisteredTypeInfo *type_info = (GIRegisteredTypeInfo*)info;
                GType type = g_registered_type_info_get_g_type(type_info);
                if(type != G_TYPE_NONE)
                    g_print("\ttypename = %s\n", g_type_name(type));
                guint n = g_enum_info_get_n_values(info);
                for (i = 0; i < n; i++)
                {
                    GIFunctionInfo *finfo = g_enum_info_get_value(info, i);
                    g_print("\t%s = %li\n",
                        g_base_info_get_name((GIBaseInfo*)finfo),
                        g_value_info_get_value((GIValueInfo*)finfo));
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                break;
            }
            default:
                break;
        }

        g_base_info_unref(info);
    }

    if(typelib != NULL)
        g_typelib_free(typelib);
    return 0;
}

