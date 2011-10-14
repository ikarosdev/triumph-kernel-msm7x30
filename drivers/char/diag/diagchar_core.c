/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
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
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/diagchar.h>
#include <linux/sched.h>
#include <mach/usbdiag.h>
#include <asm/current.h>
#include "diagchar_hdlc.h"
#include "diagfwd.h"
#include "diagmem.h"
#include "diagchar.h"
#include <linux/timer.h>
#include <linux/usb.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>

MODULE_DESCRIPTION("Diag Char Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");

struct diagchar_dev *driver;
/* The following variables can be specified by module options */
 /* for copy buffer */
static unsigned int itemsize = 2048; /*Size of item in the mempool */
static unsigned int poolsize = 10; /*Number of items in the mempool */
/* for hdlc buffer */
static unsigned int itemsize_hdlc = 8192; /*Size of item in the mempool */
static unsigned int poolsize_hdlc = 8;  /*Number of items in the mempool */
/* for usb structure buffer */
static unsigned int itemsize_usb_struct = 20; /*Size of item in the mempool */
static unsigned int poolsize_usb_struct = 8; /*Number of items in the mempool */
/* This is the max number of user-space clients supported at initialization*/
static unsigned int max_clients = 15;
static unsigned int threshold_client_limit = 30;
/* This is the maximum number of pkt registrations supported at initialization*/
unsigned int diag_max_registration = 25;
unsigned int diag_threshold_registration = 100;

/* Variables to store the Diag Responses for FTM App (0c, 63, 119a, 119b)  (size == USB_MAX_OUT_BUF)*/
char *diag_rsp_0c;
char *diag_rsp_63;
char *diag_rsp_1a;
char *diag_rsp_119a;
char *diag_rsp_119b;
unsigned char drop_0c_packet = 0;
unsigned char drop_63_packet = 0;
unsigned char drop_1a_packet = 0;
extern void diag_init_log_cmd(unsigned char log_type);
extern void diag_restore_log_masks(void);
//Slate Code Start
extern void bq27x0_battery_charging_status_update(void);
extern int slate_counter_flag;
extern int fih_charger;
 //Slate Code Ends

/* Timer variables */
static struct timer_list drain_timer;
static int timer_in_progress;
void *buf_hdlc;
module_param(itemsize, uint, 0);
module_param(poolsize, uint, 0);
module_param(max_clients, uint, 0);

static int slate_charger_state = 0;

//Slate command support FXPCAYM81
#define SLATE_EVENT_MASK_BIT_SET(id) \
  (driver->event_masks[(id)/8] & (1 << ((id) & 0x07)))
//FXPCAYM81 ends here

/* delayed_rsp_id 0 represents no delay in the response. Any other number
    means that the diag packet has a delayed response. */
static uint16_t delayed_rsp_id = 1;
#define DIAGPKT_MAX_DELAYED_RSP 0xFFFF
/* This macro gets the next delayed respose id. Once it reaches
 DIAGPKT_MAX_DELAYED_RSP, it stays at DIAGPKT_MAX_DELAYED_RSP */

#define DIAGPKT_NEXT_DELAYED_RSP_ID(x) 				\
((x < DIAGPKT_MAX_DELAYED_RSP) ? x++ : DIAGPKT_MAX_DELAYED_RSP)

#define COPY_USER_SPACE_OR_EXIT(buf, data, length)		\
do {								\
	if ((count < ret+length) || (copy_to_user(buf,		\
			(void *)&data, length))) {		\
		ret = -EFAULT;					\
		goto exit;					\
	}							\
	ret += length;						\
} while (0)

static void drain_timer_func(unsigned long data)
{
	queue_work(driver->diag_wq , &(driver->diag_drain_work));
}

void diag_drain_work_fn(struct work_struct *work)
{
	int err = 0;
	timer_in_progress = 0;

	mutex_lock(&driver->diagchar_mutex);
	if (buf_hdlc) {
		err = diag_device_write(buf_hdlc, APPS_DATA, NULL);
		if (err) {
			/*Free the buffer right away if write failed */
			diagmem_free(driver, buf_hdlc, POOL_TYPE_HDLC);
			diagmem_free(driver, (unsigned char *)driver->
				 usb_write_ptr_svc, POOL_TYPE_USB_STRUCT);
		}
		buf_hdlc = NULL;
#ifdef DIAG_DEBUG
		printk(KERN_INFO "\n Number of bytes written "
				 "from timer is %d ", driver->used);
#endif
		driver->used = 0;
	}
	mutex_unlock(&driver->diagchar_mutex);
}

void diag_read_smd_work_fn(struct work_struct *work)
{
	__diag_smd_send_req();
}

void diag_read_smd_qdsp_work_fn(struct work_struct *work)
{
	__diag_smd_qdsp_send_req();
}

static int diagchar_open(struct inode *inode, struct file *file)
{
	int i = 0;
	char currtask_name[FIELD_SIZEOF(struct task_struct, comm) + 1];

	if (driver) {
		mutex_lock(&driver->diagchar_mutex);

		for (i = 0; i < driver->num_clients; i++)
			if (driver->client_map[i].pid == 0)
				break;

		if (i < driver->num_clients) {
			driver->client_map[i].pid = current->tgid;
			strncpy(driver->client_map[i].name, get_task_comm(
						currtask_name, current), 20);
			driver->client_map[i].name[19] = '\0';
		} else {
			if (i < threshold_client_limit) {
				driver->num_clients++;
				driver->client_map = krealloc(driver->client_map
					, (driver->num_clients) * sizeof(struct
						 diag_client_map), GFP_KERNEL);
				driver->client_map[i].pid = current->tgid;
				strncpy(driver->client_map[i].name,
					get_task_comm(currtask_name,
							 current), 20);
				driver->client_map[i].name[19] = '\0';
			} else {
				mutex_unlock(&driver->diagchar_mutex);
				if (driver->alert_count == 0 ||
						 driver->alert_count == 10) {
					printk(KERN_ALERT "Max client limit for"
						 "DIAG driver reached\n");
					printk(KERN_INFO "Cannot open handle %s"
					   " %d", get_task_comm(currtask_name,
						 current), current->tgid);
				for (i = 0; i < driver->num_clients; i++)
					printk(KERN_INFO "%d) %s PID=%d"
					, i, driver->client_map[i].name,
					 driver->client_map[i].pid);
					driver->alert_count = 0;
				}
				driver->alert_count++;
				return -ENOMEM;
			}
		}
		driver->data_ready[i] |= MSG_MASKS_TYPE;
		driver->data_ready[i] |= EVENT_MASKS_TYPE;
		driver->data_ready[i] |= LOG_MASKS_TYPE;

		if (driver->ref_count == 0)
			diagmem_init(driver);
		driver->ref_count++;
		mutex_unlock(&driver->diagchar_mutex);
		return 0;
	}
	return -ENOMEM;
}

