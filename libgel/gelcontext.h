#ifndef GEL_TYPE_CONTEXT
#define GEL_TYPE_CONTEXT (gel_context_get_type())

#include <glib-object.h>


/**
 * GEL_CONTEXT_ERROR:
 *
 * Error domain for errors reported by #gel_context_eval.
 * Errors in this domain will be from the #GelContextError enumeration.
 * See #GError for information on error domains.
 */

#define GEL_CONTEXT_ERROR (gel_context_error_quark())

#ifndef __GTK_DOC_IGNORE__
GQuark gel_context_error_quark(void);
#endif

/**
 * GelContextError:
 * @GEL_CONTEXT_ERROR_ARGUMENTS: wrong arguments
 * @GEL_CONTEXT_ERROR_UNKNOWN_SYMBOL: unknown symbol
 * @GEL_CONTEXT_ERROR_SYMBOL_EXISTS: symbol exists and canot be redefined
 * @GEL_CONTEXT_ERROR_TYPE: wrong value type
 * @GEL_CONTEXT_ERROR_PROPERTY: wrong property
 * @GEL_CONTEXT_ERROR_INDEX: invalid index
 * @GEL_CONTEXT_ERROR_KEY: invalid key
 *
 * Error codes reported by #gel_context_eval
 */

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
GType gel_context_get_type(void) G_GNUC_CONST;

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

GValue* gel_context_lookup(const GelContext *self, const gchar *name);
GList* gel_context_get_variables(const GelContext *self);

void gel_context_insert(GelContext *self, const gchar *name, GValue *value);
void gel_context_insert_object(GelContext *self, const gchar *name,
                               GObject *object);
void gel_context_insert_function(GelContext *self, const gchar *name,
                                 GelFunction function, void *user_data);

gboolean gel_context_remove(GelContext *self, const gchar *name);

gboolean gel_context_eval(GelContext *self, const GValue *value, GValue *dest,
                          GError **error);

gboolean gel_context_error(const GelContext* self);
void gel_context_clear_error(GelContext* self);

#endif

