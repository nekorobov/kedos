#ifndef __RINGBUFFER__
#define __RINGBUFFER__

#include "lib/nostdlib.h"
#include "lib/nostdio.h"

typedef struct rdata_t { 
	void*   data;
	size_t  size;
} rdata;

#define RING_BUFFER_SIZE 32 * sizeof (rdata)

typedef struct rbuffer_t {
	sflag_t flags;  //1st bit - buffer is full
			//2nd bit - buffer is empty
	void*   data;
        void*   read_end;
        void*   write_end;
        size_t  size;
} rbuffer;

enum msg_type {
        ZOMBIE,
        MSG_GET_TYPE,
	MSG_GIVE_TYPE
};

typedef struct {
	enum msg_type   type;
        int32_t         param1;
        int32_t         param2;
        size_t          size;
        sflag_t  	fl;
	int 		sender;
} msg_t;

rbuffer* create_rbuffer (sflag_t FLAGS, size_t size);

void free_rbuffer (rbuffer* buffer);

void dump_rbuffer (rbuffer* buffer, void (*data_dump)(void* data));

int8_t write_data (rbuffer* buffer, void* data, size_t size);

int8_t read_data (rbuffer* buffer, void* data);

msg_t* find_msg (rbuffer* buffer, enum msg_type mtype, int32_t param1, int32_t param2, msg_t* msg);

#endif
