#include <stdio.h>
#include <errno.h>

#include <lib.h>

int retrieve(int* pn) {
    message m;
    m.m_type = 109;
	int status = sendrec(PM_PROC_NR, &m);

    *pn = m.m1_i1;
    return status;
}

int main(){
	int r,v;
	r = retrieve(&v);
	printf("res: %d  val: %d\n",r,v);

}
