#ifndef __GEL_VALUE_PRIVATE_H__
#define __GEL_VALUE_PRIVATE_H__

#include <glib-object.h>

#define gel_value_get_boolean(v) ((v)->data[0].v_int != FALSE)
#define gel_value_get_long(v) ((v)->data[0].v_long)
#define gel_value_get_double(v) ((v)->data[0].v_double)
#define gel_value_get_boxed(v) ((v)->data[0].v_pointer)
#define gel_value_get_pointer(v) ((v)->data[0].v_pointer)
#define gel_value_peek_pointer(v) ((v)->data[0].v_pointer)
#define gel_value_get_string(v) ((gchar*)(v)->data[0].v_pointer)

#define gel_value_set_boolean(v, a) ((v)->data[0].v_int = (a) != FALSE)
#define gel_value_set_long(v, a) ((v)->data[0].v_long = (glong)(a))
#define gel_value_set_uint(v, a) ((v)->data[0].v_uint = (guint)(a))
#define gel_value_set_pointer(v, a) ((v)->data[0].v_pointer = (gpointer)(a))
#define gel_value_set_double(v, a) ((v)->data[0].v_double = (gdouble)(a))

#define GEL_VALUE_HOLDS(v, t) ((v)->g_type == (t) || g_type_is_a((v)->g_type, (t)))
#define GEL_VALUE_TYPE(v) ((v)->g_type)
#define GEL_VALUE_TYPE_NAME(v) (g_type_name((v)->g_type))
#define GEL_IS_VALUE(v) ((v)->g_type != G_TYPE_INVALID)

#define gel_value_new() (g_new0(GValue, 1))


typedef
gboolean (*GelValuesArithmetic)(const GValue *l_value, const GValue *r_value,
                                GValue *dest_value);

typedef
gboolean (*GelValuesLogic)(const GValue *l_value, const GValue *r_value);

GValue* gel_value_new_of_type(GType type);
GValue* gel_value_new_from_boolean(gboolean value_boolean);
GValue* gel_value_new_from_pointer(gpointer value_pointer);
GValue* gel_value_new_from_closure(GClosure *value_closure);
GValue* gel_value_new_from_closure_marshal(GClosureMarshal marshal,
                                           gpointer data);

GValue *gel_value_dup(const GValue *value);
void gel_value_free(GValue *value);


#endif
