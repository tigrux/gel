#ifndef GEL_TYPE_VALUE_LIST
#define GEL_TYPE_VALUE_LIST (gel_value_list_get_type())

#include <glib-object.h>


GType gel_value_list_get_type(void);

GList* gel_value_list_copy(const GList *list);
void gel_value_list_free(GList *list);

gchar* gel_value_list_to_string(const GList *list);
GValueArray* gel_value_list_to_array(const GList *list);


#endif
