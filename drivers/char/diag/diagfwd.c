/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/diagchar.h>
#include <mach/usbdiag.h>
#include <mach/msm_smd.h>
#include "diagmem.h"
#include "diagchar.h"
#include "diagfwd.h"
#include "diagchar_hdlc.h"
#include <linux/pm_runtime.h>
#include "../../../arch/arm/mach-msm/proc_comm.h"    //Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +
//+{PS3-RR-ON_DEVICE_QXDM-01
#include <mach/dbgcfgtool.h>
//PS3-RR-ON_DEVICE_QXDM-01}+

#include <mach/msm_smd.h>  //SW-2-5-1-MP-DbgCfgTool-03+

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 *[
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/rtc.h>


MODULE_DESCRIPTION("Diag Char Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
static DEFINE_SPINLOCK(smd_lock);
static DECLARE_WAIT_QUEUE_HEAD(diag_wait_queue);
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 *]


//SW-2-5-1-MP-DbgCfgTool-03+[
#define  APPS_MODE_ANDROID   0x1
#define  APPS_MODE_RECOVERY  0x2
#define  APPS_MODE_FTM       0x3
#define  APPS_MODE_UNKNOWN   0xFF
//SW-2-5-1-MP-DbgCfgTool-03+]

int diag_debug_buf_idx;
unsigned char diag_debug_buf[1024];
/* Number of maximum USB requests that the USB layer should handle at
   one time. */
#define MAX_DIAG_USB_REQUESTS 12
static unsigned int buf_tbl_size = 8; /*Number of entries in table of buffers */

#define CHK_OVERFLOW(bufStart, start, end, length) \
((bufStart <= start) && (end - start >= length)) ? 1 : 0

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
#define WRITE_NV_4719_TO_ENTER_RECOVERY 1
#ifdef WRITE_NV_4719_TO_ENTER_RECOVERY
#define NV_FTM_MODE_BOOT_COUNT_I    4719

unsigned int switch_efslog = 1;

static ssize_t write_nv4179(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int nv_value = 0;
    unsigned int NVdata[3] = {NV_FTM_MODE_BOOT_COUNT_I, 0x1, 0x0};	

    sscanf(buf, "%u\n", &nv_value);
	
    printk("paul %s: %d %u\n", __func__, count, nv_value);
    NVdata[1] = nv_value;
    fih_write_nv4719((unsigned *) NVdata);

    return count;
}
DEVICE_ATTR(boot2recovery, 0644, NULL, write_nv4179);
#endif

static ssize_t OnOffEFS(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int nv_value = 0;

    sscanf(buf, "%u\n", &nv_value);
    switch_efslog = nv_value;

    printk("paul %s: %d %u\n", __func__, count, switch_efslog);
    return count;
}
DEVICE_ATTR(turnonofffefslog, 0644, NULL, OnOffEFS);
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]

//FXPCAYM-87
extern char *diag_rsp_0c;
extern char *diag_rsp_63;
extern char *diag_rsp_1a;
extern char *diag_rsp_119a;
extern char *diag_rsp_119b;
extern unsigned char drop_0c_packet;
extern unsigned char drop_63_packet;
extern unsigned char drop_1a_packet;
/* Routines added for SLATE support FXPCAYM81 */
static int  diag_process_modem_pkt(unsigned char *buf, int len);
static void diag_process_user_pkt(unsigned char *data, unsigned len);

uint32_t diag_char_debug_mask = 0;
module_param_named(
    debug_mask, diag_char_debug_mask, uint, S_IRUGO | S_IWUSR | S_IWGRP
);


void __diag_smd_send_req(void)
{
	void *buf = NULL;
	int *in_busy_ptr = NULL;
	struct diag_request *write_ptr_modem = NULL;

	if (!driver->in_busy_1) {
		buf = driver->usb_buf_in_1;
		write_ptr_modem = driver->usb_write_ptr_1;
		in_busy_ptr = &(driver->in_busy_1);
	} else if (!driver->in_busy_2) {
		buf = driver->usb_buf_in_2;
		write_ptr_modem = driver->usb_write_ptr_2;
		in_busy_ptr = &(driver->in_busy_2);
	}

	if (driver->ch && buf) {
		int r = smd_read_avail(driver->ch);

		if (r > USB_MAX_IN_BUF) {
			if (r < MAX_BUF_SIZE) {
				printk(KERN_ALERT "\n diag: SMD sending in "
						   "packets upto %d bytes", r);
				buf = krealloc(buf, r, GFP_KERNEL);
			} else {
				printk(KERN_ALERT "\n diag: SMD sending in "
				"packets more than %d bytes", MAX_BUF_SIZE);
				return;
			}
		}
		if (r > 0) {
			if (!buf)
				printk(KERN_INFO "Out of diagmem for a9\n");
			else {
				APPEND_DEBUG('i');
				smd_read(driver->ch, buf, r);
				APPEND_DEBUG('j');
				write_ptr_modem->length = r;
				*in_busy_ptr = 1;
				diag_device_write(buf, MODEM_DATA,
							 write_ptr_modem);
			}
		}
	}
}

void __clear_in_busy(struct diag_request *write_ptr)
{
	if (write_ptr->buf == (void *)driver->usb_buf_in_1) {
		driver->in_busy_1 = 0;
	} else if (write_ptr->buf == (void *)driver->usb_buf_in_2) {
		driver->in_busy_2 = 0;
	}
}

