/*
*     ftmgpio.c - GPIO access driver for FTM use
*
*     Copyright (C) 2009 Clement Hsu <clementhsu@tp.cmcs.com.tw>
*     Copyright (C) 2009 Chi Mei Communication Systems Inc.
*
*     This program is free software; you can redistribute it and/or modify
*     it under the terms of the GNU General Public License as published by
*     the Free Software Foundation; version 2 of the License.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sysctl.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/major.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <asm/ioctl.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <mach/gpio.h>
#include "../../arch/arm/mach-msm/proc_comm.h"
#include "../../arch/arm/mach-msm/smd_private.h"
//FIH, MichaelKao, 2009/7/9++
/* [FXX_CR], Add for get Hardware version*/
#include <linux/io.h>
#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
//FIH, MichaelKao, 2009/7/9++
#include "ftmgpio.h"
/* Div1-FW3-BSP-AUDIO */
#include <mach/qdsp5v2/snddev_icodec.h>
#include <mach/pmic.h>
/* Div1-FW3-BSP-AUDIO */

//FIH, WillChen, 2009/7/3++
/* [FXX_CR], Merge bsp kernel and ftm kernel*/
//#ifdef CONFIG_FIH_FXX
//extern int ftm_mode;
//#endif
//FIH, WillChen, 2009/7/3--

extern void ftm_init_clock(int on);

//FIH, Henry Juang, 2009/08/10{++
/* [FXX_CR], Add for FTM PID and testflag*/
#define PID_length 128
char bufferPID_tsflag[PID_length];
char *pbuf;
//}FIH, Henry Juang, 2009/08/10--

struct cdev * g_ftmgpio_cdev = NULL;
char ioctl_operation=0;
int gpio_pin_num=0;

//static int isReadHW=0,isJogball=0;

static const struct file_operations ftmgpio_dev_fops = {
        .open = ftmgpio_dev_open,                   
        .read = ftmgpio_dev_read,                   
        .write = ftmgpio_dev_write,                   
        .ioctl = ftmgpio_dev_ioctl,                 
};                              

static struct miscdevice ftmgpio_dev = {
        MISC_DYNAMIC_MINOR,
        "ftmgpio",
        &ftmgpio_dev_fops
};

static int ftmgpio_dev_open( struct inode * inode, struct file * file )
{
	printk( KERN_INFO "FTM GPIO driver open\n" );

	if( ( file->f_flags & O_ACCMODE ) == O_WRONLY )
	{
		printk( KERN_INFO "FTM GPIO driver device node is readonly\n" );
		return -1;
	}
	else
		return 0;
}

static ssize_t ftmgpio_dev_read( struct file * file, char __user * buffer, size_t size, loff_t * f_pos )
{
	char *st,level;
//	int HWID=0;
	printk( KERN_INFO "FTM GPIO driver read, length=%d\n",size);

	if(ioctl_operation==IOCTL_GPIO_GET)	
	{

		gpio_direction_input(gpio_pin_num);
		level = gpio_get_value(gpio_pin_num);
		printk( KERN_INFO "level:%d\n",level );
		st=kmalloc(sizeof(char),GFP_KERNEL);
		sprintf(st,"%d",level);
		if(copy_to_user(buffer,st,sizeof(char)))
		{
			kfree(st);
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		kfree(st);
		return(size);
	}	
	
	//FIH, Henry Juang, 2009/08/10++
	else if(ioctl_operation==IOCTL_PID_GET)
	{
		pbuf=(char*)bufferPID_tsflag;
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_PID_GET 1\n");

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*40))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '%s'", bufferPID_tsflag);
		
		return 1;
	}
	//FIH, godfrey, 20100505++
	else if(ioctl_operation==IOCTL_BDADDR_GET)
	{
		proc_comm_ftm_bdaddr_read((char *)bufferPID_tsflag);
		
		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*6))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		return 1;
	}
	//FIH, godfrey, 20100505--
	else if (ioctl_operation == IOCTL_HV_GET)
	{
		fih_ftm_get_hw_id((char *)bufferPID_tsflag);
		
		if (copy_to_user(buffer, bufferPID_tsflag,sizeof(char)*3))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "[ftmgpio.c] %d %d %d", bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2]);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_IMEI_GET)
	{
		pbuf=(char*)bufferPID_tsflag;

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*15))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "[ftmgpio.c] IMEI = '%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d'\n", bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2], bufferPID_tsflag[3], bufferPID_tsflag[4], bufferPID_tsflag[5], bufferPID_tsflag[6], bufferPID_tsflag[7], bufferPID_tsflag[8], bufferPID_tsflag[9], bufferPID_tsflag[10], bufferPID_tsflag[11], bufferPID_tsflag[12], bufferPID_tsflag[13], bufferPID_tsflag[14]);
		
		return 1;
	}
	//FIH, Henry Juang, 2009/08/10--
	//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+[
	else if(ioctl_operation==IOCTL_WLANADDR_GET)
	{
		proc_comm_ftm_wlanaddr_read((char *)bufferPID_tsflag);
		
		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*6))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		return 1;
	}
	//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+]
