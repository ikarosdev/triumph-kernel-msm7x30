/*
 *  linux/fs/proc/mlog.c
 *
 *  Copyright (C) 2010  by FIHTDC HenryMCWang
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/msm_iomap.h>

#include "internal.h"

static int offset = 0;

typedef struct{
	unsigned magic_number:16;
	unsigned blk0_avail :1;
	unsigned blk1_avail :1;
	unsigned current_blk_idx:1;
	unsigned log_status:2; 
	//0:only save blk0; 
	//1:only save blk1; 
	//2:both save blk0 & blk1; blk0 first
	//3:both save blk0 & blk1; blk1 first
	unsigned smem_log_flag:16;
	unsigned powerOnCause:32;
}Oemdbg_Hdr;

static int mlog_open(struct inode * inode, struct file * file)
{
	return 0;
}

static int mlog_release(struct inode * inode, struct file * file)
{
	return 0;
}

static ssize_t mlog_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned int err;
	
	err = copy_to_user(buf, MSM_MLOG_BASE + offset, count);

	return -1;
}

static unsigned int mlog_poll(struct file *file, poll_table *wait)
{
	printk(KERN_INFO "%s:%s LINE:%d \n", __FILE__, __func__, __LINE__);
	return 0;
}

static int mlog_ioctl(struct inode * inodep, struct file * filep, unsigned int cmd, unsigned long arg )
{
	Oemdbg_Hdr *p_oemdbg_header;
	
	if(arg == 0)
	{
		printk( KERN_INFO "%s: arg = 0\n", __func__);
		p_oemdbg_header = (Oemdbg_Hdr *)MSM_MLOG_BASE;
		p_oemdbg_header->blk0_avail = 1;
	}
	else if(arg == 1)
	{
		printk( KERN_INFO "%s: arg = 1\n", __func__);
		p_oemdbg_header = (Oemdbg_Hdr *)MSM_MLOG_BASE;
		p_oemdbg_header->blk1_avail = 1;
	}
	else if(arg == 2)
	{
		printk( KERN_INFO "%s: arg = 2\n", __func__);
		p_oemdbg_header = (Oemdbg_Hdr *)MSM_MLOG_BASE;
		p_oemdbg_header->smem_log_flag = 0xFFFF;
	}
	else
	{
		offset = arg - 3;
	}
		
	return 0;
}

const struct file_operations proc_mlog_operations = {
	.read		= mlog_read,
	.poll		= mlog_poll,
	.open		= mlog_open,
	.ioctl	= mlog_ioctl,
	.release	= mlog_release,
};

static int __init proc_mlog_init(void)
{
	printk(KERN_INFO "%s:%s LINE:%d \n", __FILE__, __func__, __LINE__);
	proc_create("mlog", S_IRUSR, NULL, &proc_mlog_operations);
	return 0;
}
module_init(proc_mlog_init);
