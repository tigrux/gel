#include <stdio.h>
#include <gel.h>

int main(int argc, char *argv[])
{
    const gchar *filename = NULL;
    gboolean interactive = FALSE;
    gchar *text = NULL;
    GelParser *parser = NULL;
    GelContext *context = NULL;

    g_type_init();

    parser = gel_parser_new();

    if(argc > 1)
    {
        gsize text_len = 0;
        GError *read_error = NULL;

        filename = argv[1];
        g_file_get_contents(filename, &text, &text_len, &read_error);

        if(read_error != NULL)
        {
            g_print("Error reading '%s'\n", filename);
            g_print("%s\n", read_error->message);
            g_clear_error(&read_error);
            return 1;
        }

        gel_parser_input_text(parser, text, text_len);
    }
    else
    {
        g_print("Entering Gel in interactive mode\n");
        filename = "<stdin>";
        interactive = TRUE;
        gel_parser_input_file(parser, fileno(stdin));
    }
    
    context = gel_context_new();
    gboolean running = TRUE;
    while(running)
    {
        GValue value = {0};
        GError *parse_error = NULL;

        if(interactive)
            g_print("gel> ");

        if(gel_parser_next_value(parser, &value, &parse_error))
        {
            GValue result_value = {0};
            GError *context_error = NULL;

            if(!interactive)
            {
                gchar *value_repr = gel_value_repr(&value);
                g_print("\n%s ?\n", value_repr);
                g_free(value_repr);
            }

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
                g_clear_error(&context_error);
                if(!interactive)
                    running = FALSE;
            }
            g_value_unset(&value);
        }
        else
        if(parse_error != NULL)
        {
            g_print("Error parsing '%s'\n", filename);
            g_print("%s\n", parse_error->message);
            g_clear_error(&parse_error);
            if(!interactive)
                running = FALSE;
        }
        else
            running = FALSE;
    }

    if(text != NULL)
        g_free(text);
    gel_parser_free(parser);
    gel_context_free(context);

    return 0;
}

