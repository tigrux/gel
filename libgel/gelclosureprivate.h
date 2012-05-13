#ifndef __GEL_CLOSURE_PRIVATE_H__
#define __GEL_CLOSURE_PRIVATE_H__

#include <gelcontext.h>

typedef struct _GelClosure GelClosure;

void gel_closure_close_over(GClosure *closure);

#ifdef HAVE_GOBJECT_INTROSPECTION
#include <geltypeinfo.h>

typedef struct _GelIntrospectionClosure GelIntrospectionClosure;

GClosure* gel_closure_new_introspection(const GelTypeInfo *info,
                                        guint n_args,
                                        gboolean *indirect_args,
                                        void *instance);

const GelTypeInfo* gel_introspection_closure_get_info(GelIntrospectionClosure *self);

void* gel_introspection_closure_get_instance(const GelIntrospectionClosure *self);

guint gel_introspection_closure_get_n_args(const GelIntrospectionClosure *self);

gboolean gel_introspection_closure_arg_is_expected(const GelIntrospectionClosure *self,
                                                   guint arg_index);

#endif

#endif
