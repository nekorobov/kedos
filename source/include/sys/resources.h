#ifndef _RES_H_
#define _RES_H_

#include "kthread.h"

typedef uint32_t res_type_t;

#define RES_FIRST_TYPE_MAX  RES_HAN 
#define RES_SEC_TYPE_MAX    RES_GPU 
#define RES_THIRD_TYPE_MAX  RES_TTY
#define RES_FOURTH_TYPE_MAX 0

typedef struct ftable_t ftable;

typedef struct {
        pid_t pid;
        res_type_t type;
        void* data;
        ftable* tb;
} res_unit;

struct ftable_t {
		int (*res_init) (res_unit* unit);
		int (*res_free) (res_unit* unit);
		int (*res_give) (res_unit* unit);
		int (*res_get ) (res_unit* unit);
};

enum first_level_res {
        RES_CPU = 0x01,
        RES_MEM,
        RES_DEV,
        RES_HAN // Interrupt handler
};

enum sec_level_res {
        RES_PAGE = 0x01,
        RES_GPIO,
        RES_UART,
        RES_SD,
        RES_USB,
        RES_GPU
};

enum third_level_res {
        RES_FS = 0x01,
        RES_TTY
}; 

static inline res_type_t res_compose(uint8_t first_level, uint8_t sec_level, 
                                     uint8_t third_level, uint8_t four_level)
{
        return (four_level << 24 | third_level << 16 | sec_level << 8 | first_level);
}

int res_table_init();
res_unit** res_table_get();

int res_add (res_unit* res);
int res_del (int rd);

int res_findt(res_type_t type, int start);
int res_findp(pid_t pid, int start);
int res_findtp(res_type_t type, pid_t pid, int start);

int res_get   (res_type_t type, pid_t src, sflag_t fl);
int res_give  (int rd, pid_t dest, sflag_t fl);

int msg_equal (msg_t* msg, int param1, int param2);

#define GET_RES(rd, type, from, flag) \
	do { \
		msg_t msg; \
		msg.type = MSG_GET_TYPE; \
		msg.param1 = type; \
		msg.param2 = from; \
		msg.fl = flag; \
		msg.sender = cur_thread->pid; \
		rd = send (0, &msg, sizeof (msg)); \
	} while(0)

#define GIVE_RES(rd, type, src, flag) \
	do { \
		msg_t msg; \
		msg.type = MSG_GIVE_TYPE; \
		msg.param1 = rd; \
		msg.param2 = src; \
		msg.fl = flag; \
		msg.sender = cur_thread->pid; \
		send (0, &msg, sizeof (msg)); \
	} while(0)

#endif
