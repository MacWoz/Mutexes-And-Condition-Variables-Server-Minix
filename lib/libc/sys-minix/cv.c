#include <lib.h>
#include <cv.h>
#include <string.h>
#include <errno.h>

endpoint_t get_cv_number() {
	endpoint_t CV;
    int result = minix_rs_lookup("cv", &CV);
    return CV;
}

int cs_lock(int mutex_id) {
	message m;
	endpoint_t CV = get_cv_number();
	m.m1_i1 = mutex_id;
	int result;
	while ((result = _syscall(CV, 1, &m)) == -1) {
                if (errno == EINTR) {
                    m.m1_i1 = mutex_id;
                    continue;
                }

                else return -1;
	}
	return result;
}
int cs_unlock(int mutex_id) {
	message m;
	endpoint_t CV = get_cv_number();
	m.m1_i1 = mutex_id;
	return _syscall(CV, 2, &m);
}
int cs_wait(int cond_var_id, int mutex_id) {
	message m;
	endpoint_t CV = get_cv_number();
	m.m1_i1 = mutex_id;
	m.m1_i2 = cond_var_id;
	int result = _syscall(CV, 3, &m);
	if (result == -1) {
		if (errno == EINTR) {
		    int lockRes = cs_lock(mutex_id);
		    if (lockRes == -1)
		        return -1;
		    else
		        return 0;
		}
        	else return -1;
	}
	return result;
}
int cs_broadcast(int cond_var_id) {
    message m;
	endpoint_t CV = get_cv_number();
    m.m1_i1 = cond_var_id;
    return _syscall(CV, 4, &m);
}
