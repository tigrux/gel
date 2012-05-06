#ifndef __GEL_PARSER_H__
#define __GEL_PARSER_H__

#include <glib-object.h>

/**
 * GEL_PARSE_ERROR:
 *
 * Error domain for errors reported by #gel_parse_file and #gel_parse_text.
 * Errors in this domain will be from the #GelParseError enumeration.
 * See #GError for information on error domains.
 */

#define GEL_PARSE_ERROR (gel_parse_error_quark())

#ifndef __GTK_DOC_IGNORE__
GQuark gel_parse_error_quark(void);
#endif

/**
 * GelParseError:
 * @GEL_PARSE_ERROR_UNKNOWN: unknown error
 * @GEL_PARSE_ERROR_UNEXP_EOF: unexpected end of file
 * @GEL_PARSE_ERROR_UNEXP_EOF_IN_STRING: unterminated string constant
 * @GEL_PARSE_ERROR_UNEXP_EOF_IN_COMMENT: unterminated comment
 * @GEL_PARSE_ERROR_NON_DIGIT_IN_CONST: non-digit character in a number
 * @GEL_PARSE_ERROR_DIGIT_RADIX: digit beyond radix in a number
 * @GEL_PARSE_ERROR_FLOAT_RADIX: non-decimal floating point number
 * @GEL_PARSE_ERROR_FLOAT_MALFORMED: malformed floating point number
 * @GEL_PARSE_ERROR_UNEXP_DELIM: unexpected closing delimiter
 * @GEL_PARSE_ERROR_UNEXP_EOF_IN_ARRAY: unterminated array
 * @GEL_PARSE_ERROR_UNKNOWN_TOKEN: unknown token
 * @GEL_PARSE_ERROR_MACRO_MALFORMED: malformed macro
 * @GEL_PARSE_ERROR_MACRO_ARGUMENTS: wrong arguments for macro
 *
 * Error codes reported by #gel_parse_text and #gel_parse_file
 */

typedef enum _GelParseError
{
	GEL_PARSE_ERROR_UNKNOWN,
	GEL_PARSE_ERROR_UNEXP_EOF,
	GEL_PARSE_ERROR_UNEXP_EOF_IN_STRING,
	GEL_PARSE_ERROR_UNEXP_EOF_IN_COMMENT,
	GEL_PARSE_ERROR_NON_DIGIT_IN_CONST,
	GEL_PARSE_ERROR_DIGIT_RADIX,
	GEL_PARSE_ERROR_FLOAT_RADIX,
	GEL_PARSE_ERROR_FLOAT_MALFORMED,
	GEL_PARSE_ERROR_UNEXP_DELIM,
	GEL_PARSE_ERROR_UNEXP_EOF_IN_ARRAY,
	GEL_PARSE_ERROR_UNKNOWN_TOKEN,
    GEL_PARSE_ERROR_MACRO_MALFORMED,
    GEL_PARSE_ERROR_MACRO_ARGUMENTS
} GelParseError;

GValueArray* gel_parse_file(const gchar *file, GError **error);
GValueArray* gel_parse_text(const gchar *text, guint text_len, GError **error);

#endif