static int diagchar_close(struct inode *inode, struct file *file)
{
	int i = 0;

	/* If the SD logging process exits, change logging to USB mode */
	if (driver->logging_process_id == current->tgid) {
		driver->logging_mode = USB_MODE;
		diagfwd_connect();
	}
	/* Delete the pkt response table entry for the exiting process */
	for (i = 0; i < diag_max_registration; i++)
			if (driver->table[i].process_id == current->tgid)
					driver->table[i].process_id = 0;

			if (driver) {
				mutex_lock(&driver->diagchar_mutex);
				driver->ref_count--;
				diagmem_exit(driver);
				for (i = 0; i < driver->num_clients; i++)
					if (driver->client_map[i].pid ==
					     current->tgid) {
						driver->client_map[i].pid = 0;
						break;
					}
		mutex_unlock(&driver->diagchar_mutex);
		return 0;
	}
	return -ENOMEM;
}

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
#define SD_CARD_DOWNLOAD    1
#if SD_CARD_DOWNLOAD
extern void diag_write_to_smd(uint8_t * cmd_buf, int cmd_size);
extern int diag_read_from_smd(uint8_t * res_buf, int16_t * res_size);
extern int proc_comm_alloc_sd_dl_smem(int);
#define FLASH_PART_MAGIC1     0x55EE73AA
#define FLASH_PART_MAGIC2     0xE35EBDDB
#define FLASH_PARTITION_VERSION   0x3

struct flash_partition_entry {
	char name[16];
	u32 offset;	/* Offset in blocks from beginning of device */
	u32 length;	/* Length of the partition in blocks */
	u8 attrib1;
	u8 attrib2;
	u8 attrib3;
	u8 which_flash;	/* Numeric ID (first = 0, second = 1) */
};
struct flash_partition_table {
	u32 magic1;
	u32 magic2;
	u32 version;
	u32 numparts;
	struct flash_partition_entry part_entry[16];
};

#endif
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]
extern int hsusb_chg_notify_over_tempearture(bool OT_flag);
extern u32 msm_batt_get_batt_status(void);

