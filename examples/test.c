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
    GObject *label = g_object_new(label_t, "label", "Label made in C", NULL);
    g_value_init(return_value, label_t);
    g_value_take_object(return_value, label);
}


int main(int argc, char *argv[])
{
    g_type_init();
    if(argc < 2)
    {
        g_print("%s: requires an argument\n", argv[0]);
        return 1;
    }

    gchar *filename = argv[1];
    gchar *text = NULL;
    gsize text_len = 0;
    GError *error = NULL;
    g_file_get_contents(filename, &text, &text_len, &error);
    if(error != NULL)
    {
        g_print("Error reading '%s'\n", filename);
        g_print("%s\n", error->message);
        g_error_free(error);
        return 1;
    }

    GelParser *parser = gel_parser_new();
    gel_parser_input_text(parser, text, text_len);

    GelArray *parsed_array = gel_parser_get_values(parser, &error);
    if(error != NULL)
    {
        g_print("Error parsing '%s'\n", filename);
        g_print("%s\n", error->message);
        g_error_free(error);
        g_free(text);
        gel_parser_free(parser);
        return 1;
    }

    GelContext *context = gel_context_new();
    gel_context_define(context, "title", G_TYPE_STRING, "Hello Gtk from Gel");
    gel_context_define_function(context, "make-label", make_label, NULL);

    GelArrayIter parsed_iter = {0};
    gel_array_iterator(parsed_array, &parsed_iter);
    while(gel_array_iter_next(&parsed_iter))
    {
        GValue *parsed_value = gel_array_iter_get(&parsed_iter);

        gchar *value_repr = gel_value_repr(parsed_value);
        g_print("\n%s ?\n", value_repr);
        g_free(value_repr);

        GValue evaluated_value = {0};
        if(gel_context_eval(context, parsed_value, &evaluated_value, &error))
        {
            gchar *evaluated_string = gel_value_to_string(&evaluated_value);
            g_print("= %s\n", evaluated_string);
            g_free(evaluated_string);
            g_value_unset(&evaluated_value);
        }
        else
            if(error != NULL)
            {
                g_print("Error evaluating '%s'\n", filename);
                g_print("%s\n", error->message);
                g_error_free(error);
                g_free(text);
                gel_parser_free(parser);
                gel_array_free(parsed_array);
                gel_context_free(context);
                return 1;
            }
    }

    g_free(text);
    gel_parser_free(parser);
    gel_array_free(parsed_array);
    gel_context_free(context);
    return 0;
}

