#ifndef _WAITERS_H
#define _WAITERS_H

#include "inc.h"

typedef struct {
    endpoint_t processes[256];
    int cond_var_id;
    int size;
} waiters;

waiters* createWaiters(int);

void addToWaiters(waiters* w, endpoint_t proc);

#endif
