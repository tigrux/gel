#include <string.h>

#include <gelparser.h>
#include <gelsymbol.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>

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


GQuark gel_parse_error_quark(void)
{
    return g_quark_from_static_string("gel-parse-error");
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
GValueArray* gel_parse_scanner(GScanner *scanner, guint line, guint pos,
                               gchar delim, GError **error)
{
    GValueArray *array = g_value_array_new(ARRAY_N_PREALLOCATED);

    const gchar *pre_symbol = NULL;
    switch(delim)
    {
        case '[':
            pre_symbol = "array";
            break;
         case '{':
            pre_symbol = "hash";
            break;
        default:
            break;
    }

    if(pre_symbol != NULL)
    {
        const GValue *pre_value = gel_value_lookup_predefined(pre_symbol);
        if(pre_value != NULL)
            g_value_array_append(array, pre_value);
    }

    gboolean failed = FALSE;
    gboolean parsing = TRUE;

    while(parsing)
    {
        GValue value = {0};
        const gchar *name = NULL;
        guint token = g_scanner_peek_next_token(scanner);

        switch(token)
        {
            case G_TOKEN_IDENTIFIER:
                g_scanner_get_next_token(scanner);
                name = scanner->value.v_identifier;
                break;
            case G_TOKEN_FLOAT:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_DOUBLE);
                gel_value_set_double(&value, (gdouble)scanner->value.v_float);
                break;
            case G_TOKEN_INT:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_INT64);
                gel_value_set_int64(&value, (gint64)scanner->value.v_int64);
                break;
            case '(':
            case '[':
            case '{':
            {
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_VALUE_ARRAY);
                GValueArray *inner_array = gel_parse_scanner(scanner,
                    scanner->line, scanner->position, token, error);
                if(array != NULL)
                    gel_value_take_boxed(&value, inner_array);
                else
                    failed = TRUE;
                break;
            }
            case ')':
            case ']':
            case '}':
                if((delim == '(' && token != ')') ||
                   (delim == '[' && token != ']') ||
                   (delim == '{' && token != '}'))
                {
                    g_propagate_error(error, g_error_new(
                        gel_parse_error_quark(),
                        GEL_PARSE_ERROR_UNMATCHED_DELIM,
                        "Cannot close '%c' at line %u, char %u "
                        "with '%c' at line %u, char %u",
                        delim, line, pos, token,
                        scanner->next_line, scanner->next_position));
                    failed = TRUE;
                    parsing = FALSE;
                }
                else
                if(delim == 0)
                {
                    g_propagate_error(error, g_error_new(
                        gel_parse_error_quark(),
                        GEL_PARSE_ERROR_UNEXPECTED_DELIM,
                        "Unexpected '%c' at line %u, char %u",
                        token, scanner->next_line, scanner->next_position));
                    failed = TRUE;
                    parsing = FALSE;
                }
                else
                {
                    g_scanner_get_next_token(scanner);
                    parsing = FALSE;
                }
                break;
            case G_TOKEN_EOF:
                if(line != 0)
                {
                    g_propagate_error(error, g_error_new(
                        gel_parse_error_quark(),
                        GEL_PARSE_ERROR_EXPECTED_DELIM,
                        "'%c' opened at line %u, char %u was not closed",
                        delim, line, pos));
                    failed = TRUE;
                }
                parsing = FALSE;
                break;
            case G_TOKEN_STRING:
                g_scanner_get_next_token(scanner);
                g_value_init(&value, G_TYPE_STRING);
                g_value_set_string(&value, scanner->value.v_string);
                break;
            case G_TOKEN_ERROR:
                g_scanner_get_next_token(scanner);
                g_propagate_error(error, g_error_new(
                    gel_parse_error_quark(),
                    GEL_PARSE_ERROR_SCANNER,
                    "%s at line %u, char %u",
                    scanner_errors[scanner->value.v_error],
                    scanner->line, scanner->position));
                failed = TRUE;
                parsing = FALSE;
                break;
            default:
                g_propagate_error(error, g_error_new(
                    gel_parse_error_quark(),
                    GEL_PARSE_ERROR_UNKNOWN_TOKEN,
                    "Unknown token '%c' (%d) at line %u, char %u",
                    token, token, scanner->next_line, scanner->next_position));
                failed = TRUE;
                parsing = FALSE;
                break;
        }

        if(name != NULL)
        {
            gboolean is_number = FALSE;
            guint len = strlen(name);
            if(name[0] == '-' && len > 1)
            {
                GScanner *num_scanner = g_scanner_new(NULL);
                num_scanner->config->store_int64 = TRUE;

                g_scanner_input_text(num_scanner, name+1, len-1);
                guint num_token = g_scanner_get_next_token(num_scanner);

                if(g_scanner_peek_next_token(num_scanner) != G_TOKEN_EOF)
                    num_token = G_TOKEN_EOF;

                switch(num_token)
                {
                    case G_TOKEN_FLOAT:
                        g_value_init(&value, G_TYPE_DOUBLE);
                        gel_value_set_double(&value,
                            -(gdouble)num_scanner->value.v_float);
                        is_number = TRUE;
                        break;
                    case G_TOKEN_INT:
                        g_value_init(&value, G_TYPE_INT64);
                        gel_value_set_int64(&value,
                            -(gint64)num_scanner->value.v_int64);
                        is_number = TRUE;
                        break;
                    default:
                        break;
                }

                g_scanner_destroy(num_scanner);
            }
            
            if(!is_number)
            {
                GelVariable *variable = gel_variable_lookup_predefined(name);
                GelSymbol *symbol = gel_symbol_new(name, variable);
                g_value_init(&value, GEL_TYPE_SYMBOL);
                gel_value_take_boxed(&value, symbol);
            }
        }
    
        if(G_IS_VALUE(&value))
        {
            g_value_array_append(array, &value);
            g_value_unset(&value);
        }
    }

    if(failed)
    {
        g_value_array_free(array);
        array = NULL;
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
    gchar *content = NULL;
    gsize content_len = 0;
    GValueArray *array = NULL;

    if(g_file_get_contents(file, &content, &content_len, error))
    {
        array = gel_parse_text(content, content_len, error);
        g_free(content);
    }

    return array;
}


/**
 * gel_parse_text:
 * @text: text to parse
 * @text_len: length of the content to parse, or -1 if it is zero terminated.
 *
 * Uses a #GScanner to parse @content, operators are replaced with their
 * corresponding functions (add for +, mul for *, etc). Characters [ ] are
 * used to build array literals. Integers are considered #gint64 literals,
 * strings are #gchararray literals and floats are #gdouble literals.
 *
 * #gel_context_eval_value considers array literals as closure's invokes.
 *
 * Returns: A #GValueArray with the parsed value literals.
 */
GValueArray* gel_parse_text(const gchar *text, guint text_len, GError **error)
{
    GScanner *scanner = g_scanner_new(NULL);
    GScannerConfig *config = scanner->config;

    config->cset_identifier_first =
        G_CSET_a_2_z G_CSET_A_2_Z "=_+-*/%!&<>.";
    config->cset_identifier_nth =
        G_CSET_a_2_z G_CSET_A_2_Z "=_+-*/%!&<>.?" G_CSET_DIGITS;
    config->cpair_comment_single = "#\n";
    config->scan_identifier_1char = TRUE;
    config->store_int64 = TRUE;

    g_scanner_input_text(scanner, text, text_len);

    GValueArray *array = gel_parse_scanner(scanner, 0, 0, 0, error);
    g_scanner_destroy(scanner);

    return array;
}