static int diagchar_ioctl(struct inode *inode, struct file *filp,
			   unsigned int iocmd, unsigned long ioarg)
{
        int *iStatus;
	int i, j, count_entries = 0, temp;
	int success = -1;
	//Slate Code Start Here FXPCAYM81
	int *set_charger;
	set_charger = (int *)ioarg;

	if(iocmd == USB_DIAG_SET_CHARGER)
	{
		    printk(KERN_INFO"%s:DB: CHARGER SETIING %d\n", __func__, *set_charger);
			if( msm_chg_rpc_connect() >= 0 )
			{
				switch(*set_charger)
				{
				case 0:
					//printk(KERN_INFO"%s:DB: DISABLING CHARGER\n", __func__);
				    fih_charger = 1;
					hsusb_chg_notify_over_tempearture(true);
		            msm_chg_usb_charger_disconnected();
			        bq27x0_battery_charging_status_update();
			        slate_counter_flag = true;
			        slate_charger_state = 0;
					break;
				case 1:
					//printk(KERN_INFO"%s:DB: ENABLING CHARGER\n", __func__);
				    fih_charger = 2;
					hsusb_chg_notify_over_tempearture(false);
					msm_chg_usb_charger_connected(USB_CHG_TYPE__SDP);
			        bq27x0_battery_charging_status_update();
			        slate_charger_state = 1;
					break;
				default:
					printk(KERN_INFO"%s:DB: CHARGE COMMAND: NOT a valid case\n", __func__);
					break;
				}
			}
			return 0;
	}
	else if(iocmd == USB_DIAG_GET_CHARGER)
	{
		 iStatus = (int *)ioarg;
	//	printk(KERN_INFO"DB: CHARGER GET= %d \n", msm_batt_get_batt_status());
		*iStatus = (int)slate_charger_state;
		if( *iStatus == 0 )
		{
		//	printk(KERN_INFO"%s:DB: CHARGER IN UNKNOWN STATE \n", __func__);
			*iStatus = -1;
		}
		else
		{
		//	printk(KERN_INFO"%s:DB: CHARGER IN %d\n", __func__,  *iStatus);			
		}

		return  0;
	}
	else if (iocmd == DIAG_IOCTL_COMMAND_REG) {
		struct bindpkt_params_per_process *pkt_params =
			 (struct bindpkt_params_per_process *) ioarg;

		for (i = 0; i < diag_max_registration; i++) {
			if (driver->table[i].process_id == 0) {
				success = 1;
				driver->table[i].cmd_code =
					pkt_params->params->cmd_code;
				driver->table[i].subsys_id =
					pkt_params->params->subsys_id;
				driver->table[i].cmd_code_lo =
					pkt_params->params->cmd_code_lo;
				driver->table[i].cmd_code_hi =
					pkt_params->params->cmd_code_hi;
				driver->table[i].process_id = current->tgid;
				count_entries++;
				if (pkt_params->count > count_entries)
					pkt_params->params++;
				else
					return success;
			}
		}
		if (i < diag_threshold_registration) {
			/* Increase table size by amount required */
			diag_max_registration += pkt_params->count -
							 count_entries;
			/* Make sure size doesnt go beyond threshold */
			if (diag_max_registration > diag_threshold_registration)
				diag_max_registration =
						 diag_threshold_registration;
			driver->table = krealloc(driver->table,
					 diag_max_registration*sizeof(struct
					 diag_master_table), GFP_KERNEL);
			for (j = i; j < diag_max_registration; j++) {
				success = 1;
				driver->table[j].cmd_code = pkt_params->
							params->cmd_code;
				driver->table[j].subsys_id = pkt_params->
							params->subsys_id;
				driver->table[j].cmd_code_lo = pkt_params->
							params->cmd_code_lo;
				driver->table[j].cmd_code_hi = pkt_params->
							params->cmd_code_hi;
				driver->table[j].process_id = current->tgid;
				count_entries++;
				if (pkt_params->count > count_entries)
					pkt_params->params++;
				else
					return success;
			}
		} else
			pr_err("Max size reached, Pkt Registration failed for"
						" Process %d", current->tgid);

		success = 0;
	} else if (iocmd == DIAG_IOCTL_GET_DELAYED_RSP_ID) {
		struct diagpkt_delay_params *delay_params =
					(struct diagpkt_delay_params *) ioarg;

		if ((delay_params->rsp_ptr) &&
		 (delay_params->size == sizeof(delayed_rsp_id)) &&
				 (delay_params->num_bytes_ptr)) {
			*((uint16_t *)delay_params->rsp_ptr) =
				DIAGPKT_NEXT_DELAYED_RSP_ID(delayed_rsp_id);
			*(delay_params->num_bytes_ptr) = sizeof(delayed_rsp_id);
			success = 0;
		}
	} else if (iocmd == DIAG_IOCTL_LSM_DEINIT) {
		for (i = 0; i < driver->num_clients; i++)
			if (driver->client_map[i].pid == current->tgid)
				break;
		if (i == -1)
			return -EINVAL;
		driver->data_ready[i] |= DEINIT_TYPE;
		wake_up_interruptible(&driver->wait_q);
		success = 1;
	} else if (iocmd == DIAG_IOCTL_SWITCH_LOGGING) {
		mutex_lock(&driver->diagchar_mutex);
		temp = driver->logging_mode;
		driver->logging_mode = (int)ioarg;
		driver->logging_process_id = current->tgid;
		mutex_unlock(&driver->diagchar_mutex);
		if (temp == USB_MODE && driver->logging_mode == NO_LOGGING_MODE)
			diagfwd_disconnect();
		else if (temp == NO_LOGGING_MODE && driver->logging_mode
								== USB_MODE)
			diagfwd_connect();
		else if (temp == MEMORY_DEVICE_MODE && driver->logging_mode
							== NO_LOGGING_MODE) {
			driver->in_busy_1 = 1;
			driver->in_busy_2 = 1;
			driver->in_busy_qdsp_1 = 1;
			driver->in_busy_qdsp_2 = 1;
		} else if (temp == NO_LOGGING_MODE && driver->logging_mode
							== MEMORY_DEVICE_MODE) {
			driver->in_busy_1 = 0;
			driver->in_busy_2 = 0;
			driver->in_busy_qdsp_1 = 0;
			driver->in_busy_qdsp_2 = 0;
			/* Poll SMD channels to check for data*/
			if (driver->ch)
				queue_work(driver->diag_wq,
					&(driver->diag_read_smd_work));
			if (driver->chqdsp)
				queue_work(driver->diag_wq,
					&(driver->diag_read_smd_qdsp_work));
		} else if (temp == USB_MODE && driver->logging_mode
							== MEMORY_DEVICE_MODE) {
			diagfwd_disconnect();
			driver->in_busy_1 = 0;
			driver->in_busy_2 = 0;
			driver->in_busy_qdsp_2 = 0;
			driver->in_busy_qdsp_2 = 0;
			/* Poll SMD channels to check for data*/
			if (driver->ch)
				queue_work(driver->diag_wq,
					 &(driver->diag_read_smd_work));
			if (driver->chqdsp)
				queue_work(driver->diag_wq,
					&(driver->diag_read_smd_qdsp_work));
		} else if (temp == MEMORY_DEVICE_MODE && driver->logging_mode
								== USB_MODE)
			diagfwd_connect();
		success = 1;
	}
    //Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
    else if (iocmd == DIAG_IOCTL_WRITE_BUFFER) 
    {
        struct diagpkt_ioctl_param pkt;
        uint8_t *pBuf = NULL;
        if (copy_from_user(&pkt, (void __user *)ioarg, sizeof(pkt)))
        {
            return -EFAULT;
        }
        if ((pBuf = kzalloc(4096, GFP_KERNEL)) == NULL)
            return -EFAULT;

        memcpy(pBuf, pkt.pPacket, pkt.Len);

        diag_write_to_smd(pBuf, pkt.Len);
        kfree(pBuf);
        return 0;
    }
    else if (iocmd == DIAG_IOCTL_READ_BUFFER) 
    {
    struct diagpkt_ioctl_param pkt;
    struct diagpkt_ioctl_param *ppkt;
    uint8_t *pBuf = NULL;
        if (copy_from_user(&pkt, (void __user *)ioarg, sizeof(pkt)))
        {
            return -EFAULT;
        }

        if ((pBuf = kzalloc(4096, GFP_KERNEL)) == NULL)
            return -EFAULT;

        ppkt = (struct diagpkt_ioctl_param *)ioarg;

        if (diag_read_from_smd(pBuf, &(pkt.Len)) < 0)
        {
            kfree(pBuf);
            return -EFAULT;
        }
        
        if (copy_to_user((void __user *) &ppkt->Len, &pkt.Len, sizeof(pkt.Len)))
        {
            kfree(pBuf);
            return -EFAULT;
        }
        if (copy_to_user((void __user *) pkt.pPacket, pBuf, pkt.Len))
        {
            kfree(pBuf);
            return -EFAULT;
        }
        kfree(pBuf);
        return 0;
    }
    else if (iocmd == DIAG_IOCTL_PASS_FIRMWARE_LIST)
    {
        FirmwareList FL;
        FirmwareList * pFL = NULL;
        int size;

        if (copy_from_user(&FL, (void __user *)ioarg, sizeof(FL)+4))
        {
            return -EFAULT;
        }

        printk("update flag 0x%X\n",FL.iFLAG);
        printk("image %s\n",FL.pCOMBINED_IMAGE);
        printk("0x%08X 0x%08X\n", FL.aANDROID_BOOT[0], FL.aANDROID_BOOT[1]);
        printk("FirmwareListChecksum(FL)=0x%08X, FirmwareListSize(FL)=%d\n", FL.checksum, sizeof(FL));
		
        // Fill smem_mem_type
        proc_comm_alloc_sd_dl_smem(0);

        size = sizeof(FirmwareList);
        pFL = smem_alloc(SMEM_SD_IMG_UPGRADE_STATUS, size);

        if (pFL == NULL)
            return -EFAULT;

        memcpy(pFL, &FL, sizeof(FirmwareList));
        print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET,16, 1, pFL, size, 0);

        printk("0x%08X 0x%08X\n", pFL->aANDROID_BOOT[0], pFL->aANDROID_BOOT[1]);
        printk("FirmwareListChecksum(pFL)=0x%08X, FirmwareListSize(pFL)=%d\n", pFL->checksum, size);
        
        return 0;
    }
    else if (iocmd == DIAG_IOCTL_GET_PART_TABLE_FROM_SMEM)
    {
        struct flash_partition_table *partition_table;
        //struct flash_partition_entry *part_entry;
        //struct mtd_partition *ptn = msm_nand_partitions;
        //char *name = msm_nand_names;
        //int part;

        partition_table = (struct flash_partition_table *)
            smem_alloc(SMEM_AARM_PARTITION_TABLE,
        	       sizeof(struct flash_partition_table));

        if (!partition_table) {
            printk(KERN_WARNING "%s: no flash partition table in shared "
                   "memory\n", __func__);
            return -ENOENT;
        }

        if ((partition_table->magic1 != (u32) FLASH_PART_MAGIC1) ||
            (partition_table->magic2 != (u32) FLASH_PART_MAGIC2) ||
            (partition_table->version != (u32) FLASH_PARTITION_VERSION))
        {
        	printk(KERN_WARNING "%s: version mismatch -- magic1=%#x, "
        	       "magic2=%#x, version=%#x\n", __func__,
        	       partition_table->magic1,
        	       partition_table->magic2,
        	       partition_table->version);
        	return -EFAULT;
        }
        if (copy_to_user((void __user *) ioarg, partition_table, sizeof(struct flash_partition_table)))
        {
            return -EFAULT;
        }

        return 0;
    }
    //Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]
	//Mig_Changes-FXPCAYM81
    else if (iocmd == DIAG_IOCTL_GET_RSSI) {
        //printk(KERN_INFO "DIAG_IOCTL_GET_RSSI\n");
        diag_process_get_rssi_log();
    }
    else if (iocmd == DIAG_IOCTL_GET_STATE_AND_CONN_INFO) {
        //printk(KERN_INFO "DIAG_IOCTL_GET_STATE_AND_CONN_INFO\n");
        diag_process_get_stateAndConnInfo_log();
    }
    else if (iocmd == DIAG_IOCTL_GET_KEY_EVENT_MASK) 
    {
	int *iStatus = (int *)ioarg;
       // printk(KERN_INFO "DIAG_IOCTL_GET_KEY_EVENT_MASK\n");
	*iStatus = (int)SLATE_EVENT_MASK_BIT_SET(0x1AE);
	//printk(KERN_INFO"%s:DIAG_IOCTL_GET_KEY_EVENT_MASK=%x\n", __func__,  *iStatus);	
    }
    else if (iocmd == DIAG_IOCTL_GET_PEN_EVENT_MASK) 
    {
	int *iStatus = (int *)ioarg;
        //printk(KERN_INFO "DIAG_IOCTL_GET_PEN_EVENT_MASK\n");
	*iStatus = (int)SLATE_EVENT_MASK_BIT_SET(0x1AF);//
	//printk(KERN_INFO"%s:DIAG_IOCTL_GET_PEN_EVENT_MASK=%x\n", __func__,  *iStatus);	
    }
	//FXPCAYM81 Ends here
	// Ketan_Changes-FXP - FXPCAYM-87
    else if (iocmd == DIAG_IOCTL_GET_SEARCHER_DUMP) {
		printk(KERN_INFO "DIAG_LOG_CMD_TYPE_GET_1x_SEARCHER_DUMP\n");

		diag_init_log_cmd(DIAG_LOG_CMD_TYPE_GET_1x_SEARCHER_DUMP);
	}
	else if (iocmd == DIAG_IOCTL_RESTORE_LOGGING_MASKS) {
		printk(KERN_INFO "DIAG_IOCTL_RESTORE_LOGGING_MASKS\n");
		diag_restore_log_masks();
	}
	else if (iocmd == DIAG_IOCTL_READ_DMSS_STATUS) {
		unsigned int t_cmd = 0;

		if (copy_from_user(&t_cmd,(void __user*)ioarg, 2)) 
		{
			printk(KERN_ERR "%s:%d copy_from_user failed \n", __func__, __LINE__);
			return -EFAULT;
		}

		printk(KERN_INFO "USB_MAX_OUT_BUF = %d\n", USB_MAX_OUT_BUF);

		switch ( t_cmd )
		{
			case 0x119A:
				printk(KERN_INFO "DIAG_IOCTL_READ_DMSS_STATUS copying 0x119A called\n");
				if (diag_rsp_119a == NULL) 
					success = -EFAULT;
				else 
				{
					if ( copy_to_user( (void __user*)ioarg, diag_rsp_119a, USB_MAX_OUT_BUF ) )
						return -EFAULT;

					success = 1;
				}
				break;
			case 0x0c:
				printk(KERN_INFO "DIAG_IOCTL_READ_DMSS_STATUS copying 0x0c called\n");
				if (diag_rsp_0c == NULL) 
					success = -EFAULT;
				else 
				{
					if ( copy_to_user( (void __user*)ioarg, diag_rsp_0c, USB_MAX_OUT_BUF ) )
						return -EFAULT;

					success = 1;
				}
				break;
			case 0x63:
				printk(KERN_INFO "DIAG_IOCTL_READ_DMSS_STATUS copying 0x63 called\n");
				if (diag_rsp_63 == NULL) 
					success = -EFAULT;
				else 
				{
					if ( copy_to_user( (void __user*)ioarg, diag_rsp_63, USB_MAX_OUT_BUF ) )
						return -EFAULT;

					success = 1;
				}
				break;
			case 0x1a:
				printk(KERN_INFO "DIAG_IOCTL_READ_DMSS_STATUS copying 0x1a called\n");
				if (diag_rsp_1a == NULL) 
					success = -EFAULT;
				else 
				{
					if ( copy_to_user( (void __user*)ioarg, diag_rsp_1a, USB_MAX_OUT_BUF ) )
						return -EFAULT;

					success = 1;
				}
				break;
			case 0x119B:
				printk(KERN_INFO "DIAG_IOCTL_READ_DMSS_STATUS copying 0x119B called\n");
				if (diag_rsp_119b == NULL) 
					success = -EFAULT;
				else 
				{
					if ( copy_to_user( (void __user*)ioarg, diag_rsp_119b, USB_MAX_OUT_BUF ) )
						return -EFAULT;

					success = 1;
				}
				break;
			default:
				printk(KERN_INFO "DIAG_IOCTL_READ_DMSS_STATUS copy failed, invalid cmd=%02X\n", t_cmd);
				break;
		}
	}
	else if (iocmd == DIAG_IOCTL_SEND_DMSS_STATUS ){
		unsigned char t_cmd[4];

		// IOCTL to send Custom commands (0x0c and 0x63)
		// no data needed. Hence only 4 bytes

		if (copy_from_user(&t_cmd,(void __user*)ioarg, 4)) 
		{
			printk(KERN_ERR "%s:%d copy_from_user failed \n", __func__, __LINE__);
			return -EFAULT;
		}

		if ( ! ( (t_cmd[0] == 0x0C) || (t_cmd[0] == 0x63) || (t_cmd[0] == 0x1a) ) )
		{
			printk(KERN_ERR "Command : 0x%2X not supported via this IOCTL\n", t_cmd[0]);
			return -EFAULT;
		}

		printk(KERN_INFO "DIAG_IOCTL_SEND_DMSS_STATUS ioctl called\n");

		if (t_cmd[0] == 0x0C) 
		{
			drop_0c_packet = 1;
		}
		else if (t_cmd[0] == 0x63) 
		{
			drop_63_packet = 1;
		}
		else if (t_cmd[0] == 0x1a) 
		{
			drop_1a_packet = 1;
		}
		
		// Send raw command
		diag_process_hdlc((void *)&t_cmd, 4);

		success = 1;
	}

	return success;
}

