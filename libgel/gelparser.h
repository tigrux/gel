#ifndef __GEL_PARSER_H__
#define __GEL_PARSER_H__

#include <glib-object.h>
#include <gelarray.h>

#define GEL_PARSER_ERROR (gel_parser_error_quark())

#ifndef __GTK_DOC_IGNORE__
GQuark gel_parser_error_quark(void);
#endif

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

void gel_parser_input_text(GelParser *self, const gchar *text, gsize text_len);
void gel_parser_input_file(GelParser *self, gint fd);

gboolean gel_parser_next_value(GelParser *self, GValue *value, GError **error);
GelArray* gel_parser_get_values(GelParser *self, GError **error);

typedef struct _GelParserIter GelParserIter;

struct _GelParserIter
{
    GelParser *parser;
    GValue value;
};

void gel_parser_iter_init(GelParserIter *iter, GelParser *parser);
void gel_parser_iter_destroy(GelParserIter *iter);
gboolean gel_parser_iter_next(GelParserIter *iter, GError **error);
GValue* gel_parser_iter_get(GelParserIter *iter);

#endif

