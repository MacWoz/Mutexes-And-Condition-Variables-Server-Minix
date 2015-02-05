#include "inc.h"
#include "waiters.h"
#include "queue.h"

typedef int bool;
const bool false = 0;
const bool true = 1;

int call_type;
endpoint_t who_e;

int do_lock(int);
int do_unlock(int);
int do_wait(int, int);
int do_broadcast(int);

void send_eintr(endpoint_t proc);
void remove_proc(endpoint_t proc);

int usedMutexes[MAX_MUTEXES_NUMBER] = {0};
bool isUsed[MAX_MUTEXES_NUMBER] = {false};
endpoint_t mutexOwners[MAX_MUTEXES_NUMBER] = {0};
queue* mutexQueue[MAX_MUTEXES_NUMBER] = {0};

waiters* waitersTable[MAX_CVS_NUMBER] = {0};
bool isWaiterUsed[MAX_CVS_NUMBER] = {false};

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init_fresh(int type, sef_init_info_t *info);
static void sef_cb_signal_handler(int signo);

int main(int argc, char* argv[])
{
    message m;
    env_setargs(argc, argv);
	sef_local_startup();

	while (true) {
        int r;
        if ((r = sef_receive(ANY, &m)) != OK) {
            printf("Communication failed!\n");
        }
		who_e = m.m_source;
		call_type = m.m_type;

		int mutex_id;
		int cvar_id;
		int result;
        endpoint_t proc;
        switch (call_type) {
            case MUTEX_LOCK :
                mutex_id = m.m1_i1;
                result = do_lock(mutex_id);
                m.m_type = result;
                break;

            case MUTEX_UNLOCK :
                mutex_id = m.m1_i1;
                result = do_unlock(mutex_id);
                m.m_type = result;
                break;

            case CS_WAIT :
                mutex_id = m.m1_i1;
                cvar_id = m.m1_i2;
                result = do_wait(cvar_id, mutex_id);
                m.m_type = result;
                break;

            case CS_BROADCAST :
                cvar_id = m.m1_i1;
                result = do_broadcast(cvar_id);
                m.m_type = result;
                break;

            case PM_SIGNAL_MESSAGE :
                ///if (m.m_source != PM_PROC_NR) break;
                proc = m.m1_i1;
                send_eintr(proc);
                result = EDONTREPLY;
                break;
            case PROCESS_TERMINATED:
                proc = m.m1_i1;
                endpoint_t old = who_e;
                who_e = proc;
                remove_proc(proc);
                who_e = old;
                result = EDONTREPLY;
                break;
            default:
                printf("CV warning: got illegal request from %d\n", who_e);
                m.m_type = -EINVAL;
                result = EINVAL;
        }
        if (result != EDONTREPLY) {
            int endRes = send(who_e, &m);
        }
	}
	/* Never gets here */
	return -1;
}

static void sef_local_startup()
{
    /* Register init callbacks. */
    sef_setcb_init_fresh(sef_cb_init_fresh);
    sef_setcb_init_restart(sef_cb_init_fresh);

    /* Register signal callbacks. */
    sef_setcb_signal_handler(sef_cb_signal_handler);
    /* Let SEF perform startup. */
    sef_startup();
}

static int sef_cb_init_fresh(int UNUSED(type), sef_init_info_t *UNUSED(info))
{
    return OK;
}

static void sef_cb_signal_handler(int signo) { }

int do_lock (int mutex_id) {
    printf("Locking mutex %d\n", mutex_id);
    int i;
    bool used = false;
    int position = -1;
    int firstFreePosition = -1;
    for (i=0;i<MAX_MUTEXES_NUMBER;++i) {
        if ((! isUsed[i]) && (firstFreePosition == -1))
            firstFreePosition = i;
        if ((usedMutexes[i] == mutex_id) && (isUsed[i])) {
            used = true;
            position = i;
            break;
        }
    }
    if (used) {
        if (mutexOwners[position] == who_e) /** Chce jeszcze raz ten sam mutex */
        {
            return -EPERM;
        }

        if (mutexQueue[position] == NULL) {            /** Jak nie ma jeszcze kolejki */
            mutexQueue[position] = createQueue(mutex_id);
        }
        enqueue(who_e, mutexQueue[position]);
        return EDONTREPLY;
    }
    else {
        printf("Mutex %d not used, placed at position%d\n", mutex_id, firstFreePosition);
        usedMutexes[firstFreePosition] = mutex_id;
        isUsed[firstFreePosition] = true;
        mutexOwners[firstFreePosition] = who_e;
        return OK;
    }
}

int do_unlock (int mutex_id) {
    printf("Unlocking mutex %d\n", mutex_id);
    int i;
    for (i=0;i<MAX_MUTEXES_NUMBER;++i) {
        if ((usedMutexes[i] == mutex_id) && (isUsed[i])) {
            if (mutexOwners[i] == who_e) {
                if (mutexQueue[i] == NULL) {
                    isUsed[i] = false;
                }
                else if (isEmpty(mutexQueue[i])) {
                    isUsed[i] = false;
                    destroyQueue(mutexQueue[i]);
                    mutexQueue[i] = NULL;
                }
                else {  /** Ktoś czeka w kolejce */
                    endpoint_t nextOwner = pop(mutexQueue[i]);
                    if (isEmpty(mutexQueue[i])) {
                        destroyQueue(mutexQueue[i]);
                        mutexQueue[i] = NULL;
                    }
                    mutexOwners[i] = nextOwner;
                    message mess;
                    mess.m_type = 0;
                    send(nextOwner, &mess);
                }
            }
            else
                return -EPERM;

            break;
        }
    }
    return OK;
}

