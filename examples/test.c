/*
    An example interpreter that uses the API provided by gelb (libgel).
    The goal is to show how simple is to use libgel
    to provide scripting to any program based on glib.
    It's still work in progress.
*/

#include <gel.h>


// this function will be made available to the scripts
void get_label_from_c(GClosure *closure, GValue *return_value,
                      guint n_param_values, GValue *param_values,
                      GelContext *invocation_context, gpointer user_data)
{
    // get the type of the class 'GtkLabel' (assuming the script imported Gtk)
    GType label_type = g_type_from_name("GtkLabel");
    g_value_init(return_value, label_type);

    // the property 'label' is set using the argument provided with the function
    GObject *label = g_object_new(label_type, "label", (gchar*)user_data, NULL);
    
    // the function returns the new object to the caller script
    g_value_take_object(return_value, label);
}


int main(int argc, char *argv[])
{
    // initialize the types of glib
    g_type_init();

    if(argc != 2)
    {
        g_printerr("%s requires an argument\n", argv[0]);
        return 1;
    }

    // parsing a file produces an array of values
    GValueArray *array = gel_parse_file(argv[1], NULL);
    if(array == NULL)
    {
        g_print("Could not parse file '%s'\n", argv[1]);
        return 1;
    }

    // intantiate a context to be used to evaluate
    GelContext *context = gel_context_new();

    // insert a function to make it available in the scripts
    gel_context_insert_function(context,
        "get-label-from-c", get_label_from_c, "Label made in C");

    // insert a value of type string to make it available in the scripts
    GValue *title_value = g_new0(GValue, 1);
    g_value_init(title_value, G_TYPE_STRING);
    g_value_set_string(title_value, "Hello Gtk from Gel");
    gel_context_insert(context,
        "title", title_value);

    // for each value obtained during the parsing...
    for(guint i = 0; i < array->n_values; i++)
    {
        // print a representation of the value to be evaluated
        GValue *iter_value = array->values + i;
        gchar *value_string = gel_value_to_string(iter_value);
        g_print("\n%s ?\n", value_string);
        g_free(value_string);

        GValue result_value = {0};
        // if the evaluation yields a value...
        if(gel_context_eval(context, iter_value, &result_value))
        {
            // print the value obtained from the evaluation
            value_string = gel_value_to_string(&result_value);
            g_value_unset(&result_value);
            g_print("= %s\n", value_string);
            g_free(value_string);
        }
    }

    // free the resources before exit
    g_value_array_free(array);
    gel_context_free(context);

    return 0;
}

