#ifndef GEL_TYPE_NAMESPACE
#define GEL_TYPE_NAMESPACE (gel_namespace_get_type())

#include <gelnamespace.h>
#include <gelbaseinfo.h>

typedef struct _GelNamespace GelNamespace;

GType gel_namespace_get_type(void);
GelNamespace* gel_namespace_new(const gchar *namespace_, const gchar *version);
GelNamespace* gel_namespace_ref(GelNamespace *self);
void gel_namespace_unref(GelNamespace *self);

const gchar* gel_namespace_get_name(const GelNamespace *self);
const GelBaseInfo* gel_namespace_lookup(const GelNamespace *self,
                                        const gchar *name);

#endif

