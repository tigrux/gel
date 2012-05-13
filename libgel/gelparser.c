#include <string.h>

#include <gelparser.h>
#include <gelsymbol.h>
#include <gelvalue.h>
#include <gelvalueprivate.h>
#include <gelmacro.h>

#define ARRAY_N_PREALLOCATED 8


/**
 * SECTION:gelparser
 * @short_description: Class used to get values from data
 * @title: GelParser
 * @include: gel.h
 *
 * Class used to get values from text or files.
 */

/**
 * GEL_PARSER_ERROR:
 *
 * Error domain for errors reported by #gel_parse_file and #gel_parse_text.
 * Errors in this domain will be from the #GelParserError enumeration.
 * See #GError for information on error domains.
 */

/**
 * GelParserError:
 * @GEL_PARSER_ERROR_UNKNOWN: unknown error
 * @GEL_PARSER_ERROR_UNEXP_EOF: unexpected end of file
 * @GEL_PARSER_ERROR_UNEXP_EOF_IN_STRING: unterminated string constant
 * @GEL_PARSER_ERROR_UNEXP_EOF_IN_COMMENT: unterminated comment
 * @GEL_PARSER_ERROR_NON_DIGIT_IN_CONST: non-digit character in a number
 * @GEL_PARSER_ERROR_DIGIT_RADIX: digit beyond radix in a number
 * @GEL_PARSER_ERROR_FLOAT_RADIX: non-decimal floating point number
 * @GEL_PARSER_ERROR_FLOAT_MALFORMED: malformed floating point number
 * @GEL_PARSER_ERROR_UNEXP_DELIM: unexpected closing delimiter
 * @GEL_PARSER_ERROR_UNEXP_EOF_IN_ARRAY: unterminated array
 * @GEL_PARSER_ERROR_UNKNOWN_TOKEN: unknown token
 * @GEL_PARSER_ERROR_MACRO_MALFORMED: malformed macro
 * @GEL_PARSER_ERROR_MACRO_ARGUMENTS: wrong arguments for macro
 *
 * Error codes reported by #gel_parse_text and #gel_parse_file
 */

GQuark gel_parser_error_quark(void)
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
GScanner* gel_scanner_new(void)
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
    config->scan_string_sq = FALSE;

    return scanner;
}


/**
 * SECTION:gelparser
 * @short_description: Class used to parse text
 * @title: GelParser
 * @include: gel.h
 *
 * #GelParser is a class used to parse text.
 */

struct _GelParser
{
    GScanner *scanner;
    GHashTable *macros;
    GList *next;
};


/**
 * gel_parser_new:
 *
 * Creates a #GelParser
 *
 * Returns: A new #GelParser
 */
GelParser* gel_parser_new(void)
{
    GelParser *self = g_slice_new0(GelParser);

    self->scanner = gel_scanner_new();
    self->macros = g_hash_table_new_full(
        g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)gel_macro_free);

    return self;
}


/**
 * gel_parser_free:
 * @self: #GelParser to free
 *
 * Releases resources of @self
 */
void gel_parser_free(GelParser *self)
{
    g_return_if_fail(self != NULL);

    g_scanner_destroy(self->scanner);
    g_hash_table_unref(self->macros);
    g_slice_free(GelParser, self);
}


