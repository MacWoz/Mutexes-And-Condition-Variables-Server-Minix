#include "inc.h"
#include "waiters.h"

waiters* createWaiters(int cvar_id) {
    waiters* w = malloc(sizeof(waiters));
    w->cond_var_id = cvar_id;
    w->size = 0;
    return w;
}

void addToWaiters(waiters* w, endpoint_t proc) {
    (*w).processes[w->size] = proc;
    w->size += 1;
}
