#include <stdio.h>
#include <gel.h>

int main(int argc, char *argv[])
{
    const gchar *filename = NULL;
    gboolean interactive = FALSE;
    gboolean active = FALSE;
    gchar *text = NULL;

    g_type_init();
    GelParser *parser = gel_parser_new();
    GelContext *context = gel_context_new();

    if(argc > 1)
    {
        gsize text_len = 0;
        GError *read_error = NULL;

        filename = argv[1];
        if(g_file_get_contents(filename, &text, &text_len, &read_error))
        {
            gel_parser_input_text(parser, text, text_len);
            active = TRUE;
        }
        else
        {
            g_print("Error reading '%s'\n", filename);
            g_print("%s\n", read_error->message);
            g_error_free(read_error);
        }
    }
    else
    {
        g_print("Entering Gel in interactive mode\n");
        gel_parser_input_file(parser, fileno(stdin));
        filename = "<stdin>";
        interactive = TRUE;
        active = TRUE;
    }

    while(active)
    {
        if(interactive)
            g_print("gel> ");

        GValue value = {0};
        GError *parse_error = NULL;
        if(gel_parser_next_value(parser, &value, &parse_error))
        {
            if(!interactive)
            {
                gchar *value_repr = gel_value_repr(&value);
                g_print("\n%s ?\n", value_repr);
                g_free(value_repr);
            }

            GValue result_value = {0};
            GError *context_error = NULL;
            if(gel_context_eval(context, &value, &result_value, &context_error))
            {
                gchar *value_string = gel_value_to_string(&result_value);
                g_print("= %s\n", value_string);
                g_free(value_string);
                g_value_unset(&result_value);
            }
            else
                if(context_error != NULL)
                {
                    g_print("Error evaluating '%s'\n", filename);
                    g_print("%s\n", context_error->message);
                    g_error_free(context_error);
                    if(!interactive)
                        active = FALSE;
                }

            g_value_unset(&value);
        }
        else
            if(parse_error != NULL)
            {
                g_print("Error parsing '%s'\n", filename);
                g_print("%s\n", parse_error->message);
                g_error_free(parse_error);
                if(!interactive)
                    active = FALSE;
            }
            else
                active = FALSE;
    }

    if(text != NULL)
        g_free(text);
    gel_parser_free(parser);
    gel_context_free(context);

    return 0;
}

