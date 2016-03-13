/**
 * @file bcm2835_vc.c
 *
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface\n
 * ARM to VC
 *
 */
/* Copyright (C) 2015, 2016 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Modified by Kireev Klim for kedos.

#include <bcm2836/cache.h>
#include <bcm2836/defines.h>
#include <bcm2836/mbox.h>
#include <bcm2836/videocore.h>

struct vc_msg_tag_uint32 {
	uint32_t tag_id;		///< the message id
	uint32_t buffer_size;	///< size of the buffer (which in this case is always 8 bytes)
	uint32_t data_size;		///< amount of data being sent or received
	uint32_t dev_id;		///< the ID of the clock/voltage to get or set
	uint32_t val;			///< the value (e.g. rate (in Hz)) to set
};

struct vc_msg_uint32 {
	uint32_t msg_size;				///< simply, sizeof(struct vc_msg)
	uint32_t request_code;			///< holds various information like the success and number of bytes returned (refer to mailboxes wiki)
	struct vc_msg_tag_uint32 tag;	///< the tag structure above to make
	uint32_t end_tag;				///< an end identifier, should be set to NULL
};

/**
 * @ingroup VideoCore
 *
 * @param tag_id
 * @param dev_id
 * @return
 */

inline static int32_t bcm2835_vc_get(const uint32_t tag_id, const uint32_t dev_id) {
	struct vc_msg_uint32 vc_msg  __attribute__((aligned(16)));

	vc_msg.msg_size = sizeof(struct vc_msg_uint32);
	vc_msg.request_code = 0;
	vc_msg.tag.tag_id = tag_id;
	vc_msg.tag.buffer_size = 8;
	vc_msg.tag.data_size = 4; /* we're just sending the clock ID which is one word long */
	vc_msg.tag.dev_id = dev_id;
	vc_msg.tag.val = 0;
	vc_msg.end_tag = 0;

#if defined (RPI2)
	clean_data_cache();
#endif
	dsb();

        mbox_flush();
	mbox_write(MBOX_PROP, 
                   GPU_MEM_BASE + (uint32_t)&vc_msg);
	mbox_read(MBOX_PROP);

#if defined (RPI2)
	invalidate_data_cache();
#endif
	dsb();

	if (vc_msg.request_code != MBOX_SUCCESS) {
		return -1;
	}

	if (vc_msg.tag.dev_id != dev_id) {
		return -1;
	}

	return (int32_t) vc_msg.tag.val;

}

/**
 * @ingroup VideoCore
 *
 * @param tag_id
 * @param dev_id
 * @param val
 * @return
 */
inline static int32_t bcm2835_vc_set(const uint32_t tag_id, const uint32_t dev_id, const uint32_t val) {
	struct  vc_msg_uint32 vc_msg  __attribute__((aligned(16)));

	vc_msg.msg_size = sizeof(struct vc_msg_uint32);
	vc_msg.request_code = 0;
	vc_msg.tag.tag_id = tag_id;
	vc_msg.tag.buffer_size = 8;
	vc_msg.tag.data_size = 8; /* we're sending the clock ID and the new rates which is a total of 2 words */
	vc_msg.tag.dev_id = dev_id;
	vc_msg.tag.val = val;
	vc_msg.end_tag = 0;

#if defined (RPI2)
	clean_data_cache();
#endif
	dsb();

	mbox_flush();
	mbox_write(MBOX_PROP, 
                   GPU_MEM_BASE + (uint32_t)&vc_msg);
	mbox_read(MBOX_PROP);

#if defined (RPI2)
	invalidate_data_cache();
#endif
	dsb();

	if (vc_msg.request_code != MBOX_SUCCESS) {
		return -1;
	}

	if (vc_msg.tag.dev_id != dev_id) {
		return -1;
	}

	return (int32_t) vc_msg.tag.val;
}

/**
 * @ingroup VideoCore
 *
 * @param clock_id
 *
 * @return rate (in Hz)
 */
int32_t bcm2835_vc_get_clock_rate(const uint32_t clock_id) {
	return bcm2835_vc_get(BCM2835_VC_TAG_GET_CLOCK_RATE, clock_id);
}

/**
 * @ingroup VideoCore
 *
 * @param clock_id
 *
 * @param clock_rate rate (in Hz)
 *
 * @return rate (in Hz). A rate of 0 is returned if the clock does not exist.
 */