int diag_device_write(void *buf, int proc_num, struct diag_request *write_ptr)
{
	int i, err = 0;

	if ((proc_num == MODEM_DATA) && ( *(char *)buf  == 0x0c) && (drop_0c_packet == 1) ) {
		memset(diag_rsp_0c, 0, USB_MAX_OUT_BUF);
		memcpy(diag_rsp_0c, buf, write_ptr->length); 
		printk(KERN_INFO "0x0C Buffer copied over BufferLength=%d, driver->logging_mode=%d\n", 
			   write_ptr->length, driver->logging_mode); 
		err = 0;
		__clear_in_busy(write_ptr);
		drop_0c_packet = 0;
		return err;
	}

	if ((proc_num == MODEM_DATA) && ( *(char *)buf  == 0x63) && (drop_63_packet == 1) ) {
		memset(diag_rsp_63, 0, USB_MAX_OUT_BUF);
		memcpy(diag_rsp_63, buf, write_ptr->length); 
		printk(KERN_INFO "0x63 Buffer copied over BufferLength=%d, driver->logging_mode=%d\n", 
			   write_ptr->length, driver->logging_mode); 
		err = 0;
        __clear_in_busy(write_ptr);
		drop_63_packet = 0;
		return err;
	}

	if ((proc_num == MODEM_DATA) && ( *(char *)buf  == 0x1a) && (drop_1a_packet == 1) ) {
		memset(diag_rsp_1a, 0, USB_MAX_OUT_BUF);
		memcpy(diag_rsp_1a, buf, write_ptr->length); 
		printk(KERN_INFO "0x1a Buffer copied over BufferLength=%d, driver->logging_mode=%d\n", 
			   write_ptr->length, driver->logging_mode); 
		err = 0;
        __clear_in_busy(write_ptr);
		drop_1a_packet = 0;
		return err;
	}


	if (driver->logging_mode == USB_MODE) {
		if (proc_num == APPS_DATA) {
			driver->usb_write_ptr_svc = (struct diag_request *)
			(diagmem_alloc(driver, sizeof(struct diag_request),
				 POOL_TYPE_USB_STRUCT));
			driver->usb_write_ptr_svc->length = driver->used;
			driver->usb_write_ptr_svc->buf = buf;
#ifdef SLATE_DEBUG
            printk(KERN_INFO "APPS_DATA writing data to USB," " pkt length %d \n", driver->usb_write_ptr_svc->length);
            for (i=0; i < driver->usb_write_ptr_svc->length; i++)
            {
                printk(KERN_INFO " %02X", ((char*)buf)[i]);
                if ( ((i+1) % 20) == 0 )
                {
                    printk(KERN_INFO "\n");
                }
            }
            printk(KERN_INFO "\n");
            i = 0;
#endif
			err = diag_write(driver->usb_write_ptr_svc);
		} else if (proc_num == MODEM_DATA) {
			write_ptr->buf = buf;
#ifdef DIAG_DEBUG
			printk(KERN_INFO "writing data to USB,"
				"pkt length %d\n", write_ptr->length);
			print_hex_dump(KERN_DEBUG, "Written Packet Data to"
					   " USB: ", 16, 1, DUMP_PREFIX_ADDRESS,
					    buf, write_ptr->length, 1);
#endif
			
			if (diag_process_modem_pkt(buf, write_ptr->length))
			{
				err = diag_write(write_ptr);
			}
			else
			{
				/* Do not fwd modem packet to QXDM */
				err = 0;
				__clear_in_busy(write_ptr);
			}
		} else if (proc_num == QDSP_DATA) {
			write_ptr->buf = buf;
			err = diag_write(write_ptr);
		}
		APPEND_DEBUG('k');
	} else if (driver->logging_mode == MEMORY_DEVICE_MODE) {
		if (proc_num == APPS_DATA) {
			for (i = 0; i < driver->poolsize_usb_struct; i++)
				if (driver->buf_tbl[i].length == 0) {
					driver->buf_tbl[i].buf = buf;
					driver->buf_tbl[i].length =
								 driver->used;
#ifdef DIAG_DEBUG
					printk(KERN_INFO "\n ENQUEUE buf ptr"
						   " and length is %x , %d\n",
						   (unsigned int)(driver->buf_
				tbl[i].buf), driver->buf_tbl[i].length);
#endif
					break;
				}
		}
		for (i = 0; i < driver->num_clients; i++)
			if (driver->client_map[i].pid ==
						 driver->logging_process_id)
				break;
		if (i < driver->num_clients) {
			driver->data_ready[i] |= MEMORY_DEVICE_LOG_TYPE;
			wake_up_interruptible(&driver->wait_q);
		} else
			return -EINVAL;
	} else if (driver->logging_mode == NO_LOGGING_MODE) {
		if (proc_num == MODEM_DATA) {
			driver->in_busy_1 = 0;
			driver->in_busy_2 = 0;
			queue_work(driver->diag_wq, &(driver->
							diag_read_smd_work));
		} else if (proc_num == QDSP_DATA) {
			driver->in_busy_qdsp_1 = 0;
			driver->in_busy_qdsp_2 = 0;
			queue_work(driver->diag_wq, &(driver->
						diag_read_smd_qdsp_work));
		}
		err = -1;
	}
    return err;
}

void __diag_smd_qdsp_send_req(void)
{
	void *buf = NULL;
	int *in_busy_qdsp_ptr = NULL;
	struct diag_request *write_ptr_qdsp = NULL;

	if (!driver->in_busy_qdsp_1) {
		buf = driver->usb_buf_in_qdsp_1;
		write_ptr_qdsp = driver->usb_write_ptr_qdsp_1;
		in_busy_qdsp_ptr = &(driver->in_busy_qdsp_1);
	} else if (!driver->in_busy_qdsp_2) {
		buf = driver->usb_buf_in_qdsp_2;
		write_ptr_qdsp = driver->usb_write_ptr_qdsp_2;
		in_busy_qdsp_ptr = &(driver->in_busy_qdsp_2);
	}

	if (driver->chqdsp && buf) {
		int r = smd_read_avail(driver->chqdsp);

		if (r > USB_MAX_IN_BUF) {
			if (r < MAX_BUF_SIZE) {
				printk(KERN_ALERT "\n diag: SMD sending in "
						   "packets upto %d bytes", r);
				buf = krealloc(buf, r, GFP_KERNEL);
			} else {
				printk(KERN_ALERT "\n diag: SMD sending in "
				"packets more than %d bytes", MAX_BUF_SIZE);
				return;
			}
		}
		if (r > 0) {
			if (!buf)
				printk(KERN_INFO "Out of diagmem for a9\n");
			else {
				APPEND_DEBUG('i');
				smd_read(driver->chqdsp, buf, r);
				APPEND_DEBUG('j');
				write_ptr_qdsp->length = r;
				*in_busy_qdsp_ptr = 1;
				diag_device_write(buf, QDSP_DATA,
							 write_ptr_qdsp);
			}
		}
	}
}

static void diag_print_mask_table(void)
{
/* Enable this to print mask table when updated */
#ifdef MASK_DEBUG
	int first;
	int last;
	uint8_t *ptr = driver->msg_masks;
	int i = 0;

	while (*(uint32_t *)(ptr + 4)) {
		first = *(uint32_t *)ptr;
		ptr += 4;
		last = *(uint32_t *)ptr;
		ptr += 4;
		printk(KERN_INFO "SSID %d - %d\n", first, last);
		for (i = 0 ; i <= last - first ; i++)
			printk(KERN_INFO "MASK:%x\n", *((uint32_t *)ptr + i));
		ptr += ((last - first) + 1)*4;

	}
#endif
}

static void diag_update_msg_mask(int start, int end , uint8_t *buf)
{
	int found = 0;
	int first;
	int last;
	uint8_t *ptr = driver->msg_masks;
	uint8_t *ptr_buffer_start = &(*(driver->msg_masks));
	uint8_t *ptr_buffer_end = &(*(driver->msg_masks)) + MSG_MASK_SIZE;

	mutex_lock(&driver->diagchar_mutex);
	/* First SSID can be zero : So check that last is non-zero */

	while (*(uint32_t *)(ptr + 4)) {
		first = *(uint32_t *)ptr;
		ptr += 4;
		last = *(uint32_t *)ptr;
		ptr += 4;
		if (start >= first && start <= last) {
			ptr += (start - first)*4;
			if (end <= last)
				if (CHK_OVERFLOW(ptr_buffer_start, ptr,
						  ptr_buffer_end,
						  (((end - start)+1)*4)))
					memcpy(ptr, buf , ((end - start)+1)*4);
				else
					printk(KERN_CRIT "Not enough"
							 " buffer space for"
							 " MSG_MASK\n");
			else
				printk(KERN_INFO "Unable to copy"
						 " mask change\n");

			found = 1;
			break;
		} else {
			ptr += ((last - first) + 1)*4;
		}
	}
	/* Entry was not found - add new table */
	if (!found) {
		if (CHK_OVERFLOW(ptr_buffer_start, ptr, ptr_buffer_end,
				  8 + ((end - start) + 1)*4)) {
			memcpy(ptr, &(start) , 4);
			ptr += 4;
			memcpy(ptr, &(end), 4);
			ptr += 4;
			memcpy(ptr, buf , ((end - start) + 1)*4);
		} else
			printk(KERN_CRIT " Not enough buffer"
					 " space for MSG_MASK\n");
	}
	mutex_unlock(&driver->diagchar_mutex);
	diag_print_mask_table();

}