/* FIHTDC, CHHsieh, MEID Get/Set { */
	else if(ioctl_operation==IOCTL_MEID_GET)
	{
		pbuf=(char*)bufferPID_tsflag;

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*14))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "[ftmgpio.c] MEID = '0x%x%x%x%x%x%x%x%x%x%x%x%x%x%x'\n",
				bufferPID_tsflag[ 0], bufferPID_tsflag[ 1], bufferPID_tsflag[ 2], bufferPID_tsflag[ 3],
				bufferPID_tsflag[ 4], bufferPID_tsflag[ 5], bufferPID_tsflag[ 6], bufferPID_tsflag[ 7],
				bufferPID_tsflag[ 8], bufferPID_tsflag[ 9], bufferPID_tsflag[10], bufferPID_tsflag[11],
				bufferPID_tsflag[12], bufferPID_tsflag[13]);
		
		return 1;
	}
/* } FIHTDC, CHHsieh, MEID Get/Set */
/* FIHTDC, CHHsieh, FD1 NV programming in factory { */
	else if(ioctl_operation==IOCTL_ESN_GET)
	{
		pbuf=(char*)bufferPID_tsflag;

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*8))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "[ftmgpio.c] ESN = '0x%x%x%x%x%x%x%x%x'\n",
				bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2], bufferPID_tsflag[3],
				bufferPID_tsflag[4], bufferPID_tsflag[5], bufferPID_tsflag[6], bufferPID_tsflag[7]);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_AKEY1_GET)
	{
		memset(bufferPID_tsflag,0,16*sizeof(char));
		proc_comm_ftm_akey1_read((char *)bufferPID_tsflag);
		
		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*16))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '0x%02x%02x%02x%02x%02x%02x'\n",
				bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2], bufferPID_tsflag[3],
				bufferPID_tsflag[4], bufferPID_tsflag[5]);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_SPC_GET)
	{
		memset(bufferPID_tsflag,0,6*sizeof(char));
		proc_comm_ftm_spc_read((char *)bufferPID_tsflag);
		
		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*6))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '0x%02x%02x%02x%02x%02x%02x'\n",
				bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2], bufferPID_tsflag[3],
				bufferPID_tsflag[4], bufferPID_tsflag[5]);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_CUSTOMERPID_GET)
	{
		pbuf=(char*)bufferPID_tsflag;

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*32))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '%s'", bufferPID_tsflag);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_CUSTOMERPID2_GET)
	{
		pbuf=(char*)bufferPID_tsflag;

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*32))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '%s'", bufferPID_tsflag);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_CUSTOMERSWID_GET)
	{
		pbuf=(char*)bufferPID_tsflag;

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*32))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '%s'", bufferPID_tsflag);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_DOM_GET)
	{
		pbuf=(char*)bufferPID_tsflag;

		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*14))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '%s'", bufferPID_tsflag);
		
		return 1;
	}
