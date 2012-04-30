#ifndef __GEL_CLOSURE_PRIVATE_H__
#define __GEL_CLOSURE_PRIVATE_H__

#include <gelcontext.h>

typedef struct _GelClosure GelClosure;

void gel_closure_close_over(GClosure *closure);

#ifdef HAVE_GOBJECT_INTROSPECTION
#include <geltypeinfo.h>

typedef struct _GelIntrospectionClosure GelIntrospectionClosure;

GClosure* gel_closure_new_introspection(const GelTypeInfo *info,
                                        GObject *object);
const GelTypeInfo* gel_introspection_closure_get_info(GelIntrospectionClosure *self);
GObject* gel_introspection_closure_get_object(GelIntrospectionClosure *self);
#endif

#endif
