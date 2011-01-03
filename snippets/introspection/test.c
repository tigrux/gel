#include <girepository.h>
#include <gitypelib.h>

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        g_printerr("Need a typelib name\n");
        return 1;
    }

    g_type_init();
    const gchar *namespace = argv[1];
    const gchar *version = NULL;
    GIRepository *repo = g_irepository_get_default();
    if(argc > 2)
        version = argv[2];
    GTypelib *typelib = g_irepository_require(repo, namespace, version, 0, NULL);

    if(typelib == NULL)
    {
        g_print("Could not get a typelib for %s %s\n",
            namespace, version != NULL ? version : "");
        goto clean;
    }

    guint n = n = g_irepository_get_n_infos (repo, namespace);
    guint i;

    for(i = 0; i < n; i++)
    {
        GIBaseInfo *info = g_irepository_get_info (repo, namespace, i);
        GIInfoType info_type = g_base_info_get_type(info);

        switch(info_type)
        {
            case GI_INFO_TYPE_OBJECT:
            {
                g_print("object = %s\n", g_base_info_get_name(info));
                GIRegisteredTypeInfo *type_info = (GIRegisteredTypeInfo*)info;
                GType type = g_registered_type_info_get_g_type(type_info);
                g_print("\ttypename = %s\n", g_type_name(type));
                guint n_methods = g_object_info_get_n_methods(info);
                guint i;
                for (i = 0; i < n_methods; i++)
                {
                    GIFunctionInfo *finfo = g_object_info_get_method(info, i);
                    g_print("\tmethod %s\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                break;
            }
            case GI_INFO_TYPE_STRUCT:
            {
                g_print("struct = %s\n", g_base_info_get_name(info));
                GIRegisteredTypeInfo *type_info = (GIRegisteredTypeInfo*)info;
                GType type = g_registered_type_info_get_g_type(type_info);
                if(type != G_TYPE_NONE)
                    g_print("\ttypename = %s\n", g_type_name(type));
                guint n_methods = g_struct_info_get_n_methods(info);
                guint i;
                for (i = 0; i < n_methods; i++)
                {
                    GIFunctionInfo *finfo = g_struct_info_get_method(info, i);
                    g_print("\tmethod %s\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                break;
            }
            case GI_INFO_TYPE_BOXED:
            {
                g_print("boxed = %s\n", g_base_info_get_name(info));
                GIRegisteredTypeInfo *type_info = (GIRegisteredTypeInfo*)info;
                GType type = g_registered_type_info_get_g_type(type_info);
                if(type != G_TYPE_NONE)
                    g_print("\ttypename = %s\n", g_type_name(type));
                guint n_methods = g_object_info_get_n_methods(info);
                guint i;
                for (i = 0; i < n_methods; i++)
                {
                    GIFunctionInfo *finfo = g_object_info_get_method(info, i);
                    g_print("\tmethod %s\n",
                        g_base_info_get_name((GIBaseInfo*)finfo));
                    g_base_info_unref((GIBaseInfo*)finfo);
                }
                break;
            }
            default:
                g_print("%s = %s\n",
                    g_info_type_to_string(info_type),
                    g_base_info_get_name(info)
                    );
                break;
        }

        g_base_info_unref(info);
    }

    clean:
    if(typelib != NULL)
        g_typelib_free(typelib);
    return 0;
}

