#ifndef __GEL_CLOSURE_PRIVATE_H__
#define __GEL_CLOSURE_PRIVATE_H__

#include <gelcontext.h>
#include <geltypeinfo.h>


typedef struct _GelClosure GelClosure;

GelContext* gel_closure_get_context(GelClosure *self);
void gel_closure_close_over(GClosure *closure);
GClosure* gel_closure_new_introspection(const GelTypeInfo *info);


#endif
