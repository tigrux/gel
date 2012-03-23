#ifndef __GEL_CONTEXT_PRIVATE_H
#define __GEL_CONTEXT_PRIVATE_H

#include <gelcontext.h>
#include <gelvariable.h>
#include <gelclosure.h>
#include <gelclosureprivate.h>


void gel_context_insert_variable(GelContext *self,
                                 const gchar *name, GelVariable *variable);

GelVariable* gel_context_lookup_variable(const GelContext *self,
                                         const gchar *name);

GList* gel_context_get_variables(const GelContext *self);

gboolean gel_context_has_variable(const GelContext *self, const gchar *name);

void gel_context_append_closure(GelContext *self, GelClosure *closure);


#endif
