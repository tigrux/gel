#include <gelparser.h>

#define ARRAY_N_PREALLOCATED 8


/**
 * SECTION:gelparser
 * @short_description: Functions to convert input into data ready to be evaluated
 * @title: GelParser
 * @include: gel.h
 *
 * Functions to convert input (plain text or files) into #GValueArray.
 * The resulting #GValueArray can be used to feed a #GelContext.
 */


static
GValueArray* gel_parse_strings(const char *first, ...)
{
    register GValueArray *array = g_value_array_new(ARRAY_N_PREALLOCATED);
    va_list vl;

    register const char *arg;
    va_start(vl, first);
    for(arg = first; arg != NULL; arg = va_arg(vl, const char*))
    {
        GValue value = {0};
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_string(&value, arg);
        g_value_array_append(array, &value);
        g_value_unset(&value);
    }
    va_end(vl);

    return array;
}


static
const gchar *scanner_errors[] = {
    "Unknown error",
    "Unexpected end of file",
    "Unterminated string constant",
    "Unterminated comment",
    "Non-digit character in a number",
    "Digit beyond radix in a number",
    "Non-decimal floating point number",
    "Malformed floating point number"
};


static
GValueArray* gel_parse_scanner(GScanner *scanner)
{
    register GValueArray *array = g_value_array_new(ARRAY_N_PREALLOCATED);
    register gint sign = 1;

    register gboolean parsing = TRUE;
    while(parsing)
    {
        GValue value = {0};
        register guint token = g_scanner_peek_next_token(scanner);

        switch(token)
        {
            case G_TOKEN_IDENTIFIER:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_string(&value, scanner->value.v_identifier);
                break;
            case G_TOKEN_FLOAT:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_DOUBLE);
                g_value_set_double(&value, scanner->value.v_float * sign);
                sign = 1;
                break;
            case G_TOKEN_INT:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_LONG);
                g_value_set_long(&value, scanner->value.v_int * sign);
                sign = 1;
                break;
            case G_TOKEN_LEFT_PAREN:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_VALUE_ARRAY);
                g_value_take_boxed(&value, gel_parse_scanner(scanner));
                break;
            case G_TOKEN_RIGHT_PAREN:
                g_scanner_get_next_token(scanner);
            case G_TOKEN_EOF:
                parsing = FALSE;
                break;
            case G_TOKEN_STRING:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_VALUE_ARRAY);
                g_value_take_boxed(&value,
                    gel_parse_strings("quote", scanner->value.v_string, NULL));
                break;
            case '=':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                token = g_scanner_peek_next_token(scanner);
                if(token == '=')
                {
                    g_scanner_get_next_token(scanner);
                    g_value_set_static_string(&value, "eq");
                }
                else
                    g_value_set_static_string(&value, "set");
                break;
            case '+':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_static_string(&value, "add");
                break;
            case '*':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_static_string(&value, "mul");
                break;
            case '/':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_static_string(&value, "div");
                break;
            case '%':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_static_string(&value, "mod");
                break;
            case '&':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_static_string(&value, "and");
                break;
            case '|':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_static_string(&value, "or");
                break;
            case '<':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                token = g_scanner_peek_next_token(scanner);
                if(token == '=')
                {
                    g_scanner_get_next_token(scanner);
                    g_value_set_static_string(&value, "le");
                }
                else
                    g_value_set_static_string(&value, "lt");
                break;
            case '>':
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                token = g_scanner_peek_next_token(scanner);
                if(token == '=')
                {
                    g_scanner_get_next_token(scanner);
                    g_value_set_static_string(&value, "ge");
                }
                else
                    g_value_set_static_string(&value, "gt");
                break;
            case '-':
                g_scanner_get_next_token(scanner);
                token = g_scanner_peek_next_token(scanner);
                if(token == G_TOKEN_INT || token == G_TOKEN_FLOAT)
                    sign = -sign;
                else
                {
                    g_value_init(&value, G_TYPE_STRING);
                    g_value_set_static_string(&value, "sub");
                }
                break;
            case G_TOKEN_ERROR:
                g_scanner_get_next_token(scanner);
                g_error("%s at line %u, char %u",
                    scanner_errors[scanner->value.v_error],
                    scanner->line, scanner->position);
                break;
            default:
                g_error("Unknown token (%d) '%c'", token, token);
                g_scanner_get_next_token(scanner);
        }

        if(G_IS_VALUE(&value))
        {
            g_value_array_append(array, &value);
            g_value_unset(&value);
        }
    }

    return array;
}


/**
 * gel_parse_file:
 * @file: path to a file
 * @error: #GError to be filled if an error happens.
 *
 * Reads the content of @file and then pass the content to #gel_parse_string
 *
 * Returns: A #GValueArray with the parsed value literals
 */
GValueArray* gel_parse_file(const gchar *file, GError **error)
{
    gchar *content;
    gsize content_len;
    if(!g_file_get_contents(file, &content, &content_len, error))
        return NULL;

    register GValueArray *array = gel_parse_text(content, content_len);
    g_free(content);
    return array;
}


/**
 * gel_parse_text:
 * @text: text to parse
 * @text_len: length of the content to parse, or -1 if it is zero terminated.
 *
 * Uses a #GScanner to parse @content, operators are replaced with their
 * corresponding functions (add for +, mul for *, etc). Characters [ ] are
 * used to build array literals. Integers are considered #glong literals,
 * strings are #gchararray literals and floats are #gdouble literals.
 *
 * #gel_context_eval_value considers array literals as closure's invokes.
 *
 * Returns: A #GValueArray with the parsed value literals.
 */
GValueArray* gel_parse_text(const gchar *text, guint text_len)
{
    register GScanner *scanner = g_scanner_new(NULL);

    scanner->config->cset_identifier_nth =
        (gchar*)G_CSET_a_2_z G_CSET_A_2_Z "_-0123456789";
    scanner->config->scan_identifier_1char = TRUE;
    g_scanner_input_text(scanner, text, text_len);

    register GValueArray *array = gel_parse_scanner(scanner);
    g_scanner_destroy(scanner);

    return array;
}

