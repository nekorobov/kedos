#ifndef __PLATFORM_BCM2836__
#define __PLATFORM_BCM2836__

#include "bcm2836/mmio.h"
#include "bcm2836/delay.h"
#include "bcm2836/gpio.h"
#include "bcm2836/uart.h"
#include "bcm2836/interrupt.h"
#include "bcm2836/mbox.h"
#include "bcm2836/videocore.h"

#define REV_COUNT 4
#define RPI2 2
#define RPI3 3
#define UNKNOWN_BOARD -1

void board_init();

#endif