static int diagchar_read(struct file *file, char __user *buf, size_t count,
			  loff_t *ppos)
{
	int index = -1, i = 0, ret = 0;
	int num_data = 0, data_type;
	for (i = 0; i < driver->num_clients; i++)
		if (driver->client_map[i].pid == current->tgid)
			index = i;

	if (index == -1) {
		printk(KERN_ALERT "\n Client PID not found in table");
		return -EINVAL;
	}

	wait_event_interruptible(driver->wait_q,
				  driver->data_ready[index]);
	mutex_lock(&driver->diagchar_mutex);

	if ((driver->data_ready[index] & MEMORY_DEVICE_LOG_TYPE) && (driver->
					logging_mode == MEMORY_DEVICE_MODE)) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & MEMORY_DEVICE_LOG_TYPE;
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		/* place holder for number of data field */
		ret += 4;

		for (i = 0; i < driver->poolsize_usb_struct; i++) {
			if (driver->buf_tbl[i].length > 0) {
#ifdef DIAG_DEBUG
				printk(KERN_INFO "\n WRITING the buf address "
				       "and length is %x , %d\n", (unsigned int)
					(driver->buf_tbl[i].buf),
					driver->buf_tbl[i].length);
#endif
				num_data++;
				/* Copy the length of data being passed */
				if (copy_to_user(buf+ret, (void *)&(driver->
						buf_tbl[i].length), 4)) {
						num_data--;
						goto drop;
				}
				ret += 4;

				/* Copy the actual data being passed */
				if (copy_to_user(buf+ret, (void *)driver->
				buf_tbl[i].buf, driver->buf_tbl[i].length)) {
					ret -= 4;
					num_data--;
					goto drop;
				}
				ret += driver->buf_tbl[i].length;
drop:
#ifdef DIAG_DEBUG
				printk(KERN_INFO "\n DEQUEUE buf address and"
				       " length is %x,%d\n", (unsigned int)
				       (driver->buf_tbl[i].buf), driver->
				       buf_tbl[i].length);
#endif
				diagmem_free(driver, (unsigned char *)
				(driver->buf_tbl[i].buf), POOL_TYPE_HDLC);
				driver->buf_tbl[i].length = 0;
				driver->buf_tbl[i].buf = 0;
			}
		}

		/* copy modem data */
		if (driver->in_busy_1 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					 (driver->usb_write_ptr_1->length), 4);
			/*Copy the actual data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					*(driver->usb_buf_in_1),
					 driver->usb_write_ptr_1->length);
			driver->in_busy_1 = 0;
		}
		if (driver->in_busy_2 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					 (driver->usb_write_ptr_2->length), 4);
			/*Copy the actual data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret,
					 *(driver->usb_buf_in_2),
					 driver->usb_write_ptr_2->length);
			driver->in_busy_2 = 0;
		}

		/* copy q6 data */
		if (driver->in_busy_qdsp_1 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret,
				 (driver->usb_write_ptr_qdsp_1->length), 4);
			/*Copy the actual data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret, *(driver->
							usb_buf_in_qdsp_1),
					 driver->usb_write_ptr_qdsp_1->length);
			driver->in_busy_qdsp_1 = 0;
		}
		if (driver->in_busy_qdsp_2 == 1) {
			num_data++;
			/*Copy the length of data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret,
				 (driver->usb_write_ptr_qdsp_2->length), 4);
			/*Copy the actual data being passed*/
			COPY_USER_SPACE_OR_EXIT(buf+ret, *(driver->
				usb_buf_in_qdsp_2), driver->
					usb_write_ptr_qdsp_2->length);
			driver->in_busy_qdsp_2 = 0;
		}

		/* copy number of data fields */
		COPY_USER_SPACE_OR_EXIT(buf+4, num_data, 4);
		ret -= 4;
		driver->data_ready[index] ^= MEMORY_DEVICE_LOG_TYPE;
		if (driver->ch)
			queue_work(driver->diag_wq,
					 &(driver->diag_read_smd_work));
		if (driver->chqdsp)
			queue_work(driver->diag_wq,
					 &(driver->diag_read_smd_qdsp_work));
		APPEND_DEBUG('n');
		goto exit;
	} else if (driver->data_ready[index] & MEMORY_DEVICE_LOG_TYPE) {
		/* In case, the thread wakes up and the logging mode is
		not memory device any more, the condition needs to be cleared */
		driver->data_ready[index] ^= MEMORY_DEVICE_LOG_TYPE;
	}

	if (driver->data_ready[index] & DEINIT_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & DEINIT_TYPE;
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		driver->data_ready[index] ^= DEINIT_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & MSG_MASKS_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & MSG_MASKS_TYPE;
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->msg_masks),
							 MSG_MASK_SIZE);
		driver->data_ready[index] ^= MSG_MASKS_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & EVENT_MASKS_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & EVENT_MASKS_TYPE;
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->event_masks),
							 EVENT_MASK_SIZE);
		driver->data_ready[index] ^= EVENT_MASKS_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & LOG_MASKS_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & LOG_MASKS_TYPE;
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->log_masks),
							 LOG_MASK_SIZE);
		driver->data_ready[index] ^= LOG_MASKS_TYPE;
		goto exit;
	}

	if (driver->data_ready[index] & PKT_TYPE) {
		/*Copy the type of data being passed*/
		data_type = driver->data_ready[index] & PKT_TYPE;
		COPY_USER_SPACE_OR_EXIT(buf, data_type, 4);
		COPY_USER_SPACE_OR_EXIT(buf+4, *(driver->pkt_buf),
							 driver->pkt_length);
		driver->data_ready[index] ^= PKT_TYPE;
		goto exit;
	}

