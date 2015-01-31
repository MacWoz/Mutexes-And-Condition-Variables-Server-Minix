#ifndef _PROTO_H
#define _PROTO_H

/* Function prototypes. */

/* main.c */
int main(int argc, char* argv[]);

/* functions.c */
int do_lock(void);
int do_unlock(void);
int do_wait(void);
int do_broadcast(void);

#endif
