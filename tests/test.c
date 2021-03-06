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
        gchar *program = argv[0];
        g_print("%s: requires an argument\n", program);
        return 1;
    }

    gchar *content = NULL;
    GError *error = NULL;
    GelParser *parser = NULL;
    GelContext *context = NULL;
    int status = 1;
    gchar *filename = argv[1];

    gsize content_len;
    g_file_get_contents(filename, &content, &content_len, &error);
    if(error != NULL)
        goto finally;
    
    parser = gel_parser_new();
    gel_parser_input_text(parser, content, content_len);

    context = gel_context_new();
    gel_context_define(context, "title", G_TYPE_STRING, "Hello Gtk from Gel");
    gel_context_define_function(context, "make-label", make_label, NULL);
    
    GelParserIter parser_iter;
    gel_parser_iter_init(&parser_iter, parser);
    while(gel_parser_iter_next(&parser_iter, &error))
    {
        GValue *parsed_value = gel_parser_iter_get(&parser_iter);

        gchar *parsed_repr = gel_value_repr(parsed_value);
        g_print("\n%s ?\n", parsed_repr);
        g_free(parsed_repr);

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
                goto finally;
    }

    finally:
    if(error != NULL)
    {
        g_print("Error of domain %s\n", g_quark_to_string(error->domain));
        g_print("%s\n", error->message);
        g_error_free(error);
    }
    else
        status = 0;

    if(content != NULL)
        g_free(content);
    if(parser != NULL)
        gel_parser_free(parser);
    if(context != NULL)
        gel_context_free(context);

    return status;
}

