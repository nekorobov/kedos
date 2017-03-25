#include <bcm2836/platform.h>

int rpi2_rev[] = {
        0xa01040,
        0xa01041,
        0xa21041,
        0xa22042
};

int rpi3_rev[] = {
        0xa02082,
        0xa020a0,
        0xa22082,
        0xa32082
};

static uint32_t check_board (uint32_t revision) {
        uint32_t i;
        for (i = 0; i < REV_COUNT; i++) {
                if (rpi2_rev[i] == revision)
                        return RPI2;
                if (rpi3_rev[i] == revision)
                        return RPI3;
        }
        return UNKNOWN_BOARD;
}

uint32_t p[8] __attribute__ ((aligned (16)));

uint32_t set_board_id()
{
        p[0] = 7 * sizeof (uint32_t);
        p[1] = 0;
        p[2] = MBX_TAG_GET_BOARD_REVISION;
        p[3] = 4;
        p[4] = 0;
        p[5] = 0;
        p[6] = 0;

        mbox_write (8, (uint32_t)&p + GPU_MEM_BASE);
        mbox_read (8);
        return check_board (p[5]);
}

void board_init() {}
        
