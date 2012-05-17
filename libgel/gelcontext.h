#ifndef __GEL_CONTEXT_H__
#define __GEL_CONTEXT_H__

#include <glib-object.h>

#define GEL_CONTEXT_ERROR (gel_context_error_quark())

#ifndef __GTK_DOC_IGNORE__
GQuark gel_context_error_quark(void);
#endif

typedef enum _GelContextError
{
   GEL_CONTEXT_ERROR_ARGUMENTS,
   GEL_CONTEXT_ERROR_UNKNOWN_SYMBOL,
   GEL_CONTEXT_ERROR_SYMBOL_EXISTS,
   GEL_CONTEXT_ERROR_TYPE,
   GEL_CONTEXT_ERROR_PROPERTY,
   GEL_CONTEXT_ERROR_INDEX,
   GEL_CONTEXT_ERROR_KEY
} GelContextError;

typedef struct _GelContext GelContext;

typedef void (*GelFunction)(GClosure *closure, GValue *return_value,
                            guint n_param_values, GValue *param_values,
                            GelContext *invocation_context, gpointer user_data);

GelContext* gel_context_new(void);
GelContext* gel_context_new_with_outer(GelContext *outer);
GelContext* gel_context_copy(const GelContext *self);
void gel_context_free(GelContext *self);

GValue* gel_context_lookup(const GelContext *self, const gchar *name);

void gel_context_define(GelContext *self, const gchar *name, GType type, ...);
void gel_context_define_value(GelContext *self,
                              const gchar *name, GValue *value);
void gel_context_define_object(GelContext *self, const gchar *name,
                               GObject *object);
void gel_context_define_function(GelContext *self, const gchar *name,
                                 GelFunction function, void *user_data);

gboolean gel_context_remove(GelContext *self, const gchar *name);

gboolean gel_context_eval(GelContext *self, const GValue *value, GValue *dest,
                          GError **error);

gboolean gel_context_error(const GelContext* self);
void gel_context_clear_error(GelContext* self);

#endif

