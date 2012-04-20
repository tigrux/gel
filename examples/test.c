#include <gel.h>


void get_label_from_c(GClosure *closure, GValue *return_value,
                      guint n_param_values, GValue *param_values,
                      GelContext *invocation_context, gpointer user_data)
{
    GType label_type = g_type_from_name("GtkLabel");
    g_value_init(return_value, label_type);

    GObject *label = g_object_new(label_type, "label", (gchar*)user_data, NULL);
    g_value_take_object(return_value, label);
}


int main(int argc, char *argv[])
{
    g_type_init();

    if(argc != 2)
    {
        g_printerr("%s requires an argument\n", argv[0]);
        return 1;
    }

    GValueArray *array = gel_parse_file(argv[1], NULL);
    if(array == NULL)
    {
        g_print("Could not parse file '%s'\n", argv[1]);
        return 1;
    }

    GelContext *context = gel_context_new();

    gel_context_insert_function(context,
        "get-label-from-c", get_label_from_c, "Added from C");

    GValue *window_title_value = g_new0(GValue, 1);
    g_value_init(window_title_value, G_TYPE_STRING);
    g_value_set_string(window_title_value, "Hello Gtk from Gel");
    gel_context_insert(context, "window-title", window_title_value);

    for(guint i = 0; i < array->n_values; i++)
    {
        GValue *iter_value = array->values + i;
        gchar *value_string = gel_value_to_string(iter_value);
        g_print("\n%s ?\n", value_string);
        g_free(value_string);

        GValue result_value = {0};
        if(gel_context_eval(context, iter_value, &result_value))
        {
            value_string = gel_value_to_string(&result_value);
            g_value_unset(&result_value);
            g_print("= %s\n", value_string);
            g_free(value_string);
        }
    }

    g_value_array_free(array);
    gel_context_free(context);
    return 0;
}