int do_wait (int cvar_id, int mutex_id) {
    int mutex_owner = who_e;
    int i;
    bool owner = false;
    for (i=0;i<MAX_MUTEXES_NUMBER;++i) {
        if ((isUsed[i]) && (mutexOwners[i] == mutex_owner) && (usedMutexes[i] == mutex_id)){
            owner = true;
            break;
        }
    }
    if (!owner)
        return -EINVAL;

    do_unlock(mutex_id);
    int firstFreePosition = -1;
    for (i=0;i<MAX_CVS_NUMBER;++i) {
        if ((waitersTable[i] == NULL) && (firstFreePosition == -1))
            firstFreePosition = i;
        else if ((!isWaiterUsed[i]) && (firstFreePosition == -1))
            firstFreePosition = i;

        if ((isWaiterUsed[i]) && (waitersTable[i] != NULL) && (waitersTable[i]->cond_var_id == cvar_id)) {
            addToWaiters(waitersTable[i], mutex_owner, mutex_id);
            return EDONTREPLY;
        }
    }
    waitersTable[firstFreePosition] = createWaiters(cvar_id);
    isWaiterUsed[firstFreePosition] = true;
    addToWaiters(waitersTable[firstFreePosition], mutex_owner, mutex_id);
    return EDONTREPLY;
}

int do_broadcast (int cvar_id) {
    int i;
    endpoint_t caller = who_e;
    for (i=0;i<MAX_MUTEXES_NUMBER;++i) {
        if ((isWaiterUsed[i]) && (waitersTable[i] != NULL) && (waitersTable[i]->cond_var_id == cvar_id)) {
            int j;
            for (j=0;j<waitersTable[i]->size;++j) {
                who_e = waitersTable[i]->processes[j];
                if (who_e == -1)
                    continue;

                int lockResult = do_lock(waitersTable[i]->owned_mutexes[j]);
                if (lockResult != EDONTREPLY) {
                    message mess;
                    mess.m_type = 0;
                    send(who_e, &mess);
                }
            }
            who_e = caller;
            free(waitersTable[i]);
            waitersTable[i] = NULL;
            isWaiterUsed[i] = false;
            break;
        }
    }
    return OK;
}

void send_eintr(endpoint_t proc) {
    int i;
    for (i=0;i<MAX_MUTEXES_NUMBER;++i) {
        if ((mutexQueue[i] != NULL) && (! isEmpty(mutexQueue[i]))) {
            Node* node;
            for (node = mutexQueue[i]->head->next;node != mutexQueue[i]->tail;node = node->next) {
                if (node->proc_nr == proc) {
                    message mess;
                    mess.m_type = EINTR;
                    send(proc, &mess);
                    remove_from_queue(mutexQueue[i], node);
                    printf("sent EINTR to %d\n", proc);
                    if (isEmpty(mutexQueue[i])) {
                        free(mutexQueue[i]);
                        mutexQueue[i] = NULL;
                        printf("removed queue\n");
                    }
                    return ;
                }
            }
        }
    }

    for (i=0;i<MAX_CVS_NUMBER;++i) {
        if((waitersTable[i] != NULL) && (isWaiterUsed[i])) {
            int j;
            for (j=0;j<waitersTable[i]->size;++j) {
                if (waitersTable[i]->processes[j] == proc) {
                    message mess;
                    mess.m_type = EINTR;
                    send(proc, &mess);
                    waitersTable[i]->processes[j] = -1;
                    if (waitersTable[i]->size == 1) {
                        free (waitersTable[i]);
                        isWaiterUsed[i] = false;
                        waitersTable[i] = NULL;
                    }
                }
            }
        }
    }
}

void remove_proc(endpoint_t proc) {
    int i;
    for (i=0;i<MAX_MUTEXES_NUMBER;++i) {
        if (isUsed[i] && (mutexOwners[i] == proc)) {
            do_unlock(usedMutexes[i]);
        }
        if ((mutexQueue[i] != NULL) && (! isEmpty(mutexQueue[i]))) {
            Node* node;
            for (node = mutexQueue[i]->head->next;node != mutexQueue[i]->tail;node = node->next) {
                if (node->proc_nr == proc) {
                    remove_from_queue(mutexQueue[i], node);
                    printf("Removed from queue proc %d\n", proc);
                    if (isEmpty(mutexQueue[i])) {
                        free(mutexQueue[i]);
                        mutexQueue[i] = NULL;
                        printf("removed queue\n");
                    }
                }
            }
        }
    }

    for (i=0;i<MAX_CVS_NUMBER;++i) {
        if((waitersTable[i] != NULL) && (isWaiterUsed[i])) {
            int j;
            for (j=0;j<waitersTable[i]->size;++j) {
                if (waitersTable[i]->processes[j] == proc) {
                    waitersTable[i]->processes[j] = -1;
                    if (waitersTable[i]->size == 1) {
                        free (waitersTable[i]);
                        isWaiterUsed[i] = false;
                        waitersTable[i] = NULL;
                    }
                }
            }
        }
    }
}
