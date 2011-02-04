#ifndef GEL_CONTEXT_TYPE
#define GEL_CONTEXT_TYPE (gel_context_get_type())

#include <glib-object.h>


typedef struct _GelContext GelContext;

GelContext* gel_context_new(void);
GelContext* gel_context_new_with_outer(GelContext *outer);
GelContext* gel_context_dup(const GelContext *self);
void gel_context_free(GelContext *self);
GType gel_context_get_type(void) G_GNUC_CONST;

GValue* gel_context_lookup_symbol(const GelContext *self, const gchar *name);
GelContext* gel_context_get_outer(const GelContext *self);

void gel_context_insert_symbol(GelContext *self, const gchar *name,
                               GValue *value);
void gel_context_insert_object(GelContext *self, const gchar *name,
                               GObject *object);
void gel_context_insert_function(GelContext *self, const gchar *name,
                              GFunc function, gpointer user_data);
gboolean gel_context_remove_symbol(GelContext *self, const gchar *name);

gboolean gel_context_eval(GelContext *self,
                          const GValue *value, GValue *dest);
const GValue* gel_context_eval_value(GelContext *self,
                                     const GValue *value, GValue *out_value);
gboolean gel_context_eval_params(GelContext *self, const gchar *func,
                                 GList **list, const gchar *format,
                                 guint *n_values, const GValue **values, ...);

gboolean gel_context_get_running(const GelContext *self);
void gel_context_set_running(GelContext *self, gboolean running);

#endif

