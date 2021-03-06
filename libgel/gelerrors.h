#ifndef __GEL_DEBUG_H__
#define __GEL_DEBUG_H__

#include <glib-object.h>

#include <gelcontext.h>

void gel_error_needs_at_least_n_arguments(GelContext *context,
                                          const gchar *func, guint n);

void gel_error_needs_n_arguments(GelContext *context,
                                 const gchar *func, guint n);

void gel_error_no_such_property(GelContext *context,
                                const gchar *func, const gchar *prop_name);

void gel_error_value_not_of_type(GelContext *context, const gchar *func,
                                 const GValue *value, GType type);

void gel_error_unknown_symbol(GelContext *context,
                              const gchar *func, const gchar *symbol);

void gel_error_type_not_instantiatable(GelContext *context,
                                       const gchar *func, GType type);

void gel_error_invalid_value_for_property(GelContext *context,
                                          const gchar *func,
                                          const GValue *value,
                                          const GParamSpec *spec);

void gel_error_type_name_invalid(GelContext *context,
                                 const gchar *func, const gchar *name);

void gel_error_invalid_argument_name(GelContext *context,
                                     const gchar *func, const gchar *name);

void gel_error_index_out_of_bounds(GelContext *context,
                                   const gchar *func, gint index);

void gel_error_invalid_key(GelContext *context,
                           const gchar *func, GValue *key);

void gel_error_symbol_exists(GelContext *context,
                             const gchar *func, const gchar *name);

void gel_error_expected(GelContext *context,
                        const gchar *func, const gchar *s);

void gel_error_incompatible(GelContext *context, const gchar *f,
                            const GValue *v1, const GValue *v2);

#endif