exit:
	mutex_unlock(&driver->diagchar_mutex);
	return ret;
}

static int diagchar_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	int err, ret = 0, pkt_type;
#ifdef DIAG_DEBUG
	int length = 0, i;
#endif
	struct diag_send_desc_type send = { NULL, NULL, DIAG_STATE_START, 0 };
	struct diag_hdlc_dest_type enc = { NULL, NULL, 0 };
	void *buf_copy = NULL;
	int payload_size;

	if (((driver->logging_mode == USB_MODE) && (!driver->usb_connected)) ||
				(driver->logging_mode == NO_LOGGING_MODE)) {
		/*Drop the diag payload */
		return -EIO;
	}

	/* Get the packet type F3/log/event/Pkt response */
	err = copy_from_user((&pkt_type), buf, 4);
	/*First 4 bytes indicate the type of payload - ignore these */
	payload_size = count - 4;

	if (pkt_type == MEMORY_DEVICE_LOG_TYPE) {
		if (!mask_request_validate((unsigned char *)buf)) {
			printk(KERN_ALERT "mask request Invalid ..cannot send to modem \n");
			return -EFAULT;
		}
		buf = buf + 4;
#ifdef DIAG_DEBUG
		printk(KERN_INFO "\n I got the masks: %d\n", payload_size);
		for (i = 0; i < payload_size; i++)
			printk(KERN_DEBUG "\t %x", *(((unsigned char *)buf)+i));
#endif
		diag_process_hdlc((void *)buf, payload_size);
		return 0;
	}

	buf_copy = diagmem_alloc(driver, payload_size, POOL_TYPE_COPY);
	if (!buf_copy) {
		driver->dropped_count++;
		return -ENOMEM;
	}

	err = copy_from_user(buf_copy, buf + 4, payload_size);
	if (err) {
		printk(KERN_INFO "diagchar : copy_from_user failed\n");
		ret = -EFAULT;
		goto fail_free_copy;
	}
