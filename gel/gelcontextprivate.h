#ifndef __GEL_CONTEXT_PRIVATE_H__
#define __GEL_CONTEXT_PRIVATE_H__

#include <gelcontext.h>


struct _GelContext
{
    GHashTable *symbols;
    GelContext *outer;
    gboolean running;
};


#endif

