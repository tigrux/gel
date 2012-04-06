#ifndef GEL_TYPE_BASE_INFO
#define GEL_TYPE_BASE_INFO (gel_base_info_get_type())

#include <girepository.h>

typedef struct _GelBaseInfo GelBaseInfo;

GType gel_base_info_get_type(void);
GelBaseInfo* gel_base_info_new(GIBaseInfo *info);
GelBaseInfo* gel_base_info_ref(GelBaseInfo *self);
void gel_base_info_unref(GelBaseInfo *self);

const gchar* gel_base_info_get_name(const GelBaseInfo *self);
GIBaseInfo* gel_base_info_get_info(const GelBaseInfo *self);
const GelBaseInfo* gel_base_info_lookup(const GelBaseInfo *self,
                                        const gchar *name);

#endif

