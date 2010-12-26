#include <gelvaluelist.h>
#include <gelvalue.h>


GList* gel_value_list_copy(const GList *src_list)
{
    GList *list = NULL;
    const register GList *iter;
    for(iter = src_list; iter != NULL; iter = iter->next)
    {
        GValue *iter_value = (GValue*)iter->data;
        GValue *value = gel_value_new_of_type(G_VALUE_TYPE(iter_value));
        g_value_copy(iter_value, value);
        list = g_list_append(list, value);
    }

    return list;
}


void gel_value_list_free(GList *list)
{
    const register GList *iter;
    for(iter = list; iter != NULL; iter = iter->next)
    {
        GValue *iter_value = (GValue*)iter->data;
        gel_value_free(iter_value);
    }
    g_list_free(list);
}


static
void gel_value_list_to_string_transform(const GValue *src_value,
                                        GValue *dest_value)
{
    const GList *list = (GList*)g_value_get_boxed(src_value);
    g_value_take_string(dest_value, gel_value_list_to_string(list));
}


GType gel_value_list_get_type(void)
{
    static volatile gsize gel_value_list_type = 0;
    if(g_once_init_enter (&gel_value_list_type))
    {
        GType type = g_boxed_type_register_static(
            "GList",
            (GBoxedCopyFunc)gel_value_list_copy,
            (GBoxedFreeFunc)gel_value_list_free);

        g_value_register_transform_func(
            type, G_TYPE_STRING,
            gel_value_list_to_string_transform);

        g_once_init_leave (&gel_value_list_type, type);
    }
    return (GType)gel_value_list_type;
}


gchar* gel_value_list_to_string(const GList *list)
{

    GString *buffer_string = g_string_new("[");
    const register GList *iter;
    for(iter = list; iter != NULL; iter = iter->next)
    {
        GValue *iter_value = (GValue*)iter->data;
        GValue string_value = {0};
        g_value_init(&string_value, G_TYPE_STRING);
        if(g_value_transform(iter_value, &string_value))
        {
            g_string_append(buffer_string, g_value_get_string(&string_value));
            if(iter->next != NULL)
                g_string_append_c(buffer_string, ' ');
        }
        g_value_unset(&string_value);
    }
    g_string_append_c(buffer_string, ']');

    return g_string_free(buffer_string, FALSE);
}


GValueArray* gel_value_list_to_array(const GList *list)
{
    guint n_values = g_list_length((GList*)list);
    GValueArray *array = g_value_array_new(n_values);

    const register GList *iter;
    for(iter = list; iter != NULL; iter = iter->next)
    {
        GValue *iter_value = (GValue*)iter->data;
        g_value_array_append(array, iter_value);
    }

    return array;
}

