#ifndef __GEL_MACRO_H__

#include <glib-object.h>

typedef struct _GelMacro GelMacro;

void gel_macros_new(void);

void gel_macros_free(void);

GelMacro* gel_macro_new(const gchar *name,
                        GList *args, gchar *variadic, GValueArray *code);

void gel_macro_free(GelMacro *self);

GelMacro* gel_macro_lookup(const gchar *name);

GValueArray* gel_macro_code_from_value(GValue *pre_value, GError **error);

#endif