#ifdef DIAG_DEBUG
	printk(KERN_DEBUG "data is -->\n");
	for (i = 0; i < payload_size; i++)
		printk(KERN_DEBUG "\t %x \t", *(((unsigned char *)buf_copy)+i));
#endif
	send.state = DIAG_STATE_START;
	send.pkt = buf_copy;
	send.last = (void *)(buf_copy + payload_size - 1);
	send.terminate = 1;
#ifdef DIAG_DEBUG
	printk(KERN_INFO "\n Already used bytes in buffer %d, and"
	" incoming payload size is %d\n", driver->used, payload_size);
	printk(KERN_DEBUG "hdlc encoded data is -->\n");
	for (i = 0; i < payload_size + 8; i++) {
		printk(KERN_DEBUG "\t %x \t", *(((unsigned char *)buf_hdlc)+i));
		if (*(((unsigned char *)buf_hdlc)+i) != 0x7e)
			length++;
	}
#endif
	mutex_lock(&driver->diagchar_mutex);
	if (!buf_hdlc)
		buf_hdlc = diagmem_alloc(driver, HDLC_OUT_BUF_SIZE,
						 POOL_TYPE_HDLC);
	if (!buf_hdlc) {
		ret = -ENOMEM;
		goto fail_free_hdlc;
	}
	if (HDLC_OUT_BUF_SIZE - driver->used <= (2*payload_size) + 3) {
		err = diag_device_write(buf_hdlc, APPS_DATA, NULL);
		if (err) {
			/*Free the buffer right away if write failed */
			diagmem_free(driver, buf_hdlc, POOL_TYPE_HDLC);
			diagmem_free(driver, (unsigned char *)driver->
				 usb_write_ptr_svc, POOL_TYPE_USB_STRUCT);
			ret = -EIO;
			goto fail_free_hdlc;
		}
		buf_hdlc = NULL;
#ifdef DIAG_DEBUG
		printk(KERN_INFO "\n size written is %d\n", driver->used);
#endif
		driver->used = 0;
		buf_hdlc = diagmem_alloc(driver, HDLC_OUT_BUF_SIZE,
							 POOL_TYPE_HDLC);
		if (!buf_hdlc) {
			ret = -ENOMEM;
			goto fail_free_hdlc;
		}
	}

	enc.dest = buf_hdlc + driver->used;
	enc.dest_last = (void *)(buf_hdlc + driver->used + 2*payload_size + 3);
	diag_hdlc_encode(&send, &enc);

	/* This is to check if after HDLC encoding, we are still within the
	 limits of aggregation buffer. If not, we write out the current buffer
	and start aggregation in a newly allocated buffer */
	if ((unsigned int) enc.dest >=
		 (unsigned int)(buf_hdlc + HDLC_OUT_BUF_SIZE)) {
		err = diag_device_write(buf_hdlc, APPS_DATA, NULL);
		if (err) {
			/*Free the buffer right away if write failed */
			diagmem_free(driver, buf_hdlc, POOL_TYPE_HDLC);
			diagmem_free(driver, (unsigned char *)driver->
				 usb_write_ptr_svc, POOL_TYPE_USB_STRUCT);
			ret = -EIO;
			goto fail_free_hdlc;
		}
		buf_hdlc = NULL;
#ifdef DIAG_DEBUG
		printk(KERN_INFO "\n size written is %d\n", driver->used);
#endif
		driver->used = 0;
		buf_hdlc = diagmem_alloc(driver, HDLC_OUT_BUF_SIZE,
							 POOL_TYPE_HDLC);
		if (!buf_hdlc) {
			ret = -ENOMEM;
			goto fail_free_hdlc;
		}
		enc.dest = buf_hdlc + driver->used;
		enc.dest_last = (void *)(buf_hdlc + driver->used +
							 (2*payload_size) + 3);
		diag_hdlc_encode(&send, &enc);
	}

	driver->used = (uint32_t) enc.dest - (uint32_t) buf_hdlc;
 //   printk(KERN_INFO "diagchar_write: pkt_type=%d\n", pkt_type);
 //Slate comand changes FXPCAYM81
	if ( (pkt_type == DATA_TYPE_RESPONSE) || (pkt_type == 0) ){ // 0 = DIAG_DATA_TYPE_EVENT
		err = diag_device_write(buf_hdlc, APPS_DATA,NULL);
		if (err) {
			/*Free the buffer right away if write failed */
			diagmem_free(driver, buf_hdlc, POOL_TYPE_HDLC);
			diagmem_free(driver, (unsigned char *)driver->
				 usb_write_ptr_svc, POOL_TYPE_USB_STRUCT);
			ret = -EIO;
			goto fail_free_hdlc;
		}
		buf_hdlc = NULL;
#ifdef DIAG_DEBUG
		printk(KERN_INFO "\n size written is %d\n", driver->used);
#endif
		driver->used = 0;
	}

	mutex_unlock(&driver->diagchar_mutex);
	diagmem_free(driver, buf_copy, POOL_TYPE_COPY);
	if (!timer_in_progress)	{
		timer_in_progress = 1;
		ret = mod_timer(&drain_timer, jiffies + msecs_to_jiffies(500));
	}
	return 0;

