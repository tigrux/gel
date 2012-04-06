#ifndef GEL_TYPE_TYPELIB
#define GEL_TYPE_TYPELIB (gel_typelib_get_type())

#include <geltypelib.h>
#include <gelbaseinfo.h>

typedef struct _GelTypelib GelTypelib;

GType gel_typelib_get_type(void);
GelTypelib* gel_typelib_new(const gchar *ns, const gchar *version);
GelTypelib* gel_typelib_ref(GelTypelib *self);
void gel_typelib_unref(GelTypelib *self);

const gchar* gel_typelib_get_name(const GelTypelib *self);
const GelBaseInfo* gel_typelib_lookup(const GelTypelib *self,
                                      const gchar *name);

#endif

