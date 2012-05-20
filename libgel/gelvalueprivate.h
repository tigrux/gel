#ifndef __GEL_VALUE_PRIVATE_H__
#define __GEL_VALUE_PRIVATE_H__

#include <glib-object.h>

#include <gelvariable.h>
#include <gelarray.h>

#define gel_value_new() (g_new0(GValue, 1))
#define gel_value_new_of_type(t) (g_value_init(gel_value_new(), t))

typedef
gboolean (*GelValuesArithmetic)(const GValue *l_value, const GValue *r_value,
                                GValue *dest_value);

typedef
gboolean (*GelValuesLogic)(const GValue *l_value, const GValue *r_value);

GValue* gel_value_new_from_boxed(GType type, gpointer boxed);
GValue* gel_value_dup(const GValue *value);
void gel_value_free(GValue *value);

GList* gel_args_from_array(const GelArray *vars, gchar **variadic,
                           gchar **invalid);

GHashTable* gel_hash_table_new(void);

GelVariable* gel_variable_lookup_predefined(const gchar *name);

const GValue* gel_value_lookup_predefined(const gchar *name);

#endif

