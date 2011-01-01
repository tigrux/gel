#ifndef __GEL_VALUE_H__
#define __GEL_VALUE_H__

#include <glib-object.h>


GValue* gel_value_new(void);
GValue* gel_value_new_of_type(GType type);
GValue* gel_value_new_from_boolean(gboolean value_boolean);
GValue* gel_value_new_from_pointer(gpointer value_pointer);
GValue* gel_value_new_from_closure(GClosure *value_closure);
GValue* gel_value_new_from_closure_marshal(GClosureMarshal marshal,
                                           GObject *object);

GValue *gel_value_dup(const GValue *value);
gboolean gel_value_copy(const GValue *src_value, GValue *dest_value);
void gel_value_free(GValue *value);
void gel_value_list_free(GList *list);
gchar *gel_value_to_string(const GValue *value);
gboolean gel_value_to_boolean(const GValue *value);

gboolean gel_values_add(const GValue *v1, const GValue *v2, GValue *add_value);
gboolean gel_values_sub(const GValue *v1, const GValue *v2, GValue *sub_value);
gboolean gel_values_mul(const GValue *v1, const GValue *v2, GValue *muv1);
gboolean gel_values_div(const GValue *v1, const GValue *v2, GValue *div_value);
gboolean gel_values_mod(const GValue *v1, const GValue *v2, GValue *div_value);

gint gel_values_compare(const GValue *v1, const GValue *v2);
gboolean gel_values_gt(const GValue *v1, const GValue *v2);
gboolean gel_values_ge(const GValue *v1, const GValue *v2);
gboolean gel_values_eq(const GValue *v1, const GValue *v2);
gboolean gel_values_le(const GValue *v1, const GValue *v2);
gboolean gel_values_lt(const GValue *v1, const GValue *v2);
gboolean gel_values_ne(const GValue *v1, const GValue *v2);


#endif
