#ifndef __GEL_PARSER_H__
#define __GEL_PARSER_H__

#include <glib-object.h>

#define GEL_PARSE_ERROR (gel_parse_error_quark())

GQuark gel_parse_error_quark(void);

typedef enum _GelParseError
{
   GEL_PARSE_ERROR_SCANNER,
   GEL_PARSE_ERROR_UNMATCHED_DELIM,
   GEL_PARSE_ERROR_UNEXPECTED_DELIM,
   GEL_PARSE_ERROR_EXPECTED_DELIM,
   GEL_PARSE_ERROR_UNKNOWN_TOKEN,
} GelParseError;

GValueArray* gel_parse_file(const gchar *file, GError **error);
GValueArray* gel_parse_text(const gchar *text, guint text_len, GError **error);

#endif

