#include <stdio.h>
#include <errno.h>

#include <lib.h>

int store(int n) {
    message m;
    m.m_type = 108;
    m.m1_i1 = n;
	int status = sendrec(PM_PROC_NR, &m);
	if(m.m_type < 0){
		errno = -m.m_type;
		return -1;
    }
    return 0;
}

int main(){
	int r;
	r = store(42);
	printf("res: %d \n",r);

}
