#ifndef GEL_TYPE_VARIABLE
#define GEL_TYPE_VARIABLE (gel_variable_get_type())

#define GEL_VARIABLE_VALUE(v) ((GValue*)(v))
#define GEL_VARIABLE_TYPE(v) (GEL_VALUE_TYPE((GValue*)(v)))
#define GEL_VARIABLE_TYPE_NAME(v) (GEL_VALUE_TYPE_NAME((GValue*)(v)))

#include <glib-object.h>


typedef struct _GelVariable GelVariable;

GType gel_variable_get_type(void) G_GNUC_CONST;
GelVariable* gel_variable_new(GType type);
GelVariable* gel_variable_ref(GelVariable *self);
void gel_variable_unref(GelVariable *self);

GValue* gel_variable_get_value(GelVariable *self);


#endif
