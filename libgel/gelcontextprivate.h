#ifndef __GEL_CONTEXT_PRIVATE_H__
#define __GEL_CONTEXT_PRIVATE_H__

#include <gelcontext.h>
#include <gelvariable.h>
#include <gelclosure.h>
#include <gelclosureprivate.h>

void gel_context_insert_variable(GelContext *self,
                                 const gchar *name, GelVariable *variable);

GelVariable* gel_context_lookup_variable(const GelContext *self,
                                         const gchar *name);

gboolean gel_context_eval_value(GelContext *self,
                                const GValue *value, GValue *dest);
const GValue* gel_context_eval_into_value(GelContext *self,
                                          const GValue *value,
                                          GValue *out_value);
const GValue* gel_context_eval_param_into_value(GelContext *self,
                                                const GValue *value,
                                                GValue *out_value);
gboolean gel_context_eval_params(GelContext *self, const gchar *func,
                                 guint *n_values, const GValue **values,
                                 GList **list, const gchar *format, ...);

GelVariable* gel_context_get_variable(const GelContext *self,
                                      const gchar *name);

void gel_context_set_outer(GelContext *self, GelContext *context);
void gel_context_set_error(GelContext* self, GError *error);

GelContext* gel_context_validate(GelContext *context);

#endif