static void diag_update_event_mask(uint8_t *buf, int toggle, int num_bits)
{
	uint8_t *ptr = driver->event_masks;
	uint8_t *temp = buf + 2;

	mutex_lock(&driver->diagchar_mutex);
	if (!toggle)
		memset(ptr, 0 , EVENT_MASK_SIZE);
	else
		if (CHK_OVERFLOW(ptr, ptr,
				 ptr+EVENT_MASK_SIZE,
				  num_bits/8 + 1))
			memcpy(ptr, temp , num_bits/8 + 1);
		else
			printk(KERN_CRIT "Not enough buffer space "
					 "for EVENT_MASK\n");
	mutex_unlock(&driver->diagchar_mutex);
}

static void diag_update_log_mask(uint8_t *buf, int num_items)
{
	uint8_t *ptr = driver->log_masks;
	uint8_t *temp = buf;

	mutex_lock(&driver->diagchar_mutex);
	if (CHK_OVERFLOW(ptr, ptr, ptr + LOG_MASK_SIZE,
				  (num_items+7)/8))
		memcpy(ptr, temp , (num_items+7)/8);
	else
		printk(KERN_CRIT " Not enough buffer space for LOG_MASK\n");
	mutex_unlock(&driver->diagchar_mutex);
}

static void diag_update_pkt_buffer(unsigned char *buf)
{
	unsigned char *ptr = driver->pkt_buf;
	unsigned char *temp = buf;

	mutex_lock(&driver->diagchar_mutex);
	if (CHK_OVERFLOW(ptr, ptr, ptr + PKT_SIZE, driver->pkt_length))
		memcpy(ptr, temp , driver->pkt_length);
	else
		printk(KERN_CRIT " Not enough buffer space for PKT_RESP\n");
	mutex_unlock(&driver->diagchar_mutex);
}
//Slate Added FXPCAYM81
static void diag_cfg_log_mask(unsigned char cmd_type)
{
    unsigned len = 0;
    unsigned char *ptr = driver->hdlc_buf;

    printk(KERN_INFO "diag_cfg_log_mask cmd_type=%d\n", cmd_type);

    switch(cmd_type)
    {
    case DIAG_MASK_CMD_ADD_GET_RSSI:
        if (ptr)
        {
            len = 159; // 0x4F5 bits / 8 = 159 bytes
            memset(ptr, 0, len+16);
            *ptr = 0x73;
            ptr += 4;
            *(int *)ptr = 0x0003;
            ptr += 4;
            *(int *)ptr = 0x0001;
            ptr += 4;
            *(int *)ptr = 0x04F5;
            ptr += 4;
            memcpy(ptr, driver->log_masks, len);
            ptr += 13;
            *ptr |= 0x02; // Enable 0x1069 EVDO POWER log bit

            diag_process_user_pkt(driver->hdlc_buf, len+16); // len param not used for 0x73
        }
        break;
    case DIAG_MASK_CMD_ADD_GET_STATE_AND_CONN_ATT:
        if (ptr)
        {
            len = 159; // 0x4F5 bits / 8 = 159 bytes
            memset(ptr, 0, len+16);
            *ptr = 0x73;
            ptr += 4;
            *(int *)ptr = 0x0003;
            ptr += 4;
            *(int *)ptr = 0x0001;
            ptr += 4;
            *(int *)ptr = 0x04F5;
            ptr += 4;
            memcpy(ptr, driver->log_masks, len);
            ptr += 13;
            *ptr |= 0x40; // Enable 0x106E EVDO CONN ATTEMPTS log bit
            ptr += 2;
            *ptr |= 0x40; // Enable 0x107E EVDO STATE log bit

            diag_process_user_pkt(driver->hdlc_buf, len+16);  // len param not used for 0x73
        }
        break;
	case DIAG_MASK_CMD_ADD_GET_SEARCHER_DUMP:
		if (ptr)
        {
            len = 159; // 0x4F5 bits / 8 = 159 bytes
            memset(ptr, 0, len+16);
            *ptr = 0x73;
            ptr += 4;
            *(int *)ptr = 0x0003;
            ptr += 4;
            *(int *)ptr = 0x0001;
            ptr += 4;
            *(int *)ptr = 0x04F5;
            ptr += 4;
            memcpy(ptr, driver->log_masks, len);

			ptr += 5;
			*ptr |= 0x20; // 1020 (Searcher and Finger)

			ptr += 38;
			*ptr |= 0x01; // 1158 (Internal - Core Dump)

			ptr += 8;
			*ptr |= 0x08; // 119A, 119B (Srch TNG Finger Status and Srch TNG 1x Searcher Dump)

            diag_process_user_pkt(driver->hdlc_buf, len+16);  // len param not used for 0x73
        }
		break;
    case DIAG_MASK_CMD_SAVE:
        memcpy(driver->saved_log_masks, driver->log_masks, LOG_MASK_SIZE);
        break;
    case DIAG_MASK_CMD_RESTORE:
    default:
        if (ptr)
        {
            len = 159; // 0x4F5 bits / 8 = 159 bytes
            memset(ptr, 0, len+16);
            *ptr = 0x73;
            ptr += 4;
            *(int *)ptr = 0x0003;
            ptr += 4;
            *(int *)ptr = 0x0001;
            ptr += 4;
            *(int *)ptr = 0x04F5;
            ptr += 4;
            memcpy(ptr, driver->saved_log_masks, len);

            diag_process_user_pkt(driver->hdlc_buf, len+16);  // len param not used for 0x73
        }
        break;
    }
}

void diag_init_log_cmd(unsigned char log_cmd)
{
	printk(KERN_INFO "diag_init_og_cmd log_cmd=%d\n", log_cmd);

	switch(log_cmd)
    {
		case DIAG_LOG_CMD_TYPE_GET_1x_SEARCHER_DUMP:
			diag_cfg_log_mask(DIAG_MASK_CMD_SAVE);
			diag_cfg_log_mask(DIAG_MASK_CMD_ADD_GET_SEARCHER_DUMP);
			break;
		default:
			printk(KERN_ERR "Invalid log_cmd=%d\n", log_cmd);
			break;
    }

    /* Enable log capture for slate commands */
    driver->log_count = 0;
    driver->log_cmd = log_cmd;
}

void diag_init_slate_log_cmd(unsigned char cmd_type)
{
    printk(KERN_INFO "diag_init_slate_log_cmd cmd_type=%d\n", cmd_type);

    switch(cmd_type)
    {
    case DIAG_LOG_CMD_TYPE_GET_RSSI:
        diag_cfg_log_mask(DIAG_MASK_CMD_SAVE);
        diag_cfg_log_mask(DIAG_MASK_CMD_ADD_GET_RSSI);
        break;
    case DIAG_LOG_CMD_TYPE_GET_STATE_AND_CONN_ATT:
        diag_cfg_log_mask(DIAG_MASK_CMD_SAVE);
        diag_cfg_log_mask(DIAG_MASK_CMD_ADD_GET_STATE_AND_CONN_ATT);
        break;
    default:
        printk(KERN_ERR "Invalid SLATE log cmd_type %d\n", cmd_type);
        break;
    }

    /* Enable log capture for slate commands */
    driver->slate_log_count = 0;
    driver->slate_cmd = cmd_type;
}

