#ifndef GEL_TYPE_SYMBOL
#define GEL_TYPE_SYMBOL (gel_symbol_get_type())

#include <glib-object.h>

typedef struct _GelSymbol GelSymbol;

struct _GelSymbol {
    gchar *name;
    GValue *value; /*< unowned >*/
    GHashTable *owner; /*< unowned >*/
};

GType gel_symbol_get_type(void) G_GNUC_CONST;

GelSymbol *gel_symbol_new(const gchar *name);

GelSymbol *gel_symbol_copy(const GelSymbol *self);

void gel_symbol_free(GelSymbol *self);


#endif