fail_free_hdlc:
	buf_hdlc = NULL;
	driver->used = 0;
	diagmem_free(driver, buf_copy, POOL_TYPE_COPY);
	mutex_unlock(&driver->diagchar_mutex);
	return ret;

fail_free_copy:
	diagmem_free(driver, buf_copy, POOL_TYPE_COPY);
	return ret;
}

int mask_request_validate(unsigned char mask_buf[])
{
	uint8_t packet_id;
	uint8_t subsys_id;
	uint16_t ss_cmd;

	packet_id = mask_buf[4];

	if (packet_id == 0x4B) {
		subsys_id = mask_buf[5];
		ss_cmd = *(uint16_t *)(mask_buf + 6);
		/* Packets with SSID which are allowed */
		switch (subsys_id) {
		case 0x04: /* DIAG_SUBSYS_WCDMA */
			if ((ss_cmd == 0) || (ss_cmd == 0xF))
				return 1;
			break;
		case 0x08: /* DIAG_SUBSYS_GSM */
			if ((ss_cmd == 0) || (ss_cmd == 0x1))
				return 1;
			break;
		case 0x09: /* DIAG_SUBSYS_UMTS */
		case 0x0F: /* DIAG_SUBSYS_CM */
			if (ss_cmd == 0)
				return 1;
			break;
		case 0x0C: /* DIAG_SUBSYS_OS */
			if ((ss_cmd == 2) || (ss_cmd == 0x100))
				return 1; /* MPU and APU */
			break;
		case 0x12: /* DIAG_SUBSYS_DIAG_SERV */
			if ((ss_cmd == 0) || (ss_cmd == 0x6) || (ss_cmd == 0x7))
				return 1;
			break;
		case 0x13: /* DIAG_SUBSYS_FS */
			if ((ss_cmd == 0) || (ss_cmd == 0x1))
				return 1;
			break;
		default:
			return 0;
			break;
		}
	} else {
		switch (packet_id) {
		case 0x00:    /* Version Number */
		case 0x0C:    /* CDMA status packet */
		case 0x1C:    /* Diag Version */
		case 0x1D:    /* Time Stamp */
		case 0x60:    /* Event Report Control */
		case 0x63:    /* Status snapshot */
		case 0x73:    /* Logging Configuration */
		case 0x7C:    /* Extended build ID */
		case 0x7D:    /* Extended Message configuration */
		case 0x81:    /* Event get mask */
		case 0x82:    /* Set the event mask */
			return 1;
			break;
		default:
			return 0;
			break;
		}
	}
	return 0;
}