int diag_log_is_enabled(unsigned char log_type)
{
    int log_offset = 0;
    uint8_t log_mask = 0;
    int ret = 0;

    switch (log_type)
    {
    case DIAG_LOG_TYPE_RSSI:
        log_offset = 13;
        log_mask = 0x02;
        break;
    case DIAG_LOG_TYPE_STATE:
        log_offset = 15;
        log_mask = 0x40;
        break;
    case DIAG_LOG_TYPE_CONN_ATT:
        log_offset = 13;
        log_mask = 0x40;
        break;
	case DIAG_LOG_TYPE_SEARCHER_AND_FINGER:
		log_offset = 5;
        log_mask = 0x20;
		break;
	case DIAG_LOG_TYPE_INTERNAL_CORE_DUMP:
		log_offset = 43;
        log_mask = 0x01;
		break;
	case DIAG_LOG_TYPE_SRCH_TNG_SEARCHER_DUMP:
		log_offset = 51;
        log_mask = 0x08;
		break;

    default:
        printk(KERN_ERR "diag_log_is_enabled INVALID log_type = %d\n", log_type);
        break;
    }

#ifdef SLATE_DEBUG
    {
        int i;

        printk(KERN_INFO "SAVED LOG MASK\n");
        for (i = 0; i < 16; i++)
        {
            printk(KERN_INFO " %02X", ((char*)driver->saved_log_masks)[i]);
            if ( ((i+1) % 20) == 0 )
            {
                printk(KERN_INFO "\n");
            }
        }
        printk(KERN_INFO "\n");
    }
#endif

    if ((driver->saved_log_masks[log_offset] & log_mask) == log_mask)
    {
        ret = 1;
    }

    printk(KERN_INFO "diag_log_is_enabled log_type=%d ret=%d\n", log_type, ret);
    return ret;
}

void diag_restore_log_masks(void)
{
    printk(KERN_INFO "diag_restore_log_masks\n");

    driver->log_cmd = DIAG_LOG_CMD_TYPE_RESTORE_LOG_MASKS;

    /* This restores the log masks to the state before the Slate cmd was executed */
    /* If all logs were disabled, this will disable the logs. */
    /* If some logs were enabled, then those will be restored. */
    diag_cfg_log_mask(DIAG_MASK_CMD_RESTORE);
}

static void diag_slate_restore_log_masks(void)
{
    printk(KERN_INFO "diag_slate_restore_log_masks\n");

    driver->slate_cmd = DIAG_LOG_CMD_TYPE_RESTORE_LOG_MASKS;

    /* This restores the log masks to the state before the Slate cmd was executed */
    /* If all logs were disabled, this will disable the logs. */
    /* If some logs were enabled, then those will be restored. */
    diag_cfg_log_mask(DIAG_MASK_CMD_RESTORE);

}

static void diag_log_cmd_complete(void)
{
    printk(KERN_INFO "diag_log_cmd_complete\n");

    /* Make sure we clear slate status variables */
    driver->log_cmd = DIAG_LOG_CMD_TYPE_NONE;
    driver->log_count = 0;
}

static void diag_slate_log_cmd_complete(void)
{
    printk(KERN_INFO "diag_slate_log_cmd_complete\n");

    /* Make sure we clear slate status variables */
    driver->slate_cmd = DIAG_LOG_CMD_TYPE_NONE;
    driver->slate_log_count = 0;
}

void diag_process_get_rssi_log(void)
{
    printk(KERN_INFO "diag_process_get_rssi_log\n");
    diag_init_slate_log_cmd(DIAG_LOG_CMD_TYPE_GET_RSSI);
}

void diag_process_get_stateAndConnInfo_log(void)
{
    printk(KERN_INFO "diag_process_get_stateAndConnInfo_log\n");
    diag_init_slate_log_cmd(DIAG_LOG_CMD_TYPE_GET_STATE_AND_CONN_ATT);
}
//Slate Added FXPCAYM81 ends

void diag_update_userspace_clients(unsigned int type)
{
	int i;

	mutex_lock(&driver->diagchar_mutex);
	for (i = 0; i < driver->num_clients; i++)
		if (driver->client_map[i].pid != 0)
			driver->data_ready[i] |= type;
	wake_up_interruptible(&driver->wait_q);
	mutex_unlock(&driver->diagchar_mutex);
}

void diag_update_sleeping_process(int process_id)
{
	int i;

	mutex_lock(&driver->diagchar_mutex);
	for (i = 0; i < driver->num_clients; i++)
		if (driver->client_map[i].pid == process_id) {
			driver->data_ready[i] |= PKT_TYPE;
			break;
		}
	wake_up_interruptible(&driver->wait_q);
	mutex_unlock(&driver->diagchar_mutex);
}

