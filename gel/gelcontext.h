#ifndef GEL_CONTEXT_TYPE
#define GEL_CONTEXT_TYPE (gel_context_get_type())

#include <glib-object.h>


typedef struct _GelContext GelContext;

/**
 * GelFunction:
 * @closure: The #GClosure to which this function is assigned as marshal
 * @return_value: The #GValue to store the return value, may be #NULL
 * @n_param_values: The number of parameters
 * @param_values: An array of #GValue with the parameters.
 * @invocation_context: The #GelContext for which this function is invoked, may be #NULL
 * @user_data: user data passed during the call to #gel_context_insert_function
 *
 * This type is used as the marshal of native closures.
 * It is basically a #GClosureMarshal with its arguments used to pass specific information.
 * 
 */
typedef void (*GelFunction)(GClosure *closure, GValue *return_value,
                            guint n_param_values, GValue *param_values,
                            GelContext *invocation_context, gpointer user_data);

GelContext* gel_context_new(void);
GelContext* gel_context_new_with_outer(GelContext *outer);
GelContext* gel_context_dup(const GelContext *self);
void gel_context_free(GelContext *self);
GType gel_context_get_type(void) G_GNUC_CONST;

GValue* gel_context_lookup(const GelContext *self, const gchar *name);
GelContext* gel_context_get_outer(const GelContext *self);

void gel_context_insert(GelContext *self, const gchar *name, GValue *value);
void gel_context_insert_object(GelContext *self, const gchar *name,
                               GObject *object);
void gel_context_insert_function(GelContext *self, const gchar *name,
                                 GelFunction function, void *user_data);
gboolean gel_context_remove(GelContext *self, const gchar *name);

gboolean gel_context_eval(GelContext *self,
                          const GValue *value, GValue *dest);
const GValue* gel_context_eval_into_value(GelContext *self,
                                     const GValue *value, GValue *out_value);
gboolean gel_context_eval_params(GelContext *self, const gchar *func,
                                 GList **list, const gchar *format,
                                 guint *n_values, const GValue **values, ...);

gboolean gel_context_get_running(const GelContext *self);
void gel_context_set_running(GelContext *self, gboolean running);

#endif