/* } FIHTDC, CHHsieh, FD1 NV programming in factory */
/* FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get { */
	else if(ioctl_operation==IOCTL_CATIME_GET)
	{
		memset(bufferPID_tsflag,0,14*sizeof(char));
		proc_comm_ftm_ca_time_read((char *)bufferPID_tsflag);
		
		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*7))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '0x%02x%02x%02x%02x%02x%02x%02x'\n",
				bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2], bufferPID_tsflag[3],
				bufferPID_tsflag[4], bufferPID_tsflag[5], bufferPID_tsflag[6]);
		
		return 1;
	}
	else if(ioctl_operation==IOCTL_WCATIME_GET)
	{
		memset(bufferPID_tsflag,0,14*sizeof(char));
		proc_comm_ftm_wca_time_read((char *)bufferPID_tsflag);
		
		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*7))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '0x%02x%02x%02x%02x%02x%02x%02x'\n",
				bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2], bufferPID_tsflag[3],
				bufferPID_tsflag[4], bufferPID_tsflag[5], bufferPID_tsflag[6]);
		
		return 1;
	}
/* } FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get */
/* FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get { */
	else if(ioctl_operation==IOCTL_FACTORYINFO_GET)
	{
		memset(bufferPID_tsflag,0,124*sizeof(char));
		proc_comm_ftm_factory_info_read((char *)bufferPID_tsflag);
		
		if(copy_to_user(buffer,bufferPID_tsflag,sizeof(char)*124))
		{
			printk( KERN_INFO "copy_to_user() fail!\n" );
			return -EFAULT;
		}
		
		printk(KERN_INFO "bufferPID_tsflag = '0x%02x%02x%02x%02x%02x%02x%02x'\n",
				bufferPID_tsflag[0], bufferPID_tsflag[1], bufferPID_tsflag[2], bufferPID_tsflag[3],
				bufferPID_tsflag[4], bufferPID_tsflag[5], bufferPID_tsflag[6]);
		
		return 1;
	}
/* } FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get */
	else
		printk( KERN_INFO "Undefined FTM GPIO driver IOCTL command\n" );
	
	return 0;
}

static int ftmgpio_dev_write( struct file * filp, const char __user * buffer, size_t length, loff_t * offset )
{
	int rc;
	int micbais_en;
	printk( KERN_INFO "FTM GPIO driver write, length=%d, data=%d\n",length , buffer[0] );
	if(ioctl_operation==IOCTL_GPIO_INIT)
	{
		if(buffer[0]==GPIO_IN)
//			rc = gpio_tlmm_config(GPIO_CFG(gpio_pin_num, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
/* FIHTDC, Div2-SW2-BSP CHHsieh, { */
			rc = gpio_tlmm_config(GPIO_CFG(gpio_pin_num, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
/* } FIHTDC, Div2-SW2-BSP CHHsieh, */
		else
//			rc = gpio_tlmm_config(GPIO_CFG(gpio_pin_num, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
/* FIHTDC, Div2-SW2-BSP CHHsieh, { */
			rc = gpio_tlmm_config(GPIO_CFG(gpio_pin_num, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
/* } FIHTDC, Div2-SW2-BSP CHHsieh, */
			
		if (rc) 
		{
			printk(KERN_ERR
					"%s--%d: gpio_tlmm_config=%d\n",
					__func__,__LINE__, rc);
			return -EIO;
		}
	
	}
	else if(ioctl_operation==IOCTL_GPIO_SET)	
	{
		gpio_direction_output(gpio_pin_num, buffer[0]);
	}
	else if(ioctl_operation==IOCTL_POWER_OFF)
	{
		msm_proc_comm(PCOM_POWER_DOWN, 0, 0);
		for (;;)
		;
	}
//FIH, Henry Juang, 2009/08/10++
/* [FXX_CR], Add for FTM PID and testflag*/
	else if(ioctl_operation==IOCTL_PID_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*40))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
//		proc_comm_write_PID_testFlag( SMEM_PROC_COMM_OEM_PRODUCT_ID_WRITE,(unsigned*)bufferPID_tsflag);
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_PID_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_product_id_write((unsigned*)bufferPID_tsflag);
		return 1;
	
	}
	else if(ioctl_operation==IOCTL_IMEI_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*15))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
//		proc_comm_write_PID_testFlag( SMEM_PROC_COMM_OEM_TEST_FLAG_WRITE,(unsigned*)bufferPID_tsflag);
		proc_comm_ftm_imei_write((unsigned*)bufferPID_tsflag);
		return 1;
	}
	//FIH, godfrey, 20100505++
	else if(ioctl_operation==IOCTL_BDADDR_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,6))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		proc_comm_ftm_bdaddr_write((char*)bufferPID_tsflag);
		return 1;
	}
	//FIH, godfrey, 20100505--