static int diag_process_apps_pkt(unsigned char *buf, int len)
{
	uint16_t start;
	uint16_t end, subsys_cmd_code;
	int i, cmd_code, subsys_id;
	int packet_type = 1;
	unsigned char *temp = buf;

	/* event mask */
	if ((*buf == 0x60) && (*(++buf) == 0x0)) {
		diag_update_event_mask(buf, 0, 0);
		diag_update_userspace_clients(EVENT_MASKS_TYPE);
	}
	/* check for set event mask */
	else if (*buf == 0x82) {
		buf += 4;
		diag_update_event_mask(buf, 1, *(uint16_t *)buf);
		diag_update_userspace_clients(
		EVENT_MASKS_TYPE);
	}
	/* log mask */
	else if (*buf == 0x73) {
		buf += 4;
		if (*(int *)buf == 3) {
			buf += 8;
			diag_update_log_mask(buf+4, *(int *)buf);
			diag_update_userspace_clients(LOG_MASKS_TYPE);
		}
	}
	/* Check for set message mask  */
	else if ((*buf == 0x7d) && (*(++buf) == 0x4)) {
		buf++;
		start = *(uint16_t *)buf;
		buf += 2;
		end = *(uint16_t *)buf;
		buf += 4;
		diag_update_msg_mask((uint32_t)start, (uint32_t)end , buf);
		diag_update_userspace_clients(MSG_MASKS_TYPE);
	}
	/* Set all run-time masks
	if ((*buf == 0x7d) && (*(++buf) == 0x5)) {
		TO DO
	} */

	/* Check for registered clients and forward packet to user-space */
	else{
		cmd_code = (int)(*(char *)buf);
		temp++;
		subsys_id = (int)(*(char *)temp);
		temp++;
		subsys_cmd_code = *(uint16_t *)temp;
		temp += 2;

		for (i = 0; i < diag_max_registration; i++) {
			if (driver->table[i].process_id != 0) {
				if (driver->table[i].cmd_code ==
				     cmd_code && driver->table[i].subsys_id ==
				     subsys_id &&
				    driver->table[i].cmd_code_lo <=
				     subsys_cmd_code &&
					  driver->table[i].cmd_code_hi >=
				     subsys_cmd_code){
					driver->pkt_length = len;
					diag_update_pkt_buffer(buf);
					diag_update_sleeping_process(
						driver->table[i].process_id);
						return 0;
				    } /* end of if */
				else if (driver->table[i].cmd_code == 255
					  && cmd_code == 75) {
					if (driver->table[i].subsys_id ==
					    subsys_id &&
					   driver->table[i].cmd_code_lo <=
					    subsys_cmd_code &&
					     driver->table[i].cmd_code_hi >=
					    subsys_cmd_code){
						driver->pkt_length = len;
						diag_update_pkt_buffer(buf);
						diag_update_sleeping_process(
							driver->table[i].
							process_id);
						return 0;
					}
				} /* end of else-if */
				else if (driver->table[i].cmd_code == 255 &&
					  driver->table[i].subsys_id == 255) {
					if (driver->table[i].cmd_code_lo <=
							 cmd_code &&
						     driver->table[i].
						    cmd_code_hi >= cmd_code){
						driver->pkt_length = len;
						diag_update_pkt_buffer(buf);
						diag_update_sleeping_process
							(driver->table[i].
							 process_id);
						return 0;
					}
				} /* end of else-if */
			} /* if(driver->table[i].process_id != 0) */
		}  /* for (i = 0; i < diag_max_registration; i++) */
	} /* else */
		return packet_type;
}
//Slate added FXPCAYM81 start here
static int diag_process_modem_pkt(unsigned char *buf, int len)
{
    int ret = 1; // 1 - pkt needs to be forwarded to QXDM; 0 - drop packet

#ifdef SLATE_DEBUG
    print_hex_dump(KERN_DEBUG, "MODEM_DATA writing data to USB ", 16, 1,
							   DUMP_PREFIX_ADDRESS, buf, len, 1);
#endif

    if( (((char*)buf)[0] == 0x10) && (((char*)buf)[6] == 0x69) && (((char*)buf)[7] == 0x10) )
    {
#ifdef SLATE_DEBUG
        printk(KERN_INFO "MODEM_DATA **** EVDO POWER PKT ***** slate_cmd=%d\n", driver->slate_cmd);
#endif

        switch(driver->slate_cmd)
        {
        case DIAG_LOG_CMD_TYPE_GET_RSSI:
            /* Restore log masks */
            diag_slate_restore_log_masks();

            /* Fwd packet to DIAG Daemon */
            diag_process_apps_pkt(buf, len);

            // fwd pkt if saved_log_mask has this log enabled
            if (diag_log_is_enabled(DIAG_LOG_TYPE_RSSI) == 0)
            {
                /* Do not fwd modem packet to QXDM if it was not enabled before we received the slate command */
                ret = 0;
#ifdef SLATE_DEBUG
        printk(KERN_INFO "SLATE EVDO POWER LOG PACKET DROPPED!!!!!\n");
#endif
            }
            break;
        case DIAG_LOG_CMD_TYPE_GET_STATE_AND_CONN_ATT:
        default:
            // log may have been enabled before we received the slate cmd
            break;
        }

    }
    else if( (((char*)buf)[0] == 0x10) && (((char*)buf)[6] == 0x6E) && (((char*)buf)[7] == 0x10) )
    {
#ifdef SLATE_DEBUG
        printk(KERN_INFO "MODEM_DATA **** EVDO CONN ATT ***** slate_cmd=%d\n", driver->slate_cmd);
#endif

        switch(driver->slate_cmd)
        {
        case DIAG_LOG_CMD_TYPE_GET_STATE_AND_CONN_ATT:
            if ((driver->slate_log_count & 0x02) == 0x00)
            {
                /* We haven't sent this log to the diag daemon, send it now. */
                driver->slate_log_count |= 2;
                if (driver->slate_log_count == 3)
                {
                    /* If we also sent the other log, we're done with this cmd */
                    /* Restore log masks */
                    diag_slate_restore_log_masks();
                }
                /* Fwd packet to DIAG Daemon */
                diag_process_apps_pkt(buf, len);
            }
            /* else - we already sent this log */

            // fwd pkt if saved_log_mask has this log enabled
            if (diag_log_is_enabled(DIAG_LOG_TYPE_CONN_ATT) == 0)
            {
                /* Do not fwd modem packet to QXDM if it was not enabled before we received the slate command */
                ret = 0;
#ifdef SLATE_DEBUG
        printk(KERN_INFO "SLATE EVDO CONN ATT LOG PACKET DROPPED!!!!!\n");
#endif
            }
            break;
        case DIAG_LOG_CMD_TYPE_GET_RSSI:
        default:
            // log may have been enabled before we received the slate cmd
            break;
        }

    } // the byte 0x7E is transmitted as 0x7D followed by 0x5E
    else if( (((char*)buf)[0] == 0x10) && (((char*)buf)[6]  == 0x7D) && (((char*)buf)[7]  == 0x5E) && (((char*)buf)[8]  == 0x10) )
    {
#ifdef SLATE_DEBUG
        printk(KERN_INFO "MODEM_DATA **** EVDO STATE ***** slate_cmd=%d\n", driver->slate_cmd);
#endif

        switch(driver->slate_cmd)
        {
        case DIAG_LOG_CMD_TYPE_GET_STATE_AND_CONN_ATT:
            if ((driver->slate_log_count & 0x01) == 0x00)
            {
                /* We haven't sent this log to the diag daemon, send it now. */
                driver->slate_log_count |= 1;
                if (driver->slate_log_count == 3)
                {
                    /* If we also sent the other log, we're done with this cmd */
                    /* Restore log masks */
                    diag_slate_restore_log_masks();
                }
                /* Fwd packet to DIAG Daemon */
                diag_process_apps_pkt(buf, len);
            }
            /* else - we already sent this log */

            // fwd pkt if saved_log_mask has this log enabled
            if (diag_log_is_enabled(DIAG_LOG_TYPE_STATE) == 0)
            {
                /* Do not fwd modem packet to QXDM if it was not enabled before we received the slate command */
                ret = 0;
#ifdef SLATE_DEBUG
        printk(KERN_INFO "SLATE EVDO STATE LOG PACKET DROPPED!!!!!\n");
#endif
            }
            break;
        case DIAG_LOG_CMD_TYPE_GET_RSSI:
        default:
            // log may have been enabled before we received the slate cmd
            break;
        }

    }
	else if ( (((char*)buf)[0] == 0x10) &&  ( (((char*)buf)[6]  == 0x9A) || (((char*)buf)[6]  == 0x9B) ) 
			  && (((char*)buf)[7]  == 0x11) ) {

        // Process 0x119A and 0x119B log packets in here

		if (((char*)buf)[6]  == 0x9A) 
		{
			printk(KERN_INFO "MODEM_DATA **** 119A ***** log_cmd=%d\n", driver->log_cmd);
		} 
		else if (((char*)buf)[6]  == 0x9B) 
		{
			printk(KERN_INFO "MODEM_DATA **** 119B ***** log_cmd=%d\n", driver->log_cmd);
		}
		

		switch(driver->log_cmd)
		{
			case DIAG_LOG_CMD_TYPE_GET_1x_SEARCHER_DUMP:
				// We have enabled the log and got the log so take care of it in here.
				// Save the data in our local buffer 
				if (((char*)buf)[6]  == 0x9A)
				{
					memset(diag_rsp_119a, 0, USB_MAX_OUT_BUF);
					printk("Copied %x bytes of data\n", len);
					memcpy(diag_rsp_119a, buf, len);
				} 
				else if (((char*)buf)[6]  == 0x9B) 
				{
					memset(diag_rsp_119b, 0, USB_MAX_OUT_BUF);
					printk("Copied %x bytes of data\n", len);
					memcpy(diag_rsp_119b, buf, len);
				}
	
				// fwd pkt if saved_log_mask has this log enabled
				if (diag_log_is_enabled(DIAG_LOG_TYPE_SRCH_TNG_SEARCHER_DUMP) == 0)
				{
					/* Do not fwd modem packet to QXDM if it was not enabled before we received the slate command */
					ret = 0;
#ifdef SLATE_DEBUG
					printk(KERN_INFO "DIAG_LOG_TYPE_SRCH_TNG_SEARCHER_DUMP 119A/119B LOG PACKET DROPPED!!!!!\n");
#endif
				}
				break;
			case DIAG_LOG_CMD_TYPE_RESTORE_LOG_MASKS:
				ret = 0;
				// This is the extra packet that is received before the log mask is disabled for this log
				// hence discarding it as we don't need it.
				printk(KERN_INFO "Dropping the packet as we are not looking for one\n");
				break;
			default:
				// Some other task may have enabled this log, don't do anything, let that task handle the message.
				break;
		}
	}
	else if( (((char*)buf)[0] == 0x73) && ( (driver->log_cmd != DIAG_LOG_CMD_TYPE_NONE) || (driver->slate_cmd != DIAG_LOG_CMD_TYPE_NONE)) )
    {
#ifdef SLATE_DEBUG
		if (driver->log_cmd != DIAG_LOG_CMD_TYPE_NONE) 
		{
			printk(KERN_INFO "MODEM_DATA **** THIS MUST BE RSP for LOG MASK REQ ***** log_cmd=%d\n", driver->log_cmd);
		}
		else
		{
        printk(KERN_INFO "MODEM_DATA **** THIS MUST BE RSP for SLATE MASK REQ ***** slate_cmd=%d\n", driver->slate_cmd);
		}
#endif
        // After processing slate log commands, the mask is always restored.
        // This is where the response from the modem side is handled for the log mask restore.
        if (driver->slate_cmd  == DIAG_LOG_CMD_TYPE_RESTORE_LOG_MASKS)
        {
            diag_slate_log_cmd_complete();
        }

		if (driver->log_cmd == DIAG_LOG_CMD_TYPE_RESTORE_LOG_MASKS) 
		{
			diag_log_cmd_complete();
		}

        /* Do not fwd modem packet to QXDM */
        ret = 0;
    }

    return ret;
}

