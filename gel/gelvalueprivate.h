#ifndef __GEL_VALUE_PRIVATE_H__
#define __GEL_VALUE_PRIVATE_H__

#include <glib-object.h>

#define g_value_get_boolean(v) ((v)->data[0].v_int != FALSE)
#define g_value_get_long(v) ((v)->data[0].v_long)
#define g_value_get_double(v) ((v)->data[0].v_double)
#define g_value_get_boxed(v) ((v)->data[0].v_pointer)
#define g_value_get_string(v) ((gchar*)(v)->data[0].v_pointer)

#define g_value_set_boolean(v, a) ((v)->data[0].v_int = (a) != FALSE)
#define g_value_set_long(v, a) ((v)->data[0].v_long = (glong)(a))
#define g_value_set_double(v, a) ((v)->data[0].v_double = (gdouble)(a))

#ifdef G_VALUE_HOLDS
    #undef G_VALUE_HOLDS
#endif
#define G_VALUE_HOLDS(v, t) ((v)->g_type == (t) || g_type_is_a((v)->g_type, (t)))


#ifdef G_VALUE_TYPE
    #undef G_VALUE_TYPE
#endif
#define G_VALUE_TYPE(v) ((v)->g_type)

#ifdef G_IS_VALUE
    #undef G_IS_VALUE
#endif
#define G_IS_VALUE(v) ((v)->g_type != G_TYPE_INVALID)

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
                                           gpointer data);

GValue *gel_value_dup(const GValue *value);
gboolean gel_value_copy(const GValue *src_value, GValue *dest_value);
void gel_value_free(GValue *value);
gchar *gel_value_to_string(const GValue *value);
gint gel_values_cmp(const GValue *v1, const GValue *v2);
gboolean gel_value_to_boolean(const GValue *value);


#endif
