/*===========================================================*/
/**
  @file		ftm_smd_char.h
  @brief		A char type driver of smd

  History:
  Date			Author		Comment
  04-16-2009		Paul Lo		created file

  @author Paul Lo(PaulCCLo@tp.cmcs.com.tw)
*/
/*===========================================================*/
#ifndef FTM_SMD_CHAR_H
#define FTM_SMD_CHAR_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mempool.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <mach/msm_smd.h>
#include <asm/atomic.h>

/* Size of the USB buffers used for read and write*/
//  FTM diag port buffer size. According to diagchar.h, modified to double size. Henry.Wang 2010.5.10+
//#define MAX_BUF 4096
#define MAX_BUF 8192
// FTM diag port buffer size. Henry.Wang, 2010.5.10-

/* Size of the buffer used for deframing a packet
  reveived from the PC tool*/
#define HDLC_MAX 4096

struct ftm_smd_char_dev {
	/* State for the char driver */
	unsigned int major;
	unsigned int minor;
	struct cdev *cdev;
	struct class *ftm_smd_class;
	struct mutex ftm_smd_mutex;
	struct mutex ftm_smd_write_mutex;
	wait_queue_head_t wait_q;
	int data_ready;
	
	smd_channel_t *ch;
	unsigned char *buf;
	unsigned char *hdlc_buf;
};

/*-------------------------------------------------------------------------*/
/*                         Define of HLDC                             */
/*-------------------------------------------------------------------------*/
#define ESC_CHAR     0x7D
#define CONTROL_CHAR 0x7E
#define ESC_MASK     0x20

enum ftm_smd_send_state_enum_type {
	FTM_SMD_STATE_START,
	FTM_SMD_STATE_BUSY,
	FTM_SMD_STATE_CRC1,
	FTM_SMD_STATE_CRC2,
	FTM_SMD_STATE_TERM,
	FTM_SMD_STATE_COMPLETE
};

struct ftm_smd_send_desc_type {
	const void *pkt;
	const void *last;	/* Address of last byte to send. */
	enum ftm_smd_send_state_enum_type state;
	unsigned char terminate;	/* True if this fragment
					   terminates the packet */
};

struct ftm_smd_hdlc_dest_type {
	void *dest;
	void *dest_last;
	/* Below: internal use only */
	uint16_t crc;
};

struct ftm_smd_hdlc_decode_type {
	uint8_t *src_ptr;
	unsigned int src_idx;
	unsigned int src_size;
	uint8_t *dest_ptr;
	unsigned int dest_idx;
	unsigned int dest_size;
	int escaping;

};

#endif /* end of FTM_SMD_CHAR_H*/

