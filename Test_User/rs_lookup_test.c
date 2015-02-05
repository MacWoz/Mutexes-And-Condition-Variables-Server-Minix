#include <sys/cdefs.h>
#include <stdio.h>
#include <lib.h>
#include <minix/rs.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <errno.h>

int main() {
	int CV;
	endpoint_t val = minix_rs_lookup("cv", &CV);
	printf("result %d, GOT val %d\n", val, CV);
	message m;
	m.m_type = 1;
	printf("Sendrec:\n");
	int status = sendrec(CV, &m);
	printf("%d\n%d\n", status, m.m_type);
}
