#ifndef GEL_TYPE_TYPEINFO
#define GEL_TYPE_TYPEINFO (gel_type_info_get_type())

#include <girepository.h>
#include <gelcontext.h>

typedef struct _GelTypeInfo GelTypeInfo;
GType gel_type_info_get_type(void) G_GNUC_CONST;

GelTypeInfo* gel_type_info_new(GIBaseInfo *info);
GelTypeInfo* gel_type_info_ref(GelTypeInfo *self);
void gel_type_info_unref(GelTypeInfo *self);

const GelTypeInfo* gel_type_info_lookup(const GelTypeInfo *self,
                                        const gchar *name);

GelTypeInfo* gel_type_info_from_gtype(GType type);

gchar* gel_type_info_to_string(const GelTypeInfo *self);
gboolean gel_type_info_to_value(const GelTypeInfo *self, void *instance,
                                GValue *return_value);

void gel_type_info_closure_marshal(GClosure *closure, GValue *return_value,
                                   guint n_values, const GValue *values,
                                   GelContext *context);

#endif

