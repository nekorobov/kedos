#include <sys/resources.h>
#include <bcm2836/defines.h>
#include <lib/error.h>

static res_unit** res_table;

res_unit** res_table_get() {
        return res_table;
}

static inline int free_rd() {
        int i;
        for (i = 0; i < RES_TABLE_SIZE; ++i) {
                if (res_table[i] == NULL) {
                        return i;
                }
        }                 
        return -ENOMEM;
}

int res_table_init() {
        res_table = (res_unit**) kcalloc (RES_TABLE_SIZE, sizeof(res_unit*));
        if (res_table == NULL) 
                return -EINVAL;
        return 0;
}

int res_add(res_unit* res) {
        int rd;
        if (res == NULL || res_table == NULL) 
                return -EINVAL;
        rd = free_rd();
        if (rd >= 0) {
                res_table[rd] = res;
        }
        return rd;
}

int res_del(int rd) {
        if (!(res_table != NULL && rd < RES_TABLE_SIZE)) 
                return -EINVAL;
        res_table[rd] = NULL;
        return 0;
}

int res_findt(res_type_t type, int start) {
        if (start < 0 || res_table == NULL)
                return -EINVAL;
        while (start < RES_TABLE_SIZE) {
                if (res_table[start] != NULL && 
                    (res_table[start]->type == type)) {
                        return start;
                }
                start++;
        }
        return -EAGAIN;
}

int res_findp(pid_t pid, int start) {
        if (start < 0 || res_table == NULL)
                return -EINVAL;
        while (start < RES_TABLE_SIZE) {
                if (res_table[start] != NULL && 
                    (res_table[start]->pid == pid)) {
                        return start;
                }
                start++;
        }
        return -EAGAIN;
}

int res_findtp(res_type_t type, pid_t pid, int start) {
        if (start < 0 || res_table == NULL)
                return -EINVAL;
        while (start < RES_TABLE_SIZE) {
                if (res_table[start] != NULL && 
                    (res_table[start]->type == type) &&
                    (res_table[start]->pid == pid)) {
                        return start;
                }
                start++;
        }
        return -EAGAIN;
}


static void thread_exit_shell() { 
	thread_exit();
}

static void decompose_res (res_type_t res) {
	kprint ("    RESOURCE\r\n");
	kprint ("|0x%x|0x%x|0x%x|0x%x|\r\n", (res >> 24) & 0xff,
										 (res >> 16) & 0xff,
										 (res >> 8)  & 0xff,
										 (res)  & 0xff);
}

static int res_is_valid (res_type_t res) {
	return 	(((res)      & 0xff) <= RES_FIRST_TYPE_MAX &&
			((res >> 8)  & 0xff) <= RES_SEC_TYPE_MAX   &&
			((res >> 16) & 0xff) <= RES_THIRD_TYPE_MAX &&
			((res >> 24) & 0xff) <= RES_FIRST_TYPE_MAX);
}


int res_get (res_type_t res, pid_t src, sflag_t flag) {
	
	pid_t from;
	int rd;
	res_unit* ures;

        if (res_table == NULL) 
                return -EINVAL;
 
	if (res_is_valid (res) == 0) {
#ifdef DEBUG
		kprint ("Get: Unknown resource\r\n");
#endif
		return -EINVAL;
	}

#ifdef DEBUG
	decompose_res (res);
#endif

	rd = res_findt (res, 0);
	ures = res_table[rd];

	if (ures == NULL) {
#ifdef DEBUG
		kprint ("Get: Resource doesn't exist\r\n");
#endif
		return -EINVAL;
	}

	if ((flag & R_WAITFROM) != 0 && (flag & R_NONBLOCK) != 0) {
#ifdef DEBUG
		kprint ("Get: Can't get resources with WAITFROM and NONBLOCK at the same time\r\n");
#endif
		return -EINVAL;
	}

	if (ures->pid == cur_thread->pid) 
		return rd;

	if (flag & R_WAITFROM)
		from = src;
	else 
		from = GET_KERNEL_THREAD()->pid;

	if (from == cur_thread->pid) {
		if (ures->pid == from)
			return rd;
		else 
			return -EINVAL;
	}

	if ((flag & R_WAITFROM) && from != 0) {
		cur_thread->state = BLOCKED;
		thread_exit_shell();
	} else {
		while (ures->pid != from) {
			if ((flag & R_NONBLOCK) == 0) {
				cur_thread->state = BLOCKED;
				thread_exit_shell();
			} else {
#ifdef DEBUG
				kprint ("Get: Unavaliable resource\r\n");
#endif
				return -EACCES;
			}
		}
	}
	ures->pid = cur_thread->pid;
	return rd;
}


int res_give (int rd, pid_t dest, sflag_t flag) {

	res_unit* ures;
	pid_t to;
	pid_t wait;
	msg_t msg;
        msg_t* retp;

        if (!(res_table != NULL && rd < RES_TABLE_SIZE)) 
    	        return -EINVAL;
 
	ures = res_table[rd];

	if (ures == NULL) {
#ifdef DEBUG
		kprint ("Give: Resource doesn't exist\r\n");
#endif
		return -EINVAL;
	}

	if (ures->pid != cur_thread->pid) {
#ifdef DEBUG
		kprint ("Give: Unavaliable resource\r\n");
#endif
		return -EINVAL;
	}

	if (flag & R_SENDTO) 
		to = dest;
	else
		to = GET_KERNEL_THREAD()->pid;

	kthread* receiver = find_thread_pid (to);
	
	if (receiver == NULL) {
#ifdef DEBUG
		kprint ("Give: There are no thread with %d pid\r\n", to);
#endif
		return -EINVAL;
	}

	if ((flag & R_SENDTO) != 0) {
		do {
			retp = find_msg (GET_KERNEL_THREAD()->buffer, MSG_GET_TYPE, ures->type, to, &msg);
			kprint ("here %x\r\n", retp);
			if (retp == NULL) {
				cur_thread->state = BLOCKED;
				thread_exit_shell();
			}
		} while (retp == NULL);

	} else {
		do {
			retp = find_msg (GET_KERNEL_THREAD()->buffer, MSG_GET_TYPE, ures->type, GET_KERNEL_THREAD()->pid, &msg);
			if (retp != NULL) {
				wait = msg.sender;

				if (receiver->state == BLOCKED) {
					receiver->state = WAIT;
					active_add_tail (th_active_head, receiver);
					break;
				}
			}
		} while (retp != NULL);

		ures->pid = GET_KERNEL_THREAD()->pid;
        }

	if (receiver->state == BLOCKED) {
		receiver->state = WAIT;
		active_add_tail (th_active_head, receiver);
	}

	return 0;
}

