#ifndef __GEL_VALUE_PRIVATE_H__
#define __GEL_VALUE_PRIVATE_H__

#include <glib-object.h>

#ifndef GEL_VALUE_USE_MACRO
#define GEL_VALUE_USE_MACRO 1
#endif

#if GEL_VALUE_USE_MACRO

#define gel_value_get_boolean(v) ((v)->data[0].v_int != FALSE)
#define gel_value_get_long(v) ((v)->data[0].v_long)
#define gel_value_get_int64(v) ((v)->data[0].v_int64)
#define gel_value_get_double(v) ((v)->data[0].v_double)
#define gel_value_get_gtype(v) ((GType)(v)->data[0].v_pointer)
#define gel_value_get_boxed(v) ((v)->data[0].v_pointer)
#define gel_value_get_pointer(v) ((v)->data[0].v_pointer)
#define gel_value_peek_pointer(v) ((v)->data[0].v_pointer)
#define gel_value_get_string(v) ((gchar*)(v)->data[0].v_pointer)
#define gel_value_get_object(v) ((gpointer)(v)->data[0].v_pointer)
#define gel_value_set_boolean(v, a) ((v)->data[0].v_int = (a) != FALSE)
#define gel_value_set_uint(v, a) ((v)->data[0].v_uint = (guint)(a))
#define gel_value_set_long(v, a) ((v)->data[0].v_long = (glong)(a))
#define gel_value_set_int64(v, a) ((v)->data[0].v_int64 = (gint64)(a))
#define gel_value_set_uint64(v, a) ((v)->data[0].v_uint64 = (guint64)(a))
#define gel_value_set_double(v, a) ((v)->data[0].v_double = (gdouble)(a))
#define gel_value_set_enum(v, a) ((v)->data[0].v_long = (glong)(a))
#define gel_value_set_gtype(v, a) ((v)->data[0].v_pointer = (gpointer)(gsize)(a))
#define gel_value_set_pointer(v, a) ((v)->data[0].v_pointer = (gpointer)(a))
#define gel_value_take_boxed(v, a) ((v)->data[0].v_pointer = (gpointer)(a))
#define gel_value_take_object(v, a) ((v)->data[0].v_pointer = (gpointer)(a))
#define GEL_VALUE_HOLDS(v, t) ((v)->g_type == (t) || g_type_is_a((v)->g_type, (t)))
#define GEL_VALUE_TYPE(v) ((v)->g_type)
#define GEL_VALUE_TYPE_NAME(v) (g_type_name(GEL_VALUE_TYPE(v)))
#define GEL_IS_VALUE(v) ((v)->g_type != G_TYPE_INVALID)

#else

#define gel_value_get_boolean g_value_get_boolean
#define gel_value_get_long g_value_get_long
#define gel_value_get_int64 g_value_get_int64
#define gel_value_get_double g_value_get_double
#define gel_value_get_gtype g_value_get_gtype
#define gel_value_get_boxed g_value_get_boxed
#define gel_value_get_pointer g_value_get_pointer
#define gel_value_peek_pointer g_value_peek_pointer
#define gel_value_get_string g_value_get_string
#define gel_value_get_object g_value_get_object
#define gel_value_set_boolean g_value_set_boolean
#define gel_value_set_uint g_value_set_uint
#define gel_value_set_long g_value_set_long
#define gel_value_set_int64 g_value_set_int64
#define gel_value_set_uint64 g_value_set_uint64
#define gel_value_set_double g_value_set_double
#define gel_value_set_enum g_value_set_enum
#define gel_value_set_gtype g_value_set_gtype
#define gel_value_set_pointer g_value_set_pointer
#define gel_value_take_boxed g_value_take_boxed
#define gel_value_take_object g_value_take_object
#define GEL_VALUE_HOLDS G_VALUE_HOLDS
#define GEL_VALUE_TYPE G_VALUE_TYPE
#define GEL_VALUE_TYPE_NAME G_VALUE_TYPE_NAME
#define GEL_IS_VALUE G_IS_VALUE

#endif

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

guint gel_value_hash(const GValue *value);

#endif