static
GelArray* gel_parser_macro_code_from_value(GelParser *self,
                                           GValue *pre_value, GError **error)
{
    if(GEL_VALUE_TYPE(pre_value) == GEL_TYPE_ARRAY)
    {
        GelArray *array = gel_value_get_boxed(pre_value);
        GValue *values = gel_array_get_values(array);
        guint n_values = gel_array_get_n_values(array);

        GType type = GEL_VALUE_TYPE(values + 0);
        if(type != GEL_TYPE_SYMBOL)
            return NULL;

        GelSymbol *symbol = gel_value_get_boxed(values + 0);
        const gchar *name = gel_symbol_get_name(symbol);

        n_values--;
        values++;

        if(g_strcmp0(name, "macro") == 0)
        {
            if(n_values < 2)
            {
                g_propagate_error(error, g_error_new(
                    GEL_PARSER_ERROR, GEL_PARSER_ERROR_MACRO_MALFORMED,
                    "Macro: expected arguments and code"));
                return NULL;
            }

            symbol = gel_value_get_boxed(values + 0);
            name = gel_symbol_get_name(symbol);

            GelArray *vars = gel_value_get_boxed(values + 1);
            gchar *variadic = NULL;
            gchar *invalid = NULL;
            GList* args = gel_args_from_array(vars, &variadic, &invalid);

            if(invalid != NULL)
            {
                g_propagate_error(error, g_error_new(
                    GEL_PARSER_ERROR, GEL_PARSER_ERROR_MACRO_MALFORMED,
                    "Macro: '%s' is an invalid argument name", invalid));
                g_free(invalid);
                return NULL;
            }

            n_values -= 2;
            values += 2;

            GelArray *code = gel_array_new(n_values);
            for(guint i = 0; i < n_values; i++)
                gel_array_append(code, values + i);

            g_hash_table_insert(self->macros,
                g_strdup(name), gel_macro_new(args, variadic, code));

            return gel_array_new(0);
        }
        else
        {
            GelMacro *macro = g_hash_table_lookup(self->macros, name);
            if(macro != NULL)
                return gel_macro_invoke(macro, name, n_values, values, error);
        }
    }

    return NULL;
}


static
GelArray* gel_parser_scan(GelParser *self, GValue *dest_value,
                          guint line, guint pos,
                          gchar delim, GError **error)
{
    GScanner *scanner = self->scanner;
    GelArray *array = NULL;

    if(dest_value == NULL)
        array = gel_array_new(ARRAY_N_PREALLOCATED);
    else
    if(self->next != NULL)
    {
        GValue *next_value = self->next->data;
        gel_value_copy(next_value, dest_value);
        self->next = g_list_delete_link(self->next, self->next);
        gel_value_free(next_value);
        return NULL;
    }

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
            gel_array_append(array, pre_value);
    }

    gboolean failed = FALSE;
    gboolean parsing = TRUE;
    gboolean quoted = FALSE;

    while(parsing && !failed)
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
                GelArray *inner_array = gel_parser_scan(self, NULL,
                        scanner->line, scanner->position, token, error);
                if(inner_array != NULL)
                {
                    g_value_init(&value, GEL_TYPE_ARRAY);
                    gel_value_take_boxed(&value, inner_array);
                }
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
                        GEL_PARSER_ERROR, GEL_PARSER_ERROR_UNEXP_DELIM,
                        "Cannot close '%c' at line %u, char %u "
                        "with '%c' at line %u, char %u",
                        delim, line, pos, token,
                        scanner->next_line, scanner->next_position));
                    failed = TRUE;
                }
                else
                if(delim == 0)
                {
                    g_propagate_error(error, g_error_new(
                        GEL_PARSER_ERROR, GEL_PARSER_ERROR_UNEXP_DELIM,
                        "Unexpected '%c' at line %u, char %u",
                        token, scanner->next_line, scanner->next_position));
                    failed = TRUE;
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
                        GEL_PARSER_ERROR, GEL_PARSER_ERROR_UNEXP_EOF_IN_ARRAY,
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
                    GEL_PARSER_ERROR, scanner->value.v_error,
                    "%s at line %u, char %u",
                    scanner_errors[scanner->value.v_error],
                    scanner->line, scanner->position));
                failed = TRUE;
                break;
            case '\'':
                g_scanner_get_next_token(scanner);
                quoted = TRUE;
                break;
            default:
                g_scanner_get_next_token(scanner);
                g_propagate_error(error, g_error_new(
                    GEL_PARSER_ERROR, GEL_PARSER_ERROR_UNKNOWN_TOKEN,
                    "Unknown token '%c' (%d) at line %u, char %u",
                    token, token, scanner->next_line, scanner->next_position));
                failed = TRUE;
                break;
        }

        if(name != NULL)
        {
            gboolean is_number = FALSE;
            guint len = strlen(name);
            if(name[0] == '-' && len > 1)
            {
                GScanner *num_scanner = gel_scanner_new();
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
            if(quoted)
            {
                GelArray *quoted_array = gel_array_new(2);
                gel_array_append(quoted_array,
                    gel_value_lookup_predefined("quote"));
                gel_array_append(quoted_array, &value);

                g_value_unset(&value);
                g_value_init(&value, GEL_TYPE_ARRAY);
                gel_value_take_boxed(&value, quoted_array);
                quoted = FALSE;
            }

            GelArray *code =
                gel_parser_macro_code_from_value(self, &value, error);
            if(code != NULL)
            {
                GValue *code_values = gel_array_get_values(code);
                guint code_n_values = gel_array_get_n_values(code);

                if(array != NULL)
                    for(guint i = 0; i < code_n_values; i++)
                        gel_array_append(array, code_values + i);
                else
                if(code_n_values > 0)
                {
                    gel_value_copy(code_values + 0, dest_value);
                    parsing = FALSE;

                    for(guint i = 1; i < code_n_values; i++)
                    {
                        GValue *next_value = gel_value_dup(code_values + i);
                        self->next = g_list_append(self->next, next_value);
                    }
                }

                gel_array_free(code);
            }
            else
                if(array != NULL)
                    gel_array_append(array, &value);
                else
                {
                    gel_value_copy(&value, dest_value);
                    parsing = FALSE;
                }

            g_value_unset(&value);
        }
    }

    if(failed)
        if(array != NULL)
        {
            gel_array_free(array);
            array = NULL;
        }

    return array;
}


