#ifndef __GEL_VALUE_H__
#define __GEL_VALUE_H__

#include <glib-object.h>


GValue* gel_value_new(void);
GValue* gel_value_new_of_type(GType type);
GValue* gel_value_new_from_closure(GClosure *closure);
GValue* gel_value_new_closure_from_marshall(GClosureMarshal value_marshal,
                                            GObject *self);
GValue* gel_value_new_from_static_string(const gchar *value_string);
GValue* gel_value_new_from_boolean(gboolean value_boolean);
GValue* gel_value_new_from_pointer(gpointer value_pointer);
GValue *gel_value_dup(const GValue *value);

gboolean gel_value_copy(const GValue *src_value, GValue *dest_value);
void gel_value_free(GValue *value);
gchar *gel_value_to_string(const GValue *value);
gdouble gel_value_to_double(const GValue *value);
gboolean gel_value_to_boolean(const GValue *value);

typedef
gboolean (*GelValuesArithmetic)(const GValue *l_value, const GValue *r_value,
                                GValue *dest_value);

gboolean gel_values_add(const GValue *l_value, const GValue *r_value,
                        GValue *add_value);
gboolean gel_values_sub(const GValue *l_value, const GValue *r_value,
                        GValue *sub_value);
gboolean gel_values_mul(const GValue *l_value, const GValue *r_value,
                        GValue *mul_value);
gboolean gel_values_div(const GValue *l_value, const GValue *r_value,
                        GValue *div_value);
gboolean gel_values_mod(const GValue *l_value, const GValue *r_value,
                        GValue *div_value);

typedef
gboolean (*GelValuesLogic)(const GValue *l_value, const GValue *r_value);

gboolean gel_values_gt(const GValue *l_value, const GValue *r_value);
gboolean gel_values_ge(const GValue *l_value, const GValue *r_value);
gboolean gel_values_eq(const GValue *l_value, const GValue *r_value);
gboolean gel_values_le(const GValue *l_value, const GValue *r_value);
gboolean gel_values_lt(const GValue *l_value, const GValue *r_value);
gboolean gel_values_ne(const GValue *l_value, const GValue *r_value);


#endif
