#include "sys/kthread.h"

kthread* cur_thread = NULL;
node_head* thread_head = NULL;
active_head* th_active_head = NULL;
pid_t cur_pid = 1; //kernel has pid 0

extern reg_t* _get_stack_pointer();

static kthread* kthread_init (sflag_t flags, pid_t pid, void* func, enum schedule_type stype) {
	kthread* thread = (kthread*) kcalloc (1, sizeof (kthread));

	if (thread == NULL) {
#ifdef DEBUG
	kprint ("Can't allocate thread\r\n");
#endif
		return NULL;
	}

	thread->flags = flags; 
	thread->pid = pid;
	thread->stack_pointer = (reg_t) phys_page_alloc (THREAD_PAGE_COUNT, pid) 
                        + PAGE_SIZE - sizeof(dword);
	
	//For protection from corruption next memory page

	if (thread->stack_pointer == 0) {
#ifdef DEBUG
	kprint ("Can't allocate thread stack\r\n");
#endif
		kfree (thread);
		return NULL;
	}

	thread->stack_base = thread->stack_pointer;	
	thread->buffer = create_rbuffer (0, RING_BUFFER_SIZE);

	if (thread->buffer == NULL) {
#ifdef DEBUG
	kprint ("Can't allocate thread rbuffer\r\n");
#endif
		phys_page_free ((void *) thread->stack_pointer);
		kfree (thread);
		return NULL;
	}

	thread->program_counter = (reg_t) func;
	thread->sched_type = stype;
	init_dl_node (&thread->node);
	init_dl_node (&thread->active);
	if (!cur_thread)
		cur_thread = thread;

	asm volatile (	"mov %%r1, %%sp\t\n"
			"mov %%sp, %0\t\n"
			"mov %%r0, %1\t\n"
			"push {%%r0-%%r11}\t\n"
			"push {%%r0}\t\n"
			"mrs %%r0, cpsr\t\n"
			"push {%%r0}\t\n" 
			"mov %%sp, %%r1\t\n"
			::"r"(thread->stack_base), "r"(thread->program_counter): "memory", "%r0", "%r1", "%sp");

	thread->stack_pointer -= 14 * ARCH_BITS / 8;
	return thread;
}

static int kthread_delete (kthread* thread) {
	if (thread == NULL) {
		kprint ("Incorrect thread address");
		errno = ENOMEM;
		return -1;
	}
	phys_page_free ((void *)thread->stack_base);
	free_rbuffer (thread->buffer);
	delete_node (&thread->node);
	delete_node (&thread->active);
	kfree (thread);
	return 0;
}

#ifdef DEBUG
void kthread_dump (kthread* thread) {
	kprint ("Thread #%d\r\n SB : 0x%x\r\n SP : 0x%x\r\n PC : 0x%x\r\n", 
		thread->pid, thread->stack_base, thread->stack_pointer, thread->program_counter);
}
#endif

int kthread_list_init () {
	thread_head = (node_head*) kcalloc (1, sizeof (node_head));

	if (!thread_head) {
#ifdef DEBUG
		kprint ("Can't allocate thread list\r\n");
#endif
		return -1;
	}

	node_init (thread_head);
	th_active_head = (active_head*) kcalloc (1, sizeof (active_head));

	if (!th_active_head) {
#ifdef DEBUG
		kprint ("Can't allocate active thread list\r\n");
#endif
		kfree (thread_head);
		return -1;
	}

	active_init (th_active_head);
	return 0;
}

int kthread_list_delete () {
        if (thread_head == NULL) {
	        kprint ("Thread list doesn't exist\r\n");
		errno = ENOMEM;
		return -1;
	}
	kthread* i = NULL;
	for (i = node_head_first(thread_head);
        	node_is_last(thread_head, i); i = node_next(i)) {
        	kthread_delete (i);
    	}
	kfree (thread_head);
	kfree (th_active_head);
	return 0;
}

pid_t add_kthread (sflag_t flags, void* func, enum schedule_type stype) {
	if (thread_head == NULL) {
		kprint ("Thread list doesn't exist\r\n");
		errno = ENOMEM;
		return 0;
	}
	if (th_active_head == NULL) {
		kprint ("Thread active list doesn't exist\r\n");
		errno = ENOMEM;
		return 0;
	}
	kthread* thread = kthread_init (flags, cur_pid, func, stype);
	if (!thread) {
#ifdef DEBUG
		kprint ("Can't add kthread\r\n");
#endif
		return 0;
	}

	node_add_tail (thread_head, thread);
	active_add_tail (th_active_head, thread);
	cur_pid++;
	return thread->pid;
}

int delete_kthread (pid_t pid) {
	if (thread_head == NULL) {
		kprint ("Thread list doesn't exist\r\n");
		errno = ENOMEM;
		return -1;
	}

	if (th_active_head == NULL) {
		kprint ("Thread active list doesn't exist\r\n");
		errno = ENOMEM;
		return -1;
	}

	kthread* thread = find_thread_pid (pid);
	kthread_delete (thread);
	return 0;
}

int kthread_list_dump () {
	if (thread_head == NULL) {
		kprint ("List is empty\r\n");
		return 0;
	}
	kprint ("\r\nThread list: \r\n");
	for (kthread *i = node_head_first(thread_head);
        	node_is_last(thread_head, i); i = node_next(i)) {
		kthread_dump (i);        	
    	}
        return 0;
}

kthread* find_thread_pid (pid_t pid) {
	kthread* i = NULL;
	for (i = node_head_first(thread_head);
        	node_is_last(thread_head, i); i = node_next(i)) {
        	if (i->pid == pid)
			return i;
    	}
#ifdef DEBUG
	kprint ("There are no thread with pid = %d\r\n", pid);
#endif
	return NULL;
}

extern void _set_sp(reg_t sp);

void __attribute__((naked())) run() {
	thread_entry (cur_thread->stack_pointer, cur_thread->program_counter);
}



