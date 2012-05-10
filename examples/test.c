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

    const gchar *filename = argv[1];
    gchar *text = NULL;
    gsize text_len = 0;
    GError *error = NULL;

    g_file_get_contents(filename, &text, &text_len, &error);
    if(error != NULL)
    {
        g_print("There was an error reading '%s'\n", filename);
        g_print("%s\n", error->message);
        g_error_free(error);
        return 1;
    }

    /* parsing a file returns an array of values, unless an error occurs */
    GelArray *parsed_array = gel_parse_text(text, text_len, &error);
    if(error != NULL)
    {
        /* ... then print information about the error */
        g_print("There was an error parsing '%s'\n", filename);
        g_print("%s\n", error->message);
        g_error_free(error);
        g_free(text);
        return 1;
    }

    /* instantiate a context to evaluate the parsed values */
    GelContext *context = gel_context_new();

    /* insert a function to make it available from the script */
    gel_context_define_function(context, "make-label", make_label, NULL);

    /* insert a string to make it available from the script */
    GValue *title_value = g_new0(GValue, 1);
    g_value_init(title_value, G_TYPE_STRING);
    g_value_set_string(title_value, "Hello Gtk from Gel");
    gel_context_define(context, "title", title_value);

    /* for each value obtained during the parsing ... */
    for(guint i = 0; i < gel_array_get_n_values(parsed_array); i++)
    {
        GValue *value = gel_array_get_values(parsed_array) + i;

        /* ... print a representation of the value to be evaluated */
        gchar *value_repr = gel_value_repr(value);
        g_print("\n%s ?\n", value_repr);
        g_free(value_repr);

        GValue result_value = {0};
        /* if the evaluation yields a value ... */
        if(gel_context_eval(context, value, &result_value, &error))
        {
            /* ... then print the value obtained from the evaluation */
            gchar *value_string = gel_value_to_string(&result_value);
            g_print("= %s\n", value_string);
            g_free(value_string);
            g_value_unset(&result_value);
        }
        else
        if(error != NULL)
        {
            /* ... then print information about the error */
            g_print("There was an error evaluating '%s'\n", filename);
            g_print("%s\n", error->message);
            g_error_free(error);
            break;
        }
    }

    /* free the resources before exit */
    gel_context_free(context);
    gel_array_free(parsed_array);
    g_free(text);

    return 0;
}