static const struct file_operations diagcharfops = {
	.owner = THIS_MODULE,
	.read = diagchar_read,
	.write = diagchar_write,
	.ioctl = diagchar_ioctl,
	.open = diagchar_open,
	.release = diagchar_close
};

static int diagchar_setup_cdev(dev_t devno)
{

	int err;

	cdev_init(driver->cdev, &diagcharfops);

	driver->cdev->owner = THIS_MODULE;
	driver->cdev->ops = &diagcharfops;

	err = cdev_add(driver->cdev, devno, 1);

	if (err) {
		printk(KERN_INFO "diagchar cdev registration failed !\n\n");
		return -1;
	}

	driver->diagchar_class = class_create(THIS_MODULE, "diag");

	if (IS_ERR(driver->diagchar_class)) {
		printk(KERN_ERR "Error creating diagchar class.\n");
		return -1;
	}

	device_create(driver->diagchar_class, NULL, devno,
				  (void *)driver, "diag");

	return 0;

}

static int diagchar_cleanup(void)
{
	if (driver) {
		if (driver->cdev) {
			/* TODO - Check if device exists before deleting */
			device_destroy(driver->diagchar_class,
				       MKDEV(driver->major,
					     driver->minor_start));
			cdev_del(driver->cdev);
		}
		if (!IS_ERR(driver->diagchar_class))
			class_destroy(driver->diagchar_class);
		kfree(driver);
	}
	return 0;
}

static int __init diagchar_init(void)
{
	dev_t dev;
	int error;

	printk(KERN_INFO "diagfwd initializing ..\n");
	driver = kzalloc(sizeof(struct diagchar_dev) + 5, GFP_KERNEL);

	if (driver) {
		driver->used = 0;
		timer_in_progress = 0;
		driver->debug_flag = 1;
		driver->alert_count = 0;
		setup_timer(&drain_timer, drain_timer_func, 1234);
		driver->itemsize = itemsize;
		driver->poolsize = poolsize;
		driver->itemsize_hdlc = itemsize_hdlc;
		driver->poolsize_hdlc = poolsize_hdlc;
		driver->itemsize_usb_struct = itemsize_usb_struct;
		driver->poolsize_usb_struct = poolsize_usb_struct;
		driver->num_clients = max_clients;
		driver->logging_mode = USB_MODE;
		mutex_init(&driver->diagchar_mutex);
		init_waitqueue_head(&driver->wait_q);
		INIT_WORK(&(driver->diag_drain_work), diag_drain_work_fn);
		INIT_WORK(&(driver->diag_read_smd_work), diag_read_smd_work_fn);
		INIT_WORK(&(driver->diag_read_smd_qdsp_work),
			   diag_read_smd_qdsp_work_fn);
		diagfwd_init();
		printk(KERN_INFO "diagchar initializing ..\n");
		driver->num = 1;
		driver->name = ((void *)driver) + sizeof(struct diagchar_dev);
		strlcpy(driver->name, "diag", 4);

		/* Get major number from kernel and initialize */
		error = alloc_chrdev_region(&dev, driver->minor_start,
					    driver->num, driver->name);
		if (!error) {
			driver->major = MAJOR(dev);
			driver->minor_start = MINOR(dev);
		} else {
			printk(KERN_INFO "Major number not allocated\n");
			goto fail;
		}
		driver->cdev = cdev_alloc();
		error = diagchar_setup_cdev(dev);
		if (error)
			goto fail;

		diag_rsp_0c = kzalloc(USB_MAX_OUT_BUF, GFP_KERNEL);
		if (diag_rsp_0c) {
			memset(diag_rsp_0c, 0, USB_MAX_OUT_BUF);
		} else {
			printk(KERN_INFO "kzalloc failed for diag_rsp_0c\n");
			goto fail;
		}
		diag_rsp_63 = kzalloc(USB_MAX_OUT_BUF, GFP_KERNEL);
		if (diag_rsp_63) {
			memset(diag_rsp_63, 0, USB_MAX_OUT_BUF);
		} else {
			printk(KERN_INFO "kzalloc failed for diag_rsp_63\n");
			goto fail2;
		}
		diag_rsp_119a = kzalloc(USB_MAX_OUT_BUF, GFP_KERNEL);
		if (diag_rsp_119a) {
			memset(diag_rsp_119a, 0, USB_MAX_OUT_BUF);
		} else {
			printk(KERN_INFO "kzalloc failed for diag_rsp_119a\n");
			goto fail3;
		}
		diag_rsp_119b = kzalloc(USB_MAX_OUT_BUF, GFP_KERNEL);
		if (diag_rsp_119b) {
			memset(diag_rsp_119b, 0, USB_MAX_OUT_BUF);
		} else {
			printk(KERN_INFO "kzalloc failed for diag_rsp_119b\n");
			goto fail4;
		}
		diag_rsp_1a = kzalloc(USB_MAX_OUT_BUF, GFP_KERNEL);
		if (diag_rsp_1a) {
			memset(diag_rsp_1a, 0, USB_MAX_OUT_BUF);
		} else {
			printk(KERN_INFO "kzalloc failed for diag_rsp_1a\n");
			goto fail5;
		}
	} else {
		printk(KERN_INFO "kzalloc failed\n");
		goto fail;
	}

	printk(KERN_INFO "diagchar initialized\n");
	return 0;
fail5:
	kfree(diag_rsp_119b);
fail4:
	kfree(diag_rsp_119a);
fail3:
	kfree(diag_rsp_63);
fail2:
	kfree(diag_rsp_0c);
fail:
	diagchar_cleanup();
	diagfwd_exit();
	return -1;

}

static void __exit diagchar_exit(void)
{
	printk(KERN_INFO "diagchar exiting ..\n");

	kfree(diag_rsp_0c);
	kfree(diag_rsp_63);
	kfree(diag_rsp_1a);
	kfree(diag_rsp_119a);
	kfree(diag_rsp_119b);

	diagmem_exit(driver);
	diagfwd_exit();
	diagchar_cleanup();
	printk(KERN_INFO "done diagchar exit\n");
}

module_init(diagchar_init);
module_exit(diagchar_exit);
