#ifndef _INC_H
#define _INC_H

#define _POSIX_SOURCE	1
#define _MINIX        	1
#define _SYSTEM		    1

#define MUTEX_LOCK      1
#define MUTEX_UNLOCK    2
#define CS_WAIT         3
#define CS_BROADCAST    4

#define MAX_MUTEXES_NUMBER 1024

#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/config.h>
#include <minix/endpoint.h>
#include <minix/sysutil.h>
#include <minix/const.h>
#include <minix/type.h>
#include <minix/syslib.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <machine/vm.h>
#include <machine/vmparam.h>
#include <sys/vm.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

EXTERN endpoint_t who_e;
EXTERN int call_type;

#endif