int32_t bcm2835_vc_set_clock_rate(const uint32_t clock_id, const uint32_t clock_rate) {
	return bcm2835_vc_set(BCM2835_VC_TAG_SET_CLOCK_RATE, clock_id, clock_rate);
}

int32_t bcm2835_vc_get_temperature(void) {
	return bcm2835_vc_get(BCM2835_VC_TAG_GET_TEMP, 0);
}

int32_t bcm2835_vc_get_temperature_max(void) {
	return bcm2835_vc_get(BCM2835_VC_TAG_GET_MAX_TEMP, 0);
}

/**
 * @ingroup VideoCore
 *
 * @param dev_id
 *
 * @return
 *   Bit 0: 0=off, 1=on
 *   Bit 1: 0=device exists, 1=device does not exist
 *   Bits 2-31: reserved for future use
 */
int32_t bcm2835_vc_get_power_state(const uint32_t dev_id) {
	return (bcm2835_vc_get(BCM2835_VC_TAG_GET_POWER_STATE, dev_id) & 0x3);
}

/**
 * @ingroup VideoCore
 *
 * @param dev_id
 *
 * @param state
 *  Bit 0: 0=off, 1=on
 *  Bit 1: 0=do not wait, 1=wait
 *  Bits 2-31: reserved for future use (set to 0)
 *
 * @return
 *  Bit 0: 0=off, 1=on
 *  Bit 1: 0=device exists, 1=device does not exist
 *  Bits 2-31: reserved for future use
 *
 */
int32_t bcm2835_vc_set_power_state(const uint32_t dev_id, const uint32_t state) {
	return bcm2835_vc_set(BCM2835_VC_TAG_SET_POWER_STATE, dev_id, state);
}

struct vc_msg_tag_board_mac_address {
	uint32_t tag_id;			///< the message id
	uint32_t buffer_size;		///< size of the buffer (which in this case is always 8 bytes)
	uint32_t data_size;			///< amount of data being sent or received
	uint8_t  mac_address[6];	///< MAC address

};

struct vc_msg_board_mac_address {
	uint32_t msg_size;							///< simply, sizeof(struct vc_msg)
	uint32_t request_code;						///< holds various information like the success and number of bytes returned (refer to mailboxes wiki)
	struct vc_msg_tag_board_mac_address tag;	///< the tag structure above to make
	uint32_t end_tag;							///< an end identifier, should be set to NULL
};

/**
 * @ingroup VideoCore
 *
 * @param mac_address
 * @return
 */
int32_t bcm2835_vc_get_board_mac_address(uint8_t *mac_address) {
	struct vc_msg_board_mac_address vc_msg __attribute__((aligned(16)));

	vc_msg.msg_size = sizeof(struct vc_msg_board_mac_address);
	vc_msg.request_code = 0;
	vc_msg.tag.tag_id = BCM2835_VC_TAG_GET_BOARD_MAC_ADDRESS;
	vc_msg.tag.buffer_size = 6;
	vc_msg.tag.data_size = 0;
	vc_msg.tag.mac_address[0] = 0;
	vc_msg.end_tag = 0;

#if defined (RPI2)
	clean_data_cache();
#endif
	dsb();

	mbox_flush();
	mbox_write(MBOX_PROP, 
                   GPU_MEM_BASE + (uint32_t)&vc_msg);
	mbox_read(MBOX_PROP);

#if defined (RPI2)
	invalidate_data_cache();
#endif
	dsb();

	if (vc_msg.request_code != MBOX_SUCCESS) {
		mac_address[0] = 0;
		mac_address[1] = 0;
		mac_address[2] = 0;
		mac_address[3] = 0;
		mac_address[4] = 0;
		mac_address[5] = 0;
		return -1;
	}

	mac_address[0] = vc_msg.tag.mac_address[0];
	mac_address[1] = vc_msg.tag.mac_address[1];
	mac_address[2] = vc_msg.tag.mac_address[2];
	mac_address[3] = vc_msg.tag.mac_address[3];
	mac_address[4] = vc_msg.tag.mac_address[4];
	mac_address[5] = vc_msg.tag.mac_address[5];

	return 0;
}

struct vc_msg_tag_uint32_t {
	uint32_t tag_id;			///< the message id
	uint32_t buffer_size;		///< size of the buffer (which in this case is always 8 bytes)
	uint32_t data_size;			///< amount of data being sent or received
	uint32_t value;				///<

};

