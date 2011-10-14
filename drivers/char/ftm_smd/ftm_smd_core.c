/*==========================================================================*
 * Copyright   2009 Chi Mei Communication Systems, Inc.
 *--------------------------------------------------------------------------*
 * PROJECT:
 * CREATED BY:  Lo, Chien Chung (chienchung.lo@gmail.com)
 * CREATED DATE: 04-16-2009
 *--------------------------------------------------------------------------*
 * PURPOSE: A char type driver of smd
 *==========================================================================*/

/*-------------------------------------------------------------------------*/
/*                         DEPENDANCY                                      */
/*-------------------------------------------------------------------------*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/crc-ccitt.h>
#include <linux/delay.h>
#include <mach/msm_smd.h>

#include "ftm_smd.h"
//FIH, WillChen, 2009/7/3++
/* [FXX_CR], Merge bsp kernel and ftm kernel*/
//#ifdef CONFIG_FIH_FXX
//extern int ftm_mode;
//#endif
//FIH, WillChen, 2009/7/3--
/*-------------------------------------------------------------------------*/
/*                         TYPE/CONSTANT/MACRO                             */
/*-------------------------------------------------------------------------*/
#if 1
#define D(x...) printk(x)
#else
#define D(x...)
#endif

/*-------------------------------------------------------------------------*/
/*                            VARIABLES                                    */
/*-------------------------------------------------------------------------*/
struct ftm_smd_char_dev *ftm_smd_driver;

/*-------------------------------------------------------------------------*/
/*                         Function prototype                             */
/*-------------------------------------------------------------------------*/
int ftm_smd_hdlc_decode(struct ftm_smd_hdlc_decode_type *hdlc);

