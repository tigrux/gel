#ifndef __GEL_CLOSURE_PRIVATE_H__
#define __GEL_CLOSURE_PRIVATE_H__


typedef struct _GelClosure GelClosure;

GelContext* gel_closure_get_context(GelClosure *self);
void gel_closure_set_context(GelClosure *self, GelContext *context);


#endif
