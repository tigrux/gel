/*
    An example interpreter that uses the API provided by Gel (libgel).
    The goal is to show how simple is to use libgel
    to provide scripting to any program based on glib.
    It's still work in progress.
*/

#include <gel.h>


/* the function make_label will be made available to the script */
void make_label(GClosure *closure, GValue *return_value,
                guint n_param_values, GValue *param_values,
                GelContext *invocation_context, gpointer user_data)
{
    /* get the type of the class 'GtkLabel' (assuming Gtk was required) */
    GType label_t = g_type_from_name("GtkLabel");
    g_value_init(return_value, label_t);

    /* instantiate an object of type 'GtkLabel' */
    GObject *label = g_object_new(label_t, "label", "Label made in C", NULL);
    
    /* the function returns the new object to the caller script */
    g_value_take_object(return_value, label);
}


int main(int argc, char *argv[])
{
    /* initialize the types of glib */
    g_type_init();

    if(argc != 2)
    {
        g_printerr("%s requires an argument\n", argv[0]);
        return 1;
    }

    GError *error = NULL;
    /* parsing a file returns an array of values, unless an error occurs */
    GValueArray *parsed_array = gel_parse_file(argv[1], &error);

    /* if an errors occurred ... */
    if(error != NULL)
    {
        /* ... then print information about the error */
        g_print("There was an error parsing '%s'\n", argv[1]);
        g_print("%s\n", error->message);
        g_error_free(error);
        return 1;
    }

    /* instantiate a context to evaluate the parsed values */
    GelContext *context = gel_context_new();

    /* insert a function to make it available from the script */
    gel_context_insert_function(context, "make-label", make_label, NULL);

    /* insert a string to make it available from the script */
    GValue *title_value = g_new0(GValue, 1);
    g_value_init(title_value, G_TYPE_STRING);
    g_value_set_string(title_value, "Hello Gtk from Gel");
    gel_context_insert(context, "title", title_value);

    /* for each value obtained during the parsing ... */
    for(guint i = 0; i < parsed_array->n_values; i++)
    {
        /* ... print a representation of the value to be evaluated */
        GValue *iter_value = parsed_array->values + i;
        gchar *value_string = gel_value_to_string(iter_value);
        g_print("\n%s ?\n", value_string);
        g_free(value_string);

        GValue result_value = {0};
        /* if the evaluation yields a value ... */
        if(gel_context_eval(context, iter_value, &result_value))
        {
            /* ... then print the value obtained from the evaluation */
            value_string = gel_value_to_string(&result_value);
            g_value_unset(&result_value);
            g_print("= %s\n", value_string);
            g_free(value_string);
        }
    }

    /* free the resources before exit */
    g_value_array_free(parsed_array);
    gel_context_free(context);

    return 0;
}