static void ftm_smd_notify(void *ctxt, unsigned event);
static int ftm_smd_open(struct inode *inode, struct file *filp);
static int ftm_smd_release(struct inode *inode, struct file *filp);
static ssize_t ftm_smd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t ftm_smd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static unsigned int ftm_smd_poll(struct file *filp, struct poll_table_struct *wait);
static int ftm_smd_cleanup(void);
static int ftm_smd_setup_cdev(dev_t devno);
static void ftm_smd_init_mem(void);
static void ftm_smd_cleanup_mem(void);
static int __init ftm_smd_init(void);
static void __exit ftm_smd_exit(void);

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
int ftm_smd_hdlc_decode(struct ftm_smd_hdlc_decode_type *hdlc)
{
	uint8_t *src_ptr = NULL, *dest_ptr = NULL;
	unsigned int src_length = 0, dest_length = 0;

	unsigned int len = 0;
	unsigned int i;
	uint8_t src_byte;

	int pkt_bnd = 0;

	if (hdlc && hdlc->src_ptr && hdlc->dest_ptr &&
	    (hdlc->src_size - hdlc->src_idx > 0) &&
	    (hdlc->dest_size - hdlc->dest_idx > 0)) {

		src_ptr = hdlc->src_ptr;
		src_ptr = &src_ptr[hdlc->src_idx];
		src_length = hdlc->src_size - hdlc->src_idx;

		dest_ptr = hdlc->dest_ptr;
		dest_ptr = &dest_ptr[hdlc->dest_idx];
		dest_length = hdlc->dest_size - hdlc->dest_idx;

		for (i = 0; i < src_length; i++) {

			src_byte = src_ptr[i];

			if (hdlc->escaping) {
				dest_ptr[len++] = src_byte ^ ESC_MASK;
				hdlc->escaping = 0;
			} else if (src_byte == ESC_CHAR) {
				if (i == (src_length - 1)) {
					hdlc->escaping = 1;
					i++;
					break;
				} else {
					dest_ptr[len++] = src_ptr[++i]
							  ^ ESC_MASK;
				}
			} else if (src_byte == CONTROL_CHAR) {
				dest_ptr[len++] = src_byte;
				pkt_bnd = 1;
				i++;
				break;
			} else {
				dest_ptr[len++] = src_byte;
			}

			if (len >= dest_length) {
				i++;
				break;
			}
		}

		hdlc->src_idx += i;
		hdlc->dest_idx += len;
	}

	return pkt_bnd;
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static void ftm_smd_notify(void *ctxt, unsigned event)
{
	//D(KERN_INFO "ftm_smd_notify! \n");

	if (event != SMD_EVENT_DATA)
		return;

	ftm_smd_driver->data_ready = 1;
	wake_up_interruptible(&ftm_smd_driver->wait_q);
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static int ftm_smd_open(struct inode *inode, struct file *filp)
{
	int result = 0;

	printk(KERN_INFO "ftm_smd_open! \n");
//FIH, WillChen, 2009/7/3++
/* [FXX_CR], Merge bsp kernel and ftm kernel*/
//#ifdef CONFIG_FIH_FXX
//	if (ftm_mode == 0)
//	{
//		printk(KERN_INFO "This is NOT FTM mode. return without open!!!\n");
//		return -ENOMEM;
//	}
//#endif
//FIH, WillChen, 2009/7/3--

	if (ftm_smd_driver) {
		mutex_lock(&ftm_smd_driver->ftm_smd_mutex);

		result = smd_open("DIAG", &ftm_smd_driver->ch, ftm_smd_driver, ftm_smd_notify);

		mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);
		
		printk(KERN_INFO "diag opened SMD port ; r = %d\n", result);
		
		return result;
	}
	return -ENOMEM;	
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description 	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static int ftm_smd_release(struct inode *inode, struct file *filp)
{
	D(KERN_INFO "ftm_smd_release! \n");

	mutex_lock(&ftm_smd_driver->ftm_smd_mutex);
	smd_close(ftm_smd_driver->ch);
	mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);
	
	return 0;
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description 	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static ssize_t ftm_smd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	void *smd_buf;
	int len = 0;
	int err;
	//int i;

	//D(KERN_INFO "ftm_smd_read! \n");

	mutex_lock(&ftm_smd_driver->ftm_smd_mutex);

	len = smd_read_avail(ftm_smd_driver->ch);
	while(len <= 0){
		if(filp->f_flags & O_NONBLOCK) {
			mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);
			return -EAGAIN;
		}

		if(wait_event_interruptible(ftm_smd_driver->wait_q, ftm_smd_driver->data_ready)) {
			mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);
			return -ERESTARTSYS;
		}
		
		len = smd_read_avail(ftm_smd_driver->ch);
	}

	if (len > MAX_BUF) {
		printk(KERN_INFO "diag dropped num bytes = %d\n", len);
		ftm_smd_driver->data_ready = 0;
		mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);
		return -EFAULT;
	}
	else {

		smd_buf = ftm_smd_driver->buf;
		if (!smd_buf) {
			
			printk(KERN_INFO "Out of diagmem for a9\n");

		} 
		else {
			smd_read_from_cb(ftm_smd_driver->ch, smd_buf, len);


			//if(len > 0){
			//	D("\n");
			//	D("ftm_smd_read: len=%x\n", len);
			//	for(i = 0; i < len; i++)
			//		D("%2x ", ftm_smd_driver->buf[i]);
			//	D("\n\n");
			//}

			err = copy_to_user(buf, smd_buf, len);
			if(err){
				printk(KERN_INFO "copy_to_user result = %d\n", len);
				ftm_smd_driver->data_ready = 0;
				mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);
				return -EFAULT;
			}
		}
	}

	ftm_smd_driver->data_ready = 0;


	mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);
	
	return len;
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description 	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static ssize_t ftm_smd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct ftm_smd_hdlc_decode_type hdlc;
	
	int ret;
	int err;
	int i;
	unsigned int len = 0;

	//D(KERN_INFO "ftm_smd_write! \n");

	mutex_lock(&ftm_smd_driver->ftm_smd_write_mutex);

	err = copy_from_user(ftm_smd_driver->buf, buf, count);
	if (err) {
		printk(KERN_INFO "ftm_smd : copy_from_user failed \n");
		mutex_unlock(&ftm_smd_driver->ftm_smd_write_mutex);
		return -EFAULT;
	}

	len = count;

	//D("\n");
	//D("ftm_smd_write: len=%x\n", len);
	//for(i = 0; i < len; i++)
	//	D("%2x ", ftm_smd_driver->buf[i]);
	//D("\n\n");

	hdlc.dest_ptr = ftm_smd_driver->hdlc_buf;
	hdlc.dest_size = MAX_BUF;
	hdlc.src_ptr = ftm_smd_driver->buf;
	hdlc.src_size = len;
	hdlc.src_idx = 0;
	hdlc.dest_idx = 0;
	hdlc.escaping = 0;

	ret = ftm_smd_hdlc_decode(&hdlc);

	if(!ret){
		printk("\n");
		printk("this will be discarded: len=%x\n", len);
		for(i = 0; i < len; i++)
			printk("%2x ", ftm_smd_driver->buf[i]);
		printk("\n\n");

		for(i = 0; i < hdlc.dest_idx; i++)
			printk("%2x ", ftm_smd_driver->hdlc_buf[i]);
		printk("\n\n");

		return count;
	}

	if ((ftm_smd_driver->ch) && (ret) && (hdlc.dest_idx > 3)){
		
		//D("\n");
		//D("HDLC: len=%x\n", hdlc.dest_idx);
		//for(i = 0; i < hdlc.dest_idx; i++)
		//	D("%2x ", ftm_smd_driver->hdlc_buf[i]);
		//D("\n\n");

		len = hdlc.dest_idx - 3;
		ret = smd_write_avail(ftm_smd_driver->ch);
		while (ret < len) {
			mutex_unlock(&ftm_smd_driver->ftm_smd_write_mutex);
			msleep(250);
			mutex_lock(&ftm_smd_driver->ftm_smd_write_mutex);
			ret = smd_write_avail(ftm_smd_driver->ch);
	}

		ret = smd_write(ftm_smd_driver->ch, ftm_smd_driver->hdlc_buf, len);
	
		if (ret != len) {
			printk(KERN_ERR "ERROR:%s:%i:%s: "
				"smd_write(ch,buf,count = %i) ret %i.\n",
				__FILE__,
				__LINE__,
				__func__,
				len,
				ret);
		}
		
	}
	
	mutex_unlock(&ftm_smd_driver->ftm_smd_write_mutex);
	
	return count;
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static unsigned int ftm_smd_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned mask = 0;
	int len = 0;

	mutex_lock(&ftm_smd_driver->ftm_smd_mutex);
	len = smd_read_avail(ftm_smd_driver->ch);
	if(len > 0)
		mask |= POLLIN | POLLRDNORM;

	if (!mask) {
		poll_wait(filp, &ftm_smd_driver->wait_q, wait);
		len = smd_read_avail(ftm_smd_driver->ch);
		if(len > 0)
			mask |= POLLIN | POLLRDNORM;
	}
	mutex_unlock(&ftm_smd_driver->ftm_smd_mutex);

	return mask;
}

