#ifndef __GEL_PARSER_H__
#define __GEL_PARSER_H__

#include <glib-object.h>


GValueArray* gel_parse_file(const gchar *file, GError *error);
GValueArray* gel_parse_string(const gchar *text, guint text_len);


#endif
