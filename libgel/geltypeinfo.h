#ifndef GEL_TYPE_TYPEINFO
#define GEL_TYPE_TYPEINFO (gel_type_info_get_type())

#include <girepository.h>

typedef struct _GelTypeInfo GelTypeInfo;
GType gel_type_info_get_type(void) G_GNUC_CONST;

GelTypeInfo* gel_type_info_new(GIBaseInfo *info);
GelTypeInfo* gel_type_info_ref(GelTypeInfo *self);
void gel_type_info_unref(GelTypeInfo *self);

const GelTypeInfo* gel_type_info_lookup(const GelTypeInfo *self,
                                        const gchar *name);

GelTypeInfo* gel_type_info_from_gtype(GType type);

gboolean gel_type_info_to_value(const GelTypeInfo *self, GObject *object,
                                GValue *return_value);

#endif

