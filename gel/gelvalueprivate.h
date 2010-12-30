#ifndef __GEL_VALUE_PRIVATE_H__
#define __GEL_VALUE_PRIVATE_H__

#include <glib-object.h>


typedef
gboolean (*GelValuesArithmetic)(const GValue *l_value, const GValue *r_value,
                                GValue *dest_value);

typedef
gboolean (*GelValuesLogic)(const GValue *l_value, const GValue *r_value);


#endif