static void diag_process_user_pkt(unsigned char *data, unsigned len)
{
	int type = 0;

#ifdef SLATE_DEBUG
    print_hex_dump(KERN_DEBUG, "diag_process_user_pkt ", 16, 1,
							   DUMP_PREFIX_ADDRESS, data, len, 1);
#endif

    type = diag_process_apps_pkt(data, len);

	if ((driver->ch) && (type))
    {
        /* Fwd pkt to modem */
		smd_write(driver->ch, data, len);
	}
    else
    {
        printk(KERN_ERR "DIAG User Packet not written to SMD (MODEM) type=%d\n", type);
    }

}

//FXPCAYM81 ends here
void diag_process_hdlc(void *data, unsigned len)
{
	struct diag_hdlc_decode_type hdlc;
	int ret, type = 0;
#ifdef DIAG_DEBUG
	int i;
	printk(KERN_INFO "\n HDLC decode function, len of data  %d\n", len);
#endif
	hdlc.dest_ptr = driver->hdlc_buf;
	hdlc.dest_size = USB_MAX_OUT_BUF;
	hdlc.src_ptr = data;
	hdlc.src_size = len;
	hdlc.src_idx = 0;
	hdlc.dest_idx = 0;
	hdlc.escaping = 0;

	ret = diag_hdlc_decode(&hdlc);

	if (ret)
		type = diag_process_apps_pkt(driver->hdlc_buf,
							  hdlc.dest_idx - 3);
	else if (driver->debug_flag) {
		printk(KERN_ERR "Packet dropped due to bad HDLC coding/CRC"
				" errors or partial packet received, packet"
				" length = %d\n", len);
		print_hex_dump(KERN_DEBUG, "Dropped Packet Data: ", 16, 1,
					   DUMP_PREFIX_ADDRESS, data, len, 1);
		driver->debug_flag = 0;
	}
#ifdef DIAG_DEBUG
	printk(KERN_INFO "\n hdlc.dest_idx = %d \n", hdlc.dest_idx);
	for (i = 0; i < hdlc.dest_idx; i++)
		printk(KERN_DEBUG "\t%x", *(((unsigned char *)
							driver->hdlc_buf)+i));
#endif
	/* ignore 2 bytes for CRC, one for 7E and send */
	if ((driver->ch) && (ret) && (type) && (hdlc.dest_idx > 3)) {
		APPEND_DEBUG('g');
		smd_write(driver->ch, driver->hdlc_buf, hdlc.dest_idx - 3);
		APPEND_DEBUG('h');
#ifdef DIAG_DEBUG
		printk(KERN_INFO "writing data to SMD, pkt length %d \n", len);
		print_hex_dump(KERN_DEBUG, "Written Packet Data to SMD: ", 16,
			       1, DUMP_PREFIX_ADDRESS, data, len, 1);
#endif
	}

}

int diagfwd_connect(void)
{
	int err;

	printk(KERN_DEBUG "diag: USB connected\n");
	err = diag_open(driver->poolsize + 3); /* 2 for A9 ; 1 for q6*/
	if (err)
		printk(KERN_ERR "diag: USB port open failed");
	driver->usb_connected = 1;
	driver->in_busy_1 = 0;
	driver->in_busy_2 = 0;
	driver->in_busy_qdsp_1 = 0;
	driver->in_busy_qdsp_2 = 0;

	/* Poll SMD channels to check for data*/
	queue_work(driver->diag_wq, &(driver->diag_read_smd_work));
	queue_work(driver->diag_wq, &(driver->diag_read_smd_qdsp_work));

	driver->usb_read_ptr->buf = driver->usb_buf_out;
	driver->usb_read_ptr->length = USB_MAX_OUT_BUF;
	APPEND_DEBUG('a');
	diag_read(driver->usb_read_ptr);
	APPEND_DEBUG('b');
	return 0;
}

int diagfwd_disconnect(void)
{
	printk(KERN_DEBUG "diag: USB disconnected\n");
	driver->usb_connected = 0;
	driver->in_busy_1 = 1;
	driver->in_busy_2 = 1;
	driver->in_busy_qdsp_1 = 1;
	driver->in_busy_qdsp_2 = 1;
	driver->debug_flag = 1;
	diag_close();
	/* TBD - notify and flow control SMD */
	return 0;
}

int diagfwd_write_complete(struct diag_request *diag_write_ptr)
{
	unsigned char *buf = diag_write_ptr->buf;
	/*Determine if the write complete is for data from arm9/apps/q6 */
	/* Need a context variable here instead */
	if (buf == (void *)driver->usb_buf_in_1) {
		driver->in_busy_1 = 0;
		APPEND_DEBUG('o');
		queue_work(driver->diag_wq, &(driver->diag_read_smd_work));
	} else if (buf == (void *)driver->usb_buf_in_2) {
		driver->in_busy_2 = 0;
		APPEND_DEBUG('O');
		queue_work(driver->diag_wq, &(driver->diag_read_smd_work));
	} else if (buf == (void *)driver->usb_buf_in_qdsp_1) {
		driver->in_busy_qdsp_1 = 0;
		APPEND_DEBUG('p');
		queue_work(driver->diag_wq, &(driver->diag_read_smd_qdsp_work));
	} else if (buf == (void *)driver->usb_buf_in_qdsp_2) {
		driver->in_busy_qdsp_2 = 0;
		APPEND_DEBUG('P');
		queue_work(driver->diag_wq, &(driver->diag_read_smd_qdsp_work));
	} else {
		diagmem_free(driver, (unsigned char *)buf, POOL_TYPE_HDLC);
		diagmem_free(driver, (unsigned char *)diag_write_ptr,
							 POOL_TYPE_USB_STRUCT);
		APPEND_DEBUG('q');
	}
	return 0;
}

int diagfwd_read_complete(struct diag_request *diag_read_ptr)
{
	int len = diag_read_ptr->actual;

	APPEND_DEBUG('c');
#ifdef DIAG_DEBUG
	printk(KERN_INFO "read data from USB, pkt length %d \n",
		    diag_read_ptr->actual);
	print_hex_dump(KERN_DEBUG, "Read Packet Data from USB: ", 16, 1,
		       DUMP_PREFIX_ADDRESS, diag_read_ptr->buf,
		       diag_read_ptr->actual, 1);
#endif
	driver->read_len = len;
	if (driver->logging_mode == USB_MODE)
		queue_work(driver->diag_wq , &(driver->diag_read_work));
	return 0;
}

