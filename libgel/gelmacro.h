#ifndef __GEL_MACRO_H__

#include <glib-object.h>
#include <gelarray.h>

typedef struct _GelMacro GelMacro;

GelMacro* gel_macro_new(GList *args, gchar *variadic, GelArray *code);

void gel_macro_free(GelMacro *self);

GelMacro* gel_macro_lookup(const gchar *name);

GelArray* gel_macro_invoke(const GelMacro *self, const gchar *name,
                           guint n_values, const GValue *values,
                           GError **error);

#endif
