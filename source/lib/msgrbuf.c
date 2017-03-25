/* Copyright (C) 2017 by Klim Kireev <edvmartell@gmail.com> <https://github.com/proffK>

*   This file is part of kedos.

*   kedos is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.

*   kedos is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with kedos.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "lib/msgrbuf.h"


rbuffer* create_rbuffer (sflag_t FLAGS, size_t size) {
	int i = 0;
	int j = 0;
	if (size < 1) {
#ifdef DEBUG
		kprint ("Incorrect ringbuffer size. It must be larger then 0.\r\n");
#endif
		return NULL;
	}

	rbuffer* buffer = (rbuffer *) kcalloc (1, sizeof (rbuffer));

	if (!buffer) {
#ifdef DEBUG
		kprint ("Can't allocate rbuffer\r\n");
#endif
		return NULL;
	}

	buffer->flags = FLAGS | RBUFFER_IS_EMPTY;
	buffer->size = size;
        buffer->data = kcalloc (size, sizeof (byte));
	
        if (!buffer->data) {
#ifdef DEBUG
		kprint ("Can't allocate data\r\n");
#endif
		kfree (buffer);
                return NULL;
	}

        buffer->write_end = buffer->data;
        buffer->read_end = buffer->data;

	return buffer;
}

void free_rbuffer (rbuffer* buffer) {

	if (buffer == NULL) {
		errno = ENOMEM;
#ifdef DEBUG
	kprint ("Incorrect buffer address\r\n");
#endif
		return;
	}

        kfree (buffer->data);	
	kfree (buffer);
}


void dump_rbuffer (rbuffer* buffer, void (*data_dump)(void* data)) {

	if (buffer == NULL) {
		errno = ENOMEM;
#ifdef DEBUG
	kprint ("Incorrect buffer address\r\n");
#endif
		return;
	}
        
        //TODO
	kprint ("Is full      : %d\r\n", FLAG_DUMP(buffer->flags & RBUFFER_IS_FULL));
	kprint ("Is empty     : %d\r\n", FLAG_DUMP(buffer->flags & RBUFFER_IS_EMPTY));
}

int8_t write_data (rbuffer* buffer, void* data, size_t size) {
	if (buffer == NULL) {
		errno = ENOMEM;
#ifdef DEBUG
	kprint ("Incorrect buffer address\r\n");
#endif
		return ENOMEM;
	}
	
	if (buffer->write_end == buffer->read_end && !(buffer->flags & RBUFFER_IS_EMPTY)) 
		buffer->flags = buffer->flags | RBUFFER_IS_FULL;
        
        if (buffer->flags & RBUFFER_IS_FULL) {
#ifdef DEBUG
	kprint ("Can't write data. Buffer is full. Lose data...\r\n");
#endif
		return -1;
	}


	rdata tmp;
        tmp.data = (void *) kcalloc (size, sizeof (byte));
	
	if (!tmp.data) { 
#ifdef DEBUG
	kprint ("Can't allocate data in rbuffer\r\n");
#endif
		return -1;
	}

        tmp.size = size;
	memcpy (data, tmp.data, size);
        memcpy (buffer->write_end, &tmp, sizeof (rdata));

        buffer->write_end += sizeof (rdata);

        if (buffer->write_end == buffer->data + buffer->size) 
                buffer->write_end = buffer->data;

	if (buffer->flags & RBUFFER_IS_EMPTY)
		buffer->flags = buffer->flags & (~RBUFFER_IS_EMPTY);

	return 0;
}

int8_t read_data (rbuffer* buffer, void* data) {
	if (buffer == NULL) {
		errno = ENOMEM;
#ifdef DEBUG
	kprint ("Incorrect buffer address\r\n");
#endif
		return ENOMEM;
	}

	if (buffer->flags & RBUFFER_IS_EMPTY) {
#ifdef DEBUG
	kprint ("Can't read from empty buffer\r\n");
#endif
		return -1;
	}

        while (((msg_t *)((rdata*)buffer->read_end)->data)->type == ZOMBIE) {

                kfree (((rdata*)buffer->read_end)->data);
                buffer->read_end += sizeof (rdata);

                if (buffer->data + buffer->size == buffer->read_end)
                        buffer->read_end = buffer->data;

                if (buffer->read_end == buffer->write_end) {
                        buffer->flags |= RBUFFER_IS_EMPTY;
                        return -1;
                }
        }

        memcpy (((rdata*)buffer->read_end)->data, data, *(size_t *) buffer->read_end);
        kfree (((rdata*)buffer->read_end)->data);
        buffer->read_end += sizeof (rdata);

        if (buffer->data + buffer->size == buffer->read_end)
                buffer->read_end = buffer->data;

        if (buffer->read_end == buffer->write_end)
                buffer->flags |= RBUFFER_IS_EMPTY;

	return 0;
}

static int msg_equal (void* data, enum msg_type mtype, int param1, int param2) {
	msg_t* message = (msg_t*) data;
	return (        mtype  == message->type    &&
			param1 == message->param1  &&
			param2 == message->param2);
}

msg_t* find_msg (rbuffer* buffer, enum msg_type mtype, int32_t param1, int32_t param2, msg_t* msg) {
	
        if (buffer == NULL) {
		errno = ENOMEM;
#ifdef DEBUG
		kprint ("Incorrect buffer address\r\n");
#endif
		return NULL;
	}

        void* bufr = buffer->read_end;
        void* bufw = buffer->write_end;
        rdata tmp;
	
        if (buffer->flags & RBUFFER_IS_EMPTY) {
#ifdef DEBUG
		kprint ("Can't read from empty buffer\r\n");
#endif
		return NULL;
	}

        while (1) {
                if (msg_equal (((rdata *) bufr)->data, mtype, param1, param2)) {
                        memcpy (((rdata *) bufr)->data, msg, *(size_t *) bufr);
                        ((msg_t *)(((rdata *) bufr)->data))->type = ZOMBIE;
                        return msg;
                }
                bufr += sizeof (rdata);

                if (bufr == buffer->data + buffer->size)
                        bufr = buffer->data;
                if (bufr == bufw)
                        break;
        }

	return NULL;
}




	
