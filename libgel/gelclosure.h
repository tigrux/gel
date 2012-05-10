#ifndef __GEL_CLOSURE_H__
#define __GEL_CLOSURE_H__

#include <gelcontext.h>
#include <gelarray.h>

GClosure* gel_closure_new(const gchar *name, GList *args, gchar *variadic, 
                          guint n_values, const GValue *values,
                          GelContext *context);

GClosure* gel_closure_new_native(const gchar *name, GClosureMarshal marshal);

const gchar* gel_closure_get_name(const GClosure *closure);

#endif
