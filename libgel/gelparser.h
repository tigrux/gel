#ifndef __GEL_PARSER_H__
#define __GEL_PARSER_H__

#include <glib-object.h>
#include <gelarray.h>

/**
 * GEL_PARSER_ERROR:
 *
 * Error domain for errors reported by #gel_parse_file and #gel_parse_text.
 * Errors in this domain will be from the #GelParserError enumeration.
 * See #GError for information on error domains.
 */

#define GEL_PARSER_ERROR (gel_parser_error_quark())

#ifndef __GTK_DOC_IGNORE__
GQuark gel_parser_error_quark(void);
#endif

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

typedef enum _GelParserError
{
    GEL_PARSER_ERROR_UNKNOWN,
    GEL_PARSER_ERROR_UNEXP_EOF,
    GEL_PARSER_ERROR_UNEXP_EOF_IN_STRING,
    GEL_PARSER_ERROR_UNEXP_EOF_IN_COMMENT,
    GEL_PARSER_ERROR_NON_DIGIT_IN_CONST,
    GEL_PARSER_ERROR_DIGIT_RADIX,
    GEL_PARSER_ERROR_FLOAT_RADIX,
    GEL_PARSER_ERROR_FLOAT_MALFORMED,
    GEL_PARSER_ERROR_UNEXP_DELIM,
    GEL_PARSER_ERROR_UNEXP_EOF_IN_ARRAY,
    GEL_PARSER_ERROR_UNKNOWN_TOKEN,
    GEL_PARSER_ERROR_MACRO_MALFORMED,
    GEL_PARSER_ERROR_MACRO_ARGUMENTS
} GelParserError;


typedef struct _GelParser GelParser;

GelParser* gel_parser_new(void);
void gel_parser_free(GelParser *self);

GelArray* gel_parser_input_text(GelParser *self,
                                const gchar *text, gsize text_len,
                                GError **error);

#endif

