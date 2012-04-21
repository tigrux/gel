#ifndef __GEL_CLOSURE_PRIVATE_H__
#define __GEL_CLOSURE_PRIVATE_H__

#include <gelcontext.h>
#include <geltypeinfo.h>


typedef struct _GelClosure GelClosure;
typedef struct _GelIntrospectionClosure GelIntrospectionClosure;

GelContext* gel_closure_get_context(GelClosure *self);
void gel_closure_close_over(GClosure *closure);

GClosure* gel_closure_new_introspection(const GelTypeInfo *info,
                                        GObject *object);
const GelTypeInfo* gel_introspection_closure_get_info(GelIntrospectionClosure *self);
GObject* gel_introspection_closure_get_object(GelIntrospectionClosure *self); 


#endif
