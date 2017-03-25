#include "test/test.h"
#include "bcm2836/gpio.h"
#include "lib/nostdlib.h"
#include "bcm2836/mmio.h"
#include "bcm2836/timer.h"

#define SEND(param, size, to) \
	do { \
		memcpy (param, msg, size); \
		send (to, msg, size); \
	} while (0)



static void sendf() {
	char msg[256];
	char* msg1 = "SEND THREAD TO NO BLOCK READ\r\n";
	char* msg2 = "SEND THREAD TO BLOCK READ\r\n";
	while (1) {
		SEND (msg1, strlen (msg1) + 1, 2);
		ssleep (0x60000);
		SEND (msg2, strlen (msg2) + 1, 4);
		ssleep (0x60000);
		receive (msg);
	}
}

static void try_readf() {	
	char msg[256];
	while (1) {
		while (try_receive (msg) >= 0)
			kprint ("%s", msg);
	}
}

static void blinkf() {
	res_type_t type = res_compose (RES_DEV, RES_GPIO, 0, 0);
	int prd;
	int on = 1;
	int off = 0;
	char msg[4];
	uint32_t volatile ra = 0;// mmio_read (GPFSEL4);
	ra = 1 << 3;
	//mmio_write (GPFSEL4, ra);

	*((volatile uint32_t*) GPFSEL2) = ra;
	ra = 1 << 21;
	GET_RES (prd, type, 0, 0);
	while (1) {	
		ssleep (0x60000);
		*((volatile uint32_t*) GPSET0) = ra;
		//mmio_write (GPSET1, ra);
		SEND(&on, 4, 5);
		ssleep (0x60000);
		//mmio_write (GPCLR1, ra);
		*((volatile uint32_t*) GPCLR0) = ra;
		SEND(&off, 4, 5);
	}
	GIVE_RES (prd, type, 0, 0);
}

static void readf() {
	char msg[256];
	while (1) {
		receive (msg);
		kprint ("%s", msg);
		SEND("", 1, 1);
	}		
}

static void led_info() {
	int mode;
	char* on_msg = "LED IS ON\r\n";
	char* off_msg = "LED IS OFF\r\n";
	char msg[256];
	while (1) {
		receive (&mode);
		switch (mode) {
			case 0: 
				SEND(off_msg, strlen (off_msg) + 1, 4);
				break;
			case 1:
				SEND(on_msg, strlen (on_msg) + 1, 4);
				break;
		}
	}	
}
	

int demonstrate() {
	add_kthread (0, sendf, 		SOFT);
	add_kthread (0, try_readf, 	SOFT);
	add_kthread (0, blinkf,		SOFT);
	add_kthread (0, readf, 		SOFT);
	add_kthread (0, led_info,	SOFT);
}
