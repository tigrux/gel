#ifndef GEL_TYPE_CONTEXT
#define GEL_TYPE_CONTEXT (gel_context_get_type())

#include <glib-object.h>

#define GEL_CONTEXT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GEL_TYPE_CONTEXT, GelContext))

#define GEL_CONTEXT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GEL_TYPE_CONTEXT, GelContextClass))

#define GEL_IS_CONTEXT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEL_TYPE_CONTEXT))

#define GEL_IS_CONTEXT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GEL_TYPE_CONTEXT))

#define GEL_CONTEXT_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GEL_TYPE_CONTEXT, GelContextClass))


GType gel_context_get_type(void);

typedef struct _GelContextPrivate GelContextPrivate;

typedef struct _GelContext
{
    GObject parent_instance;
    GelContextPrivate *priv;
} GelContext;

typedef struct _GelContextClass
{
    GObjectClass parent_class;
} GelContextClass;

GelContext* gel_context_new(void);
GelContext* gel_context_new_with_outer(GelContext *outer);
void gel_context_unref(GelContext *self);

GValue* gel_context_find_symbol(const GelContext *self, const gchar *name);
GelContext* gel_context_get_outer(const GelContext *self);

void gel_context_add_symbol(GelContext *self, const gchar *name, GValue *value);
void gel_context_add_object(GelContext *self, const gchar *name, GObject *obj);
void gel_context_add_default_symbols(GelContext *self);
void gel_context_remove_symbol(GelContext *self, const gchar *name);

gboolean gel_context_eval(GelContext *self,
                          const GValue *value, GValue *dest_value);
const GValue* gel_context_eval_value(GelContext *self,
                                     const GValue *value, GValue *dest_value);
gboolean gel_context_eval_params(GelContext *self, const gchar *func,
                                 GList **list, const gchar *format,
                                 guint *n_values, const GValue **values, ...);


#endif