/*-------------------------------------------------------------------------*/
/*                         file operation                                      */
/*-------------------------------------------------------------------------*/
struct file_operations ftm_smd_fops = {
	.owner =	THIS_MODULE,	
	.read =	ftm_smd_read,
	.write =	ftm_smd_write,
	.poll =	ftm_smd_poll,
	.open =	ftm_smd_open,
	.release =	ftm_smd_release,
};

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static int ftm_smd_cleanup(void)
{
	if (ftm_smd_driver) {
		if (ftm_smd_driver->cdev) {
			/* TODO - Check if device exists before deleting */
			device_destroy(ftm_smd_driver->ftm_smd_class,
				MKDEV(ftm_smd_driver->major,
				ftm_smd_driver->minor));
			
			cdev_del(ftm_smd_driver->cdev);
		}
		
		if (!IS_ERR(ftm_smd_driver->ftm_smd_class))
			class_destroy(ftm_smd_driver->ftm_smd_class);
		
		kfree(ftm_smd_driver);
	}

	return 0;
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static int ftm_smd_setup_cdev(dev_t devno)
{
	int err;

	cdev_init(ftm_smd_driver->cdev, &ftm_smd_fops);

	ftm_smd_driver->cdev->owner = THIS_MODULE;
	ftm_smd_driver->cdev->ops = &ftm_smd_fops;

	err = cdev_add(ftm_smd_driver->cdev, devno, 1);

	if (err) {
		printk(KERN_INFO "ftm_smd cdev registration failed !\n\n");
		return -1;
	}

	ftm_smd_driver->ftm_smd_class = class_create(THIS_MODULE, "ftm_smd");

	if (IS_ERR(ftm_smd_driver->ftm_smd_class)) {
		printk(KERN_ERR "Error creating ftm_smd class.\n");
		return -1;
	}

	device_create(ftm_smd_driver->ftm_smd_class, NULL, devno,
		(void *)ftm_smd_driver, "ftm_smd");

	return 0;
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static void ftm_smd_init_mem(void)
{
	if (ftm_smd_driver->buf  == NULL &&
		(ftm_smd_driver->buf = kzalloc(MAX_BUF, GFP_KERNEL)) == NULL)
			goto err;
	
	if (ftm_smd_driver->hdlc_buf == NULL
		&& (ftm_smd_driver->hdlc_buf = kzalloc(HDLC_MAX, GFP_KERNEL)) == NULL)
			goto err;

	return;

err:
	printk(KERN_INFO "\n Could not initialize ftm_smd buffers \n");
	kfree(ftm_smd_driver->buf);
	kfree(ftm_smd_driver->hdlc_buf);
}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static void ftm_smd_cleanup_mem(void)
{
	printk(KERN_INFO "ftm_smd_cleanup_mem!\n");

	ftm_smd_driver->ch = 0;

	kfree(ftm_smd_driver->buf);
	kfree(ftm_smd_driver->hdlc_buf);
}


/*-------------------------------------------------------------------------
 	Name 		: 	
	Description 	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static int __init ftm_smd_init(void)
{
	dev_t devno;
	int error;
	
	printk(KERN_INFO "FTM_SMD driver initializing ...\n");
//FIH, WillChen, 2009/7/3++
/* [FXX_CR], Merge bsp kernel and ftm kernel*/
//#ifdef CONFIG_FIH_FXX
//	if (ftm_mode == 0)
//	{
//		printk(KERN_INFO "This is NOT FTM mode. return without init!!!\n");
//		return -1;
//	}
//#endif
//FIH, WillChen, 2009/7/3--

	ftm_smd_driver = kzalloc(sizeof(struct ftm_smd_char_dev), GFP_KERNEL);	

	if (ftm_smd_driver) {

		ftm_smd_driver->data_ready = 0;
		
		init_waitqueue_head(&ftm_smd_driver->wait_q);
		mutex_init(&ftm_smd_driver->ftm_smd_mutex);
		mutex_init(&ftm_smd_driver->ftm_smd_write_mutex);
		ftm_smd_init_mem();
		
		/* Get major number from kernel and initialize */
		error = alloc_chrdev_region(&devno, 0, 1, "ftm_smd");
		if (!error) {
			ftm_smd_driver->major = MAJOR(devno);
			ftm_smd_driver->minor = MINOR(devno);
		} else {
			printk(KERN_INFO "Major number not allocated \n");
			goto fail;
		}
		
		ftm_smd_driver->cdev = cdev_alloc();
		
		error = ftm_smd_setup_cdev(devno);
		if (error)
			goto fail;

	} else {
		printk(KERN_INFO "kzalloc failed\n");
		goto fail;
	}

	printk(KERN_INFO "FTM_SMD driver initialized ...\n");
	
	return 0;

fail:
	printk(KERN_INFO "Failed to initialize FTM_SMD driver ...\n");
	ftm_smd_cleanup();
	return -1;

}

/*-------------------------------------------------------------------------
 	Name 		: 	
	Description	:	
	Parameter 	: 	
	Return 		: 	
-------------------------------------------------------------------------*/
static void __exit ftm_smd_exit(void)
{
	printk(KERN_INFO "diagchar exiting ..\n");

	ftm_smd_cleanup_mem(); // This should be first before ftm_smd_cleanup().
	ftm_smd_cleanup();
	
	printk(KERN_INFO "done diagchar exit\n");
}

module_init(ftm_smd_init);
module_exit(ftm_smd_exit);
MODULE_DESCRIPTION("FTM SMD Driver");
MODULE_AUTHOR("Lo, Chien Chung <chienchung.lo@gmail.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1");

