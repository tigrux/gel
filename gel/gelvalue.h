#ifndef __GEL_VALUE_H__
#define __GEL_VALUE_H__

#include <glib-object.h>


void gel_value_list_free(GList *list);

gboolean gel_values_add(const GValue *v1, const GValue *v2, GValue *dest_value);
gboolean gel_values_sub(const GValue *v1, const GValue *v2, GValue *dest_value);
gboolean gel_values_mul(const GValue *v1, const GValue *v2, GValue *dest_Value);
gboolean gel_values_div(const GValue *v1, const GValue *v2, GValue *dest_value);
gboolean gel_values_mod(const GValue *v1, const GValue *v2, GValue *dest_value);

gint gel_values_compare(const GValue *v1, const GValue *v2);

gboolean gel_values_gt(const GValue *v1, const GValue *v2);
gboolean gel_values_ge(const GValue *v1, const GValue *v2);
gboolean gel_values_eq(const GValue *v1, const GValue *v2);
gboolean gel_values_le(const GValue *v1, const GValue *v2);
gboolean gel_values_lt(const GValue *v1, const GValue *v2);
gboolean gel_values_ne(const GValue *v1, const GValue *v2);


#endif
