#ifndef __GEL_CLOSURE_H__
#define __GEL_CLOSURE_H__

#include <gelcontext.h>


GClosure* gel_closure_new(const gchar *name, gchar **args, GValueArray *code,
                          GelContext *context);

GClosure* gel_closure_new_native(const gchar *name, GClosureMarshal marshal);

const gchar* gel_closure_get_name(const GClosure *closure);


#endif
