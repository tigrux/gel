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
const GValue* gel_context_eval_param_into_value(GelContext *self,
                                     const GValue *value, GValue *out_value);

gboolean gel_context_has_variable(const GelContext *self, const gchar *name);

void gel_context_append_closure(GelContext *self, GelClosure *closure);

void gel_context_set_outer(GelContext *self, GelContext *context);

#endif
