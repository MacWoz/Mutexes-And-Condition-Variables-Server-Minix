#ifndef _CV_H_
#define _CV_H_

#include <lib.h>
#include <minix/endpoint.h>
#include <stdio.h>

endpoint_t get_cv_number();

int cs_lock(int);
int cs_unlock(int);
int cs_wait(int, int);
int cs_broadcast(int);

#endif	/* !_CV_H_ */
