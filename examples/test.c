#include <gtk/gtk.h>
#include <gel.h>


GelContext *context;
GValueArray *array;


void demo_quit(GClosure *closure, GValue *return_value,
               guint n_param_values, GValue *param_values,
               GelContext *invocation_context, gpointer user_data)
{
    if(return_value != NULL)
    {
        g_value_init(return_value, G_TYPE_STRING);
        g_value_set_static_string(return_value, "Bye!");
    }
    gtk_main_quit();
}


void demo_run(void)
{
    guint n_values = array->n_values;
    const GValue *array_values = array->values;

    guint i;
    for(i = 0; i < n_values; i++)
    {
        const GValue *iter_value = array_values + i;
        gchar *value_string = gel_value_to_string(iter_value);
        g_print("\n%s ?\n", value_string);
        g_free(value_string);

        GValue result_value = {0};
        if(gel_context_eval(context, iter_value, &result_value))
        {
            value_string = gel_value_to_string(&result_value);
            g_value_unset(&result_value);
            g_print("= %s\n\n", value_string);
            g_free(value_string);
        }
    }
    g_value_array_free(array);
}


int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    if(argc != 2)
    {
        g_printerr("%s requires an argument\n", argv[0]);
        return 1;
    }

    array = gel_parse_file(argv[1], NULL);
    if(array == NULL)
    {
        g_print("Could not parse file '%s'\n", argv[1]);
        return 1;
    }

    context = gel_context_new();
    gel_context_insert_function(context,
        "quit", demo_quit, NULL);
    gel_context_insert_object(context,
        "label", (GObject*)gtk_label_new("Added from C"));

    gtk_init_add((GtkFunction)demo_run, NULL);
    gtk_main();

    gel_context_free(context);
    return 0;
}