static struct diag_operations diagfwdops = {
	.diag_connect = diagfwd_connect,
	.diag_disconnect = diagfwd_disconnect,
	.diag_char_write_complete = diagfwd_write_complete,
	.diag_char_read_complete = diagfwd_read_complete
};

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
#define DIAG_READ_RETRY_COUNT    100  //100 * 5 ms = 500ms
int diag_read_from_smd(uint8_t * res_buf, int16_t* res_size)
{
    unsigned long flags;

    int sz;
    int gg = 0;
    int retry = 0;
    int rc = -1;

    for(retry = 0; retry < DIAG_READ_RETRY_COUNT; )
    {
        sz = smd_cur_packet_size(driver->ch);

        if (sz == 0)
        {
            msleep(5);
            retry++;
            continue;
        }
        gg = smd_read_avail(driver->ch);

        if (sz > gg)
        {
            continue;
        }
        //if (sz > 535) {
        //	smd_read(driver->ch, 0, sz);
        //	continue;
        //}

        spin_lock_irqsave(&smd_lock, flags);//mutex_lock(&nmea_rx_buf_lock);
        if (smd_read(driver->ch, res_buf, sz) == sz) {
            spin_unlock_irqrestore(&smd_lock, flags);//mutex_unlock(&nmea_rx_buf_lock);
            break;
        }
        //nmea_devp->bytes_read = sz;
        spin_unlock_irqrestore(&smd_lock, flags);//mutex_unlock(&nmea_rx_buf_lock);
    }
    *res_size = sz;
    if (retry >= DIAG_READ_RETRY_COUNT)
    {
        rc = -2;
        goto lbExit;
    }
    rc = 0;
lbExit:
    return rc;
}
EXPORT_SYMBOL(diag_read_from_smd);

void diag_write_to_smd(uint8_t * cmd_buf, int cmd_size)
{
	unsigned long flags;
	int need;
	//paul// need = sizeof(cmd_buf);
	need = cmd_size;

	spin_lock_irqsave(&smd_lock, flags);
	while (smd_write_avail(driver->ch) < need) {
		spin_unlock_irqrestore(&smd_lock, flags);
		msleep(10);
		spin_lock_irqsave(&smd_lock, flags);
	}
       smd_write(driver->ch, cmd_buf, cmd_size);
       spin_unlock_irqrestore(&smd_lock, flags);
}
EXPORT_SYMBOL(diag_write_to_smd);
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]

static void diag_smd_notify(void *ctxt, unsigned event)
{
	queue_work(driver->diag_wq, &(driver->diag_read_smd_work));
}

#if defined(CONFIG_MSM_N_WAY_SMD)
static void diag_smd_qdsp_notify(void *ctxt, unsigned event)
{
	queue_work(driver->diag_wq, &(driver->diag_read_smd_qdsp_work));
}
#endif

static int diag_smd_probe(struct platform_device *pdev)
{
	int r = 0;

//SW2-5-1-MP-DbgCfgTool-03*[
//+{PS3-RR-ON_DEVICE_QXDM-01
	int cfg_val;
	int boot_mode;
	
	boot_mode = fih_read_boot_mode_from_smem();
	if(boot_mode != APPS_MODE_FTM) {
		r = DbgCfgGetByBit(DEBUG_MODEM_LOGGER_CFG, (int*)&cfg_val);
		if ((r == 0) && (cfg_val == 1) && (boot_mode != APPS_MODE_RECOVERY)) {
			printk(KERN_INFO "FIH:Embedded QXDM Enabled. %s", __FUNCTION__);
		} else {
			printk(KERN_ERR "FIH:Fail to call DbgCfgGetByBit(), ret=%d or Embedded QxDM disabled, cfg_val=%d.", r, cfg_val);
			printk(KERN_ERR "FIH:Default: Embedded QXDM Disabled.\n");
			if (pdev->id == 0) {
				if (driver->usb_buf_in_1 == NULL ||
							 driver->usb_buf_in_2 == NULL) {
					if (driver->usb_buf_in_1 == NULL)
						driver->usb_buf_in_1 = kzalloc(USB_MAX_IN_BUF,
										GFP_KERNEL);
					if (driver->usb_buf_in_2 == NULL)
						driver->usb_buf_in_2 = kzalloc(USB_MAX_IN_BUF,
										GFP_KERNEL);
					if (driver->usb_buf_in_1 == NULL ||
						 driver->usb_buf_in_2 == NULL)
						goto err;
					else
						r = smd_open("DIAG", &driver->ch, driver,
								 diag_smd_notify);
				}
				else
					r = smd_open("DIAG", &driver->ch, driver,
								 diag_smd_notify);
			}
		}
	}
	else {
		if (pdev->id == 0) {
			if (driver->usb_buf_in_1 == NULL ||
						 driver->usb_buf_in_2 == NULL) {
				if (driver->usb_buf_in_1 == NULL)
					driver->usb_buf_in_1 = kzalloc(USB_MAX_IN_BUF,
									GFP_KERNEL);
				if (driver->usb_buf_in_2 == NULL)
					driver->usb_buf_in_2 = kzalloc(USB_MAX_IN_BUF,
									GFP_KERNEL);
				if (driver->usb_buf_in_1 == NULL ||
					 driver->usb_buf_in_2 == NULL)
					goto err;
				else
					r = smd_open("DIAG", &driver->ch, driver,
							 diag_smd_notify);
			}
			else
				r = smd_open("DIAG", &driver->ch, driver,
							 diag_smd_notify);
		}
	}
//PS3-RR-ON_DEVICE_QXDM-01}+
//SW2-5-1-MP-DbgCfgTool-03*]

#if defined(CONFIG_MSM_N_WAY_SMD)
	if (pdev->id == 1) {
		if (driver->usb_buf_in_qdsp_1 == NULL ||
					 driver->usb_buf_in_qdsp_2 == NULL) {
			if (driver->usb_buf_in_qdsp_1 == NULL)
				driver->usb_buf_in_qdsp_1 = kzalloc(
						USB_MAX_IN_BUF, GFP_KERNEL);
			if (driver->usb_buf_in_qdsp_2 == NULL)
				driver->usb_buf_in_qdsp_2 = kzalloc(
						USB_MAX_IN_BUF, GFP_KERNEL);
			if (driver->usb_buf_in_qdsp_1 == NULL ||
				 driver->usb_buf_in_qdsp_2 == NULL)
				goto err;
			else
				r = smd_named_open_on_edge("DIAG", SMD_APPS_QDSP
						, &driver->chqdsp, driver,
							 diag_smd_qdsp_notify);
		}
		else
			r = smd_named_open_on_edge("DIAG", SMD_APPS_QDSP,
			 &driver->chqdsp, driver, diag_smd_qdsp_notify);
	}
#endif
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	printk(KERN_INFO "diag opened SMD port ; r = %d\n", r);

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
#ifdef WRITE_NV_4719_TO_ENTER_RECOVERY
	r = device_create_file(&pdev->dev, &dev_attr_boot2recovery);

	if (r < 0)
	{
		dev_err(&pdev->dev, "%s: Create nv4719 attribute failed!! <%d>", __func__, r);
	}
#endif

	r = device_create_file(&pdev->dev, &dev_attr_turnonofffefslog);

	if (r < 0)
	{
		dev_err(&pdev->dev, "%s: Create turnonofffefslog attribute failed!! <%d>", __func__, r);
	}
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]
err:
	return 0;
}