/**
 * gel_parser_next_value:
 * @self: a #GelParser
 * @value: address of a #GValue to store the result
 * @error: return location for a #GError, or NULL
 *
 * Obtains the next #GValue parsed by the #GelParser
 *
 * Returns: #TRUE if a value was obtained, #FALSE if an error occured
 */
gboolean gel_parser_next_value(GelParser *self, GValue *value, GError **error)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if(GEL_IS_VALUE(value))
        g_value_unset(value);

    gboolean result = FALSE;
    GError *parsed_error = NULL;

    gel_parser_scan(self, value, 0, 0, 0, &parsed_error);

    if(parsed_error != NULL)
    {
        g_propagate_error(error, parsed_error);
        if(GEL_IS_VALUE(value))
            g_value_unset(value);
    }
    else
        result = GEL_IS_VALUE(value);

    return result;
}


/**
 * gel_parser_get_values:
 * @self: a #GelParser
 * @error: return location for a #GError, or NULL
 *
 * Obtains an array of parser #GValue
 *
 * Returns: an array of parsed #GValue
 */
GelArray* gel_parser_get_values(GelParser *self, GError **error)
{
    return gel_parser_scan(self, NULL, 0, 0, 0, error);
}


/**
 * gel_parser_input_text:
 * @self: a #GelParser
 * @text: text to parse
 * @text_len: length of the content to parse, or -1 if it is zero terminated.
 *
 * Prepares the scanner associated to the #GelParser @self to scan a text
 *
 */
void gel_parser_input_text(GelParser *self, const gchar *text, gsize text_len)
{
    g_return_if_fail(self != NULL);

    g_scanner_input_text(self->scanner, text, (guint)text_len);
}


/**
 * gel_parser_input_file:
 * @self: a #GelParser
 * @fd: a file descriptor
 *
 * Prepares the scanner associated to the #GelParser @self to scan a file
 */
void gel_parser_input_file(GelParser *self, gint fd)
{
    g_return_if_fail(self != NULL);

    g_scanner_input_file(self->scanner, fd);
}

