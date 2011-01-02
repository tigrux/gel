#ifndef __GEL_VALUE_PRIVATE_H__
#define __GEL_VALUE_PRIVATE_H__

#include <glib-object.h>


typedef
gboolean (*GelValuesArithmetic)(const GValue *l_value, const GValue *r_value,
                                GValue *dest_value);

typedef
gboolean (*GelValuesLogic)(const GValue *l_value, const GValue *r_value);

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
gchar *gel_value_to_string(const GValue *value);
gboolean gel_value_to_boolean(const GValue *value);


#endif