static int diagfwd_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: suspending...\n");
	return 0;
}

static int diagfwd_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: resuming...\n");
	return 0;
}

static const struct dev_pm_ops diagfwd_dev_pm_ops = {
	.runtime_suspend = diagfwd_runtime_suspend,
	.runtime_resume = diagfwd_runtime_resume,
};


static struct platform_driver msm_smd_ch1_driver = {

	.probe = diag_smd_probe,
	.driver = {
		   .name = "DIAG",
		   .owner = THIS_MODULE,
		   .pm   = &diagfwd_dev_pm_ops,
		   },
};


void diag_read_work_fn(struct work_struct *work)
{
	APPEND_DEBUG('d');
	diag_process_hdlc(driver->usb_buf_out, driver->read_len);
	driver->usb_read_ptr->buf = driver->usb_buf_out;
	driver->usb_read_ptr->length = USB_MAX_OUT_BUF;
	APPEND_DEBUG('e');
	diag_read(driver->usb_read_ptr);
	APPEND_DEBUG('f');
}

void diagfwd_init(void)
{
	diag_debug_buf_idx = 0;
	if (driver->usb_buf_out  == NULL &&
	     (driver->usb_buf_out = kzalloc(USB_MAX_OUT_BUF,
					 GFP_KERNEL)) == NULL)
		goto err;
	if (driver->hdlc_buf == NULL
	    && (driver->hdlc_buf = kzalloc(HDLC_MAX, GFP_KERNEL)) == NULL)
		goto err;
	if (driver->msg_masks == NULL
	    && (driver->msg_masks = kzalloc(MSG_MASK_SIZE,
					     GFP_KERNEL)) == NULL)
		goto err;
	if (driver->log_masks == NULL &&
	    (driver->log_masks = kzalloc(LOG_MASK_SIZE, GFP_KERNEL)) == NULL)
		goto err;
//Slate code FXPCAYM81 Start here		
    if (driver->saved_log_masks == NULL &&
        (driver->saved_log_masks = kzalloc(LOG_MASK_SIZE, GFP_KERNEL)) == NULL)
        goto err;
//Slate code FXPCAYM81 ends here				
	if (driver->event_masks == NULL &&
	    (driver->event_masks = kzalloc(EVENT_MASK_SIZE,
					    GFP_KERNEL)) == NULL)
		goto err;
	if (driver->client_map == NULL &&
	    (driver->client_map = kzalloc
	     ((driver->num_clients) * sizeof(struct diag_client_map),
		   GFP_KERNEL)) == NULL)
		goto err;
	if (driver->buf_tbl == NULL)
			driver->buf_tbl = kzalloc(buf_tbl_size *
			  sizeof(struct diag_write_device), GFP_KERNEL);
	if (driver->buf_tbl == NULL)
		goto err;
	if (driver->data_ready == NULL &&
	     (driver->data_ready = kzalloc(driver->num_clients * sizeof(struct
					 diag_client_map), GFP_KERNEL)) == NULL)
		goto err;
	if (driver->table == NULL &&
	     (driver->table = kzalloc(diag_max_registration*
		      sizeof(struct diag_master_table),
		       GFP_KERNEL)) == NULL)
		goto err;
	if (driver->usb_write_ptr_1 == NULL)
		driver->usb_write_ptr_1 = kzalloc(
			sizeof(struct diag_request), GFP_KERNEL);
		if (driver->usb_write_ptr_1 == NULL)
			goto err;
	if (driver->usb_write_ptr_2 == NULL)
		driver->usb_write_ptr_2 = kzalloc(
			sizeof(struct diag_request), GFP_KERNEL);
		if (driver->usb_write_ptr_2 == NULL)
			goto err;
	if (driver->usb_write_ptr_qdsp_1 == NULL)
		driver->usb_write_ptr_qdsp_1 = kzalloc(
			sizeof(struct diag_request), GFP_KERNEL);
		if (driver->usb_write_ptr_qdsp_1 == NULL)
			goto err;
	if (driver->usb_write_ptr_qdsp_2 == NULL)
		driver->usb_write_ptr_qdsp_2 = kzalloc(
			sizeof(struct diag_request), GFP_KERNEL);
		if (driver->usb_write_ptr_qdsp_2 == NULL)
			goto err;
	if (driver->usb_read_ptr == NULL)
		driver->usb_read_ptr = kzalloc(
			sizeof(struct diag_request), GFP_KERNEL);
		if (driver->usb_read_ptr == NULL)
			goto err;
	if (driver->pkt_buf == NULL &&
	     (driver->pkt_buf = kzalloc(PKT_SIZE,
			 GFP_KERNEL)) == NULL)
		goto err;

//Slate support added FXPCAYM81
    driver->slate_cmd = 0;
    driver->slate_log_count = 0;
//FXPCAYM81 ends here	
	driver->log_cmd = 0;
	driver->log_count = 0;

	driver->diag_wq = create_singlethread_workqueue("diag_wq");
	INIT_WORK(&(driver->diag_read_work), diag_read_work_fn);

	diag_usb_register(&diagfwdops);

	platform_driver_register(&msm_smd_ch1_driver);

	return;
err:
		printk(KERN_INFO "\n Could not initialize diag buffers\n");
		kfree(driver->usb_buf_out);
		kfree(driver->hdlc_buf);
		kfree(driver->msg_masks);
		kfree(driver->log_masks);
		//Slate FXPCAYM81 start Here
        kfree(driver->saved_log_masks);
		//Slate FXPCAYM81 End
		kfree(driver->event_masks);
		kfree(driver->client_map);
		kfree(driver->buf_tbl);
		kfree(driver->data_ready);
		kfree(driver->table);
		kfree(driver->pkt_buf);
		kfree(driver->usb_write_ptr_1);
		kfree(driver->usb_write_ptr_2);
		kfree(driver->usb_write_ptr_qdsp_1);
		kfree(driver->usb_write_ptr_qdsp_2);
		kfree(driver->usb_read_ptr);
}

void diagfwd_exit(void)
{
	smd_close(driver->ch);
	smd_close(driver->chqdsp);
	driver->ch = 0;		/*SMD can make this NULL */
	driver->chqdsp = 0;

	if (driver->usb_connected)
		diag_close();

	platform_driver_unregister(&msm_smd_ch1_driver);

	diag_usb_unregister();

	kfree(driver->usb_buf_in_1);
	kfree(driver->usb_buf_in_2);
	kfree(driver->usb_buf_in_qdsp_1);
	kfree(driver->usb_buf_in_qdsp_2);
	kfree(driver->usb_buf_out);
	kfree(driver->hdlc_buf);
	kfree(driver->msg_masks);
	kfree(driver->log_masks);
	//Slate FXPCAYM81 start Here
    kfree(driver->saved_log_masks);
	//Slate FXPCAYM81 End
	kfree(driver->event_masks);
	kfree(driver->client_map);
	kfree(driver->buf_tbl);
	kfree(driver->data_ready);
	kfree(driver->table);
	kfree(driver->pkt_buf);
	kfree(driver->usb_write_ptr_1);
	kfree(driver->usb_write_ptr_2);
	kfree(driver->usb_write_ptr_qdsp_1);
	kfree(driver->usb_write_ptr_qdsp_2);
	kfree(driver->usb_read_ptr);
}