//FIH, Henry Juang, 2009/08/10--
	/* FIH, Michael Kao, 2009/08/28{ */
	/* [FXX_CR], Add to turn on Mic Bias */
	else if(ioctl_operation==IOCTL_MICBIAS_SET)
	{
		micbais_en=buffer[0];
//		proc_comm_micbias_set(&micbais_en);
	}
	/* FIH, Michael Kao, 2009/08/28{ */
	else if(ioctl_operation==IOCTL_WRITE_NV_8029)
	{
//		if(write_NV_single(gpio_pin_num,buffer[0])!=0)
//		{
//			printk( KERN_INFO "IOCTL_WRITE_NV_8029 fail! nv=%d,val=%d\n",gpio_pin_num,buffer[0]);
//			return -EFAULT;
//		}
	}
	//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+[
	else if(ioctl_operation==IOCTL_WLANADDR_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,6))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		proc_comm_ftm_wlanaddr_write((char*)bufferPID_tsflag);
		return 1;
	}
	//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+]
/* FIHTDC, CHHsieh, MEID Get/Set { */
	else if(ioctl_operation==IOCTL_MEID_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*7))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		if( proc_comm_ftm_meid_write((char*)bufferPID_tsflag) != 0 ){
			printk( KERN_INFO "proc_comm_ftm_meid_write() fail!\n" );
			return -EFAULT;
		}
		return 1;
	}
/* } FIHTDC, CHHsieh, MEID Get/Set */
/* FIHTDC, CHHsieh, FD1 NV programming in factory { */
	else if(ioctl_operation==IOCTL_AKEY1_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*16))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		if( proc_comm_ftm_akey1_write((unsigned*)bufferPID_tsflag) != 0 ){
			printk( KERN_INFO "proc_comm_ftm_akey1_write() fail!\n" );
			return -EFAULT;
		}
		return 1;
	}
	else if(ioctl_operation==IOCTL_AKEY2_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*16))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		if( proc_comm_ftm_akey2_write((unsigned*)bufferPID_tsflag) != 0 ){
			printk( KERN_INFO "proc_comm_ftm_akey2_write() fail!\n" );
			return -EFAULT;
		}
		return 1;
	}
	else if(ioctl_operation==IOCTL_SPC_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*8))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		if( proc_comm_ftm_spc_write((unsigned*)bufferPID_tsflag) != 0 ){
			printk( KERN_INFO "proc_comm_ftm_spc_write() fail!\n" );
			return -EFAULT;
		}
		return 1;
	}
	else if(ioctl_operation==IOCTL_CUSTOMERPID_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*32))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_CUSTOMERPID_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_customer_pid_write((unsigned*)bufferPID_tsflag);
		return 1;
	}
	else if(ioctl_operation==IOCTL_CUSTOMERPID2_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*32))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_CUSTOMERPID2_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_customer_pid2_write((unsigned*)bufferPID_tsflag);
		return 1;
	}
	else if(ioctl_operation==IOCTL_CUSTOMERSWID_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*32))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_CUSTOMERSWID_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_customer_swid_write((unsigned*)bufferPID_tsflag);
		return 1;
	}
	else if(ioctl_operation==IOCTL_DOM_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*14))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_DOM_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_dom_write((unsigned*)bufferPID_tsflag);
		return 1;
	}
/* } FIHTDC, CHHsieh, FD1 NV programming in factory */
/* FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get { */
	else if(ioctl_operation==IOCTL_CATIME_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*7))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_CATIME_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_ca_time_write((char*)bufferPID_tsflag);
		return 1;
	
	}
	else if(ioctl_operation==IOCTL_WCATIME_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*7))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_WCATIME_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_wca_time_write((char*)bufferPID_tsflag);
		return 1;
	
	}
/* } FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get */
/* FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get { */
	else if(ioctl_operation==IOCTL_FACTORYINFO_SET)
	{
		if(copy_from_user(bufferPID_tsflag,buffer,sizeof(char)*124))
		{
			printk( KERN_INFO "copy_from_user() fail!\n" );
			return -EFAULT;
		}
		printk(KERN_INFO "===========================> ftmgpio.c IOCTL_FACTORYINFO_SET '%s'\n", bufferPID_tsflag);
		proc_comm_ftm_factory_info_write((char*)bufferPID_tsflag);
		return 1;
	
	}
/* } FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get */
	else
		printk( KERN_INFO "Undefined FTM GPIO driver IOCTL command\n" );
				
	return length;
}	

//#define CLK_UART_NS_REG (MSM_CLK_CTL_BASE + 0x000000E0)

static int ftmgpio_dev_ioctl( struct inode * inode, struct file * filp, unsigned int cmd, unsigned long arg )
{
	//[+++]FIH, ChiaYuan, 2007/07/10
	// [FXX_CR] Put the modem into LPM mode for FTM test
//	unsigned smem_response;
//	uint32_t oem_cmd;
//	unsigned oem_parameter=0;
	//[---]FIH, ChiaYuan, 2007/07/10
//FIH, Henry Juang, 20091116 ++
/*Add for modifing UART3 source clock*/
//	unsigned long value_UART_NS_REG=0x0;
//FIH, Henry Juang, 20091116 --
	
	printk( KERN_INFO "FTM GPIO driver ioctl, cmd = %d, arg = %ld\n", cmd , arg);
	
	switch(cmd)
	{
		case IOCTL_GPIO_INIT:		
		case IOCTL_GPIO_SET:		
		case IOCTL_GPIO_GET:	
		//FIH, MichaelKao, 2009/7/9++
		case IOCTL_HV_GET:
		//FIH, MichaelKao, 2009/7/9++
		case IOCTL_POWER_OFF:
		case IOCTL_MICBIAS_SET:
			//printk("[Ftmgoio.c][ftmgpio_dev_ioctl]power_off_test++++");
		//FIH, godfrey, 20100505++
		case IOCTL_BDADDR_GET:
		case IOCTL_BDADDR_SET:
		//FIH, godfrey, 20100505--
			ioctl_operation = cmd;
			gpio_pin_num = arg;			
			break;

//FIH, Henry Juang, 2009/08/10{++
		case IOCTL_PID_GET:
			memset(bufferPID_tsflag,0,40*sizeof(char));
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_PID_GET 2\n");
//			proc_comm_read_PID_testFlag( SMEM_PROC_COMM_OEM_PRODUCT_ID_READ,(unsigned*)bufferPID_tsflag);
			proc_comm_ftm_product_id_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;

		case IOCTL_PID_SET:
			ioctl_operation = cmd;
			gpio_pin_num = arg;			
			break;
		case IOCTL_IMEI_GET:
			memset(bufferPID_tsflag,0,15*sizeof(char));
//			proc_comm_read_PID_testFlag( SMEM_PROC_COMM_OEM_TEST_FLAG_READ,(unsigned*)bufferPID_tsflag);
			proc_comm_ftm_imei_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;
		case IOCTL_IMEI_SET:
			ioctl_operation = cmd;
			gpio_pin_num = arg;			
			break;
//}FIH, Henry Juang, 2009/08/10--
		
		//[+++]FIH, ChiaYuan, 2007/07/10
		// [FXX_CR] Put the modem into LPM mode for FTM test	
		case IOCTL_MODEM_LPM:
//			oem_cmd = SMEM_PROC_COMM_OEM_EBOOT_SLEEP_REQ;
//			msm_proc_comm_oem(PCOM_CUSTOMER_CMD1, &oem_cmd, &smem_response, &oem_parameter );
			break;
		//[---]FIH, ChiaYuan, 2007/07/10							
//FIH, Henry Juang, 20091116 ++
		case IOCTL_UART_BAUDRATE_460800:
			if (arg != 1)
				ftm_init_clock(0);
			else
				ftm_init_clock(1);
/*
				//add for testing.	
				value_UART_NS_REG=0x7ff8fff & readl(CLK_UART_NS_REG);//set UART source clock to TCXO.
				printk("1******value_UART_NS_REG=0x%x%x,addr=0x%x\n",(unsigned int)(value_UART_NS_REG>>8)&0xff,(unsigned int)(value_UART_NS_REG & 0xff),(unsigned int)CLK_UART_NS_REG);	
				if(arg!=1){
					value_UART_NS_REG|=0x0001000; //roll back UART source clock to TCXO/4.
				}
				writel(value_UART_NS_REG,CLK_UART_NS_REG);	
				value_UART_NS_REG=readl(CLK_UART_NS_REG);	
				printk("2******value_UART_NS_REG=0x%x%x,addr=0x%x\n",(unsigned int)(value_UART_NS_REG>>8)&0xff,(unsigned int)(value_UART_NS_REG & 0xff),(unsigned int)CLK_UART_NS_REG);	
*/
			break;
//FIH, Henry Juang, 20091116 --
//FIH, Henry Juang, 20100426 ++
		case IOCTL_WRITE_NV_8029:
			printk("select  nv_%d to write. \n",(int)arg);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
	
			break;
//FIH, Henry Juang, 20100426 --
		//MM-SL-AudioFTM-00*{
		/* Div1-FW3-BSP-AUDIO */
		case IOCTL_LOOPBACK_EN:
			printk("enable loopback\n");
			debugfs_adie_loopback(1, arg);		
			break;

		case IOCTL_LOOPBACK_DIS:
			printk("disable loopback\n");
			debugfs_adie_loopback(0, arg);		
			break;

        case IOCTL_MICBIAS_CTL:
			pmic_hsed_enable(PM_HSED_CONTROLLER_1, arg);
			break;

        case IOCTL_FM_EN:
            printk("enable FM path\n");
            fm_set_rx_path(1, arg);      
        	break;
        case IOCTL_FM_DIS:
            printk("disable FM path\n");
            fm_set_rx_path(0, arg);      
            break;
        /* Div1-FW3-BSP-AUDIO */
		//MM-SL-AudioFTM-00*}
		
		//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+[
		case IOCTL_WLANADDR_GET:
		case IOCTL_WLANADDR_SET:
			ioctl_operation = cmd;
			gpio_pin_num = arg;
			break;
		//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+]
/* FIHTDC, Div2-SW2-BSP CHHsieh, NVBackup { */
		case IOCTL_BACKUP_UNIQUE_NV:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_BACKUP_UNIQUE_NV 2\n");
			if( proc_comm_ftm_backup_unique_nv() != 0 ){
				return -1;
			}
			break;
		case IOCTL_HW_RESET:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_HW_RESET 2\n");
			proc_comm_ftm_hw_reset();
			break;
		case IOCTL_BACKUP_NV_FLAG:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_BACKUP_NV_FLAG 2\n");
			if( proc_comm_ftm_nv_flag() != 0){
				return -1;
			}
			break;
		case IOCTL_HW_RESET_TO_FTM:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_HW_RESET_TO_FTM\n");
			proc_comm_hw_reset_to_ftm();
			break;
/* } FIHTDC, Div2-SW2-BSP CHHsieh, NVBackup */
		case IOCTL_FUSE_BOOT_SET:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_FUSE_BOOT_SET 2\n");
			if( proc_comm_fuse_boot_set() != 0){
				return -1;
			}
			break;
		case IOCTL_FUSE_BOOT_GET:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_FUSE_BOOT_GET 2\n");
			if( proc_comm_fuse_boot_get() != 0){
				return -1;
			}
			break;
/* FIHTDC, CHHsieh, PMIC Unlock { */
		case IOCTL_PMIC_UNLOCK:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_PMIC_UNLOCK 2\n");
			if( proc_comm_ftm_pmic_unlock() != 0){
				return -1;
			}
			break;
/* } FIHTDC, CHHsieh, PMIC Unlock */
/* FIHTDC, Div2-SW2-BSP CHHsieh, MEID Get/Set { */
		case IOCTL_MEID_GET:
			memset(bufferPID_tsflag,0,14*sizeof(char));
			proc_comm_ftm_meid_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;
		case IOCTL_MEID_SET:
			ioctl_operation = cmd;
			gpio_pin_num = arg;			
			break;
/* } FIHTDC, Div2-SW2-BSP CHHsieh, MEID Get/Set */
/* FIHTDC, CHHsieh, FD1 NV programming in factory { */
		case IOCTL_ESN_GET:
			memset(bufferPID_tsflag,0,8*sizeof(char));
			proc_comm_ftm_esn_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;
		case IOCTL_CUSTOMERPID_GET:
			memset(bufferPID_tsflag,0,32*sizeof(char));
			proc_comm_ftm_customer_pid_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;
		case IOCTL_CUSTOMERPID2_GET:
			memset(bufferPID_tsflag,0,32*sizeof(char));
			proc_comm_ftm_customer_pid2_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;
		case IOCTL_CUSTOMERSWID_GET:
			memset(bufferPID_tsflag,0,32*sizeof(char));
			proc_comm_ftm_customer_swid_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;
		case IOCTL_DOM_GET:
			memset(bufferPID_tsflag,0,14*sizeof(char));
			proc_comm_ftm_dom_read((unsigned*)bufferPID_tsflag);
			ioctl_operation = cmd;
			gpio_pin_num = arg;	
			break;
		case IOCTL_AKEY1_SET:
		case IOCTL_AKEY2_SET:
		case IOCTL_AKEY1_GET:
		case IOCTL_SPC_GET:
		case IOCTL_SPC_SET:
		case IOCTL_CUSTOMERPID_SET:
		case IOCTL_CUSTOMERPID2_SET:
		case IOCTL_CUSTOMERSWID_SET:
		case IOCTL_DOM_SET:
			ioctl_operation = cmd;
			gpio_pin_num = arg;			
			break;
/* } FIHTDC, CHHsieh, FD1 NV programming in factory */
/* FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get { */
		case IOCTL_CATIME_GET:
		case IOCTL_WCATIME_GET:
		case IOCTL_CATIME_SET:
		case IOCTL_WCATIME_SET:
			ioctl_operation = cmd;
			gpio_pin_num = arg;			
			break;
/* } FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get */
/* FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get { */
		case IOCTL_FACTORYINFO_GET:
		case IOCTL_FACTORYINFO_SET:
			ioctl_operation = cmd;
			gpio_pin_num = arg;			
			break;
/* } FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get */
		case IOCTL_CHANGE_MODEM_LPM:
			printk(KERN_INFO "===========================> ftmgpio.c IOCTL_CHANGE_MODEM_LPM\n");
			if( proc_comm_ftm_change_modem_lpm() != 1){
				return -1;
			}
			break;
		default:
			printk( KERN_INFO "Undefined FTM GPIO driver IOCTL command\n" );
			return -1;
	}


	return 0;
}

 static int __init ftmgpio_init(void)
{
        int ret;

        printk( KERN_INFO "FTM GPIO Driver init\n" );
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
	ret = misc_register(&ftmgpio_dev);
	if (ret){
		printk(KERN_WARNING "FTM GPIO Unable to register misc device.\n");
		return ret;
	}

        return ret;
        
}

static void __exit ftmgpio_exit(void)                         
{                                                                              
        printk( KERN_INFO "FTM GPIO Driver exit\n" );          
        misc_deregister(&ftmgpio_dev);
}                                                        
                                                         
module_init(ftmgpio_init);                            
module_exit(ftmgpio_exit);                                
                                                         
MODULE_AUTHOR( "Clement Hsu <clementhsu@tp.cmcs.com.tw>" );
MODULE_DESCRIPTION( "FTM GPIO driver" );                
MODULE_LICENSE( "GPL" );                                 



















