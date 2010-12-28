#include <gtk/gtk.h>
#include <gel.h>


void quit(GelContext *context)
{
    gel_context_unref(context);
    gtk_main_quit();
}


void init(GValueArray *array)
{
    GelContext *context = gel_context_new();
    g_signal_connect(G_OBJECT(context), "quit", (GCallback)quit, context);

    const guint last = array->n_values;
    const GValue *const array_values = array->values;
    register guint i;
    for(i = 0; i < last; i++)
    {
        const GValue *const iter_value = array_values + i;
        gchar *value_string = gel_value_to_string(iter_value);
        g_print("%s ?\n", value_string);
        g_free(value_string);

        GValue tmp_value = {0};
        const GValue *value =
            gel_context_eval_value(context, iter_value, &tmp_value);
        if(G_IS_VALUE(value))
        {
            value_string = gel_value_to_string(value);
            g_print("= %s\n\n", value_string);
            g_free(value_string);
        }
        if(G_IS_VALUE(&tmp_value))
            g_value_unset(&tmp_value);
    }
    g_value_array_free(array);
}


int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    volatile GType type;
    type = gtk_window_get_type();
    type = gtk_vbox_get_type();
    type = gtk_hbox_get_type();
    type = gtk_entry_get_type();
    type = gtk_button_get_type();

    if(argc != 2)
    {
        g_printerr("%s requires an argument\n", argv[0]);
        return 1;
    }

    const char *file = argv[1];

    GValueArray *array = gel_parse_file(file, NULL);
    if(array == NULL)
    {
        g_print("Could not parse file '%s'\n", file);
        return 1;
    }

    gtk_init_add((GtkFunction)init, array);
    gtk_main();
    return 0;
}

