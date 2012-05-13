/*
    An example interpreter that uses the API provided by Gel (libgel).
    The goal is to show how simple is to use libgel
    to provide scripting to any program based on glib.
    It's still work in progress.
*/

#include <stdio.h>
#include <gel.h>

/* this function will be made available to the script */
void make_label(GClosure *closure, GValue *return_value,
                guint n_param_values, GValue *param_values,
                GelContext *invocation_context, gpointer user_data)
{
    GType label_t = g_type_from_name("GtkLabel");
    g_value_init(return_value, label_t);

    GObject *label = g_object_new(label_t, "label", "Label made in C", NULL);
    g_value_take_object(return_value, label);
}


int main(int argc, char *argv[])
{
    int status = 1;
    const gchar *filename = argv[1];
    gchar *text = NULL;
    gsize text_len = 0;
    GError *error = NULL;
    GelParser *parser = NULL;
    GelArray *array = NULL;
    GelContext *context = NULL;

    g_type_init();

    if(argc < 2)
    {
        g_print("%s: requires an argument\n", argv[0]);
        return 1;
    }

    if(g_file_get_contents(filename, &text, &text_len, &error))
    {
        parser = gel_parser_new();
        gel_parser_input_text(parser, text, text_len);
    }
    else
        if(error != NULL)
        {
            g_print("Error reading '%s'\n", filename);
            g_print("%s\n", error->message);
            goto free_and_exit;
        }

    array = gel_parser_get_values(parser, &error);
    if(error != NULL)
    {
        g_print("Error parsing '%s'\n", filename);
        g_print("%s\n", error->message);
        goto free_and_exit;
    }

    context = gel_context_new();
    /* These values are defined just as a demonstration */
    gel_context_define(context, "title", G_TYPE_STRING, "Hello Gtk from Gel");
    gel_context_define_function(context, "make-label", make_label, NULL);

    const guint n_values = gel_array_get_n_values(array);
    const GValue *values = gel_array_get_values(array);

    for(guint i = 0; i < n_values; i++)
    {
        const GValue *value = values + i;
        GValue result_value = {0};

        gchar *value_repr = gel_value_repr(value);
        g_print("\n%s ?\n", value_repr);
        g_free(value_repr);

        if(gel_context_eval(context, value, &result_value, &error))
        {
            gchar *value_string = gel_value_to_string(&result_value);
            g_print("= %s\n", value_string);
            g_free(value_string);
            g_value_unset(&result_value);
        }
        else
            if(error != NULL)
            {
                g_print("Error evaluating '%s'\n", filename);
                g_print("%s\n", error->message);
                goto free_and_exit;
            }
    }
    status = 0;

    free_and_exit:
    if(text != NULL)
        g_free(text);
    if(error != NULL)
        g_error_free(error);
    if(array != NULL)
        gel_array_free(array);
    if(parser != NULL)
        gel_parser_free(parser);
    if(context != NULL)
        gel_context_free(context);

    return status;
}

