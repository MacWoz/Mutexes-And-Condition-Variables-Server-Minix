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
void handle_vm_notify(void);

int usedMutexes[1024] = {0};
bool isUsed[1024] = {false};
endpoint_t mutexOwners[1024] = {0};
queue* mutexQueue[1024] = {0};

waiters* waitersTable[1024] = {0};
bool isWaiterUsed[1024] = {false};

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

		if (call_type & NOTIFY_MESSAGE) {
            switch (who_e) {
                case VM_PROC_NR:
                    handle_vm_notify();
                    break;
                default:
                    printf("Ignoring notify() from %d\n.", who_e)
            }
            continue;
		}

		int mutex_id;
		int cvar_id;
		int result;
        printf("CV get %d %d from %d\n", r, call_type, who_e);
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

            default:
                printf("CV warning: got illegal request from %d\n", who_e);
                m.m_type = -EINVAL;
                result = EINVAL;
        }
        printf("OK, server got result %d\n", result);
        if (result != EDONTREPLY) {
            int endRes = send(who_e, &m);
            printf ("OK, sent reply, result %d\n", endRes);
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
            printf("Found position of mutex %d: %d\n", mutex_id, i);
            break;
        }
    }
    if (used) {
        if (mutexOwners[position] == who_e) /** Chce jeszcze raz ten sam mutex */
        {
            return -EPERM;
        }

        if (mutexQueue[position] == NULL) {
            printf("Created queue\n");              /** Jak nie ma jeszcze kolejki */
            mutexQueue[position] = createQueue(mutex_id);
        }
        printf("Enqueued process %d.\n", who_e);
        enqueue(who_e, mutexQueue[position]);
        int vm = vm_watch_exit(m.m_source);
        printf("OK, watch dla %d - result %d\n", who_e, vm);
        return EDONTREPLY;
    }
    else {
        printf("Mutex %d not used, placed at position%d\n", mutex_id, firstFreePosition);
        usedMutexes[firstFreePosition] = mutex_id;
        isUsed[firstFreePosition] = true;
        mutexOwners[firstFreePosition] = who_e;
        int vm = vm_watch_exit(m.m_source);
        printf("OK, watch dla %d - result %d\n", who_e, vm);
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
                    printf("Unlocking, %d first in queue to lock.\n", nextOwner);
                    if (isEmpty(mutexQueue[i])) {
                        printf("Queue is empty.\n");
                        destroyQueue(mutexQueue[i]);
                        mutexQueue[i] = NULL;
                    }
                    mutexOwners[i] = nextOwner;
                    message mess;
                    mess.m_type = 0;
                    send(nextOwner, &mess);
                    printf("sent response for lock.\n");
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
    for (i=0;i<MAX_MUTEXES_NUMBER;++i) {
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
                int lockResult = do_lock(waitersTable[i]->owned_mutexes[j]);
                printf("Locking %d on mutex %d, result %d\n", who_e, waitersTable[i]->owned_mutexes[j], lockResult);
                if (lockResult != EDONTREPLY) {
                    printf("Sent response from do_broadcast.\n");
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

void handle_vm_notify() {

}