struct vc_msg_uint32_t {
	uint32_t msg_size;					///< simply, sizeof(struct vc_msg)
	uint32_t request_code;				///< holds various information like the success and number of bytes returned (refer to mailboxes wiki)
	struct vc_msg_tag_uint32_t tag;		///< the tag structure above to make
	uint32_t end_tag;					///< an end identifier, should be set to NULL
};

/**
 * @ingroup VideoCore
 *
 * @param tag_id
 * @return
 */
inline static int32_t bcm2835_vc_get_uint32_t(uint32_t tag_id) {
	struct vc_msg_uint32_t vc_msg __attribute__((aligned(16)));

	vc_msg.msg_size = sizeof(struct vc_msg_uint32_t);
	vc_msg.request_code = 0;
	vc_msg.tag.tag_id = tag_id;
	vc_msg.tag.buffer_size = 4;
	vc_msg.tag.data_size = 0;
	vc_msg.tag.value = 0;
	vc_msg.end_tag = 0;

#if defined (RPI2)
	clean_data_cache();
#endif
	dsb();

	mbox_flush();
	mbox_write(MBOX_PROP, 
                   GPU_MEM_BASE + (uint32_t)&vc_msg);
        mbox_read(MBOX_PROP);

#if defined (RPI2)
	invalidate_data_cache();
#endif
	dsb();

	if (vc_msg.request_code != MBOX_SUCCESS) {
		return -1;
	}

	return (int32_t) vc_msg.tag.value;
}

/**
 * @ingroup VideoCore
 *
 * @return
 */
int32_t bcm2835_vc_get_get_firmware_revision(void) {
	return bcm2835_vc_get_uint32_t(BCM2835_VC_TAG_GET_FIRMWARE_REV);
}

/**
 * @ingroup VideoCore
 *
 * @return
 */
int32_t bcm2835_vc_get_get_board_model(void) {
	return bcm2835_vc_get_uint32_t(BCM2835_VC_TAG_GET_BOARD_MODEL);
}

/**
 *
 * @return
 */
int32_t bcm2835_vc_get_get_board_revision(void) {
	return bcm2835_vc_get_uint32_t(BCM2835_VC_TAG_GET_BOARD_REV);
}

struct vc_msg_tag_ram {
	uint32_t tag_id;			///< the message id
	uint32_t buffer_size;		///< size of the buffer (which in this case is always 8 bytes)
	uint32_t data_size;			///< amount of data being sent or received
	uint32_t base_address;		///< base address in bytes
	uint32_t size;				///< size in bytes
};

struct vc_msg_board_ram {
	uint32_t msg_size;							///< simply, sizeof(struct vc_msg)
	uint32_t request_code;						///< holds various information like the success and number of bytes returned (refer to mailboxes wiki)
	struct vc_msg_tag_ram tag;					///< the tag structure above to make
	uint32_t end_tag;							///< an end identifier, should be set to NULL
};

/**
 * @ingroup VideoCore
 *
 * @param mac_address
 * @return
 */
int32_t bcm2835_vc_get_memory(const uint32_t tag_id) {
	struct vc_msg_board_ram vc_msg __attribute__((aligned(16)));

	if ((tag_id != BCM2835_VC_TAG_GET_ARM_MEMORY) 
             && (tag_id != BCM2835_VC_TAG_GET_VC_MEMORY)) {
		return -1;
	}

	vc_msg.msg_size = sizeof(struct vc_msg_board_ram);
	vc_msg.request_code = 0;
	vc_msg.tag.tag_id = tag_id;
	vc_msg.tag.buffer_size = 8;
	vc_msg.tag.data_size = 0;
	vc_msg.tag.base_address = 0;
	vc_msg.end_tag = 0;

#if defined (RPI2)
	clean_data_cache();
#endif
	dsb();

	mbox_flush();
	mbox_write(MBOX_PROP, 
                   GPU_MEM_BASE + (uint32_t)&vc_msg);
	mbox_read(MBOX_PROP);

#if defined (RPI2)
	invalidate_data_cache();
#endif
	dsb();

	if (vc_msg.request_code != MBOX_SUCCESS) {
		return -1;
	}

	return (int32_t)vc_msg.tag.size;
}
