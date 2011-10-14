/*
 *     gps_sync.c - For user space to sync EXT_GPS_LNA_EN (81) and GPS_FM_LNA_2V8_EN (79).
 *
 *     Copyright (C) 2009 Owen Huang <owenhuang@fihspec.com>
 *     Copyright (C) 2009 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
//#include <asm/gpio.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
//#include "nvodm_services.h"
#include <linux/mutex.h>

/* FIHTDC, Div2-SW2-BSP Godfrey, FB0.B-396 */
#include <media/tavarua.h>

#define DEBUG 1

#if DEBUG
#define DBG_MSG(msg, ...) printk("[gps_sync_DBG] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#else
#define DBG_MSG(msg, ...)
#endif

#define INF_MSG(msg, ...) printk("[gps_sync_INF] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define ERR_MSG(msg, ...) printk("[gps_sync_ERR] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)

struct gps_sync
{
	int reference_counter; //used gps LNA/LDA counter
	int status; //right now gps LNA/LDA status
}gps_sync;

//status bit
#define HIGH 1
#define LOW 0


#define GPS_FM_LNA_2V8_EN	  79
#define EXT_GPS_LNA_EN			81

//IOCTL Command
#define GPS_ON_CMD  0x00
#define GPS_OFF_CMD 0x01

static struct gps_sync LNA_LDO_sync;

static int gps_sync_dev_open( struct inode * inodep, struct file * filep )
{
	DBG_MSG("LNA_LDO_open");

	LNA_LDO_sync.reference_counter++;
	return 0;
}

static int gps_sync_dev_release( struct inode * inodep, struct file * filep )
{
	DBG_MSG("LNA_LDO_release");
	LNA_LDO_sync.reference_counter--;
//	if (LNA_LDO_sync.reference_counter == 0)
//		LNA_LDO_sync.fd = 0;
    
	return 0;
}

static int gps_sync_dev_ioctl( struct inode * inodep, struct file * filep, unsigned int cmd, unsigned long arg )
{
	DBG_MSG("ioctl command = %d", cmd);
	switch(cmd)
	{
		case GPS_ON_CMD:
			//Div2-SW6-LNA_REFERECCE_COUNTER++{
			if (LNA_LDO_sync.reference_counter == 1)
			{
			//Div2-SW6-LNA_REFERECCE_COUNTER++}
			LNA_LDO_sync.status &= HIGH;
			/* FIHTDC, Div2-SW2-BSP Godfrey, FB0.B-396 */
			enableGPS_FM_LNA(true);
			//gpio_set_value(GPS_FM_LNA_2V8_EN, 1);
			gpio_set_value(EXT_GPS_LNA_EN, 1);
			//Div2-SW6-LNA_REFERECCE_COUNTER++{
			DBG_MSG("reference_counter = %d", LNA_LDO_sync.reference_counter);
		  }
		  else
		  {
		  	DBG_MSG("reference_counter = %d", LNA_LDO_sync.reference_counter);
		  }
		  //Div2-SW6-LNA_REFERECCE_COUNTER++}
			break;
		case GPS_OFF_CMD:
			//Div2-SW6-LNA_REFERECCE_COUNTER++{
			if (LNA_LDO_sync.reference_counter == 0)
			{
			//Div2-SW6-LNA_REFERECCE_COUNTER++}			
			LNA_LDO_sync.status &= LOW;
			/* FIHTDC, Div2-SW2-BSP Godfrey, FB0.B-396 */
			enableGPS_FM_LNA(false);
			//gpio_set_value(GPS_FM_LNA_2V8_EN, 0);
			gpio_set_value(EXT_GPS_LNA_EN, 0);
			//Div2-SW6-LNA_REFERECCE_COUNTER++{
			DBG_MSG("reference_counter = %d", LNA_LDO_sync.reference_counter);
		  }
		  else
		  {
		  	DBG_MSG("reference_counter = %d", LNA_LDO_sync.reference_counter);
		  }
		  //Div2-SW6-LNA_REFERECCE_COUNTER++}			
			break;
		default:
			ERR_MSG("ioctl error!");
			break;
	}	
	return 0;
}

static struct file_operations gps_sync_dev_fops = {
	.open = gps_sync_dev_open,
	.ioctl = gps_sync_dev_ioctl,
	.release = gps_sync_dev_release,
};

static struct miscdevice gps_sync_cdev = {
	MISC_DYNAMIC_MINOR,
	"gps_sync",
	&gps_sync_dev_fops
};

static int __init gps_sync_init(void)
{
//	INF_MSG("start");
    int ret = 0;
	INF_MSG("init");
	//initialize
	LNA_LDO_sync.status = 0;
	LNA_LDO_sync.reference_counter= 0;
    gpio_tlmm_config(GPIO_CFG(GPS_FM_LNA_2V8_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
    ret = gpio_request(GPS_FM_LNA_2V8_EN,"GPS_LDO");
    if(ret)
    	{
    		INF_MSG("GPS_LDO failed!");
    	}
    gpio_tlmm_config(GPIO_CFG(EXT_GPS_LNA_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);	
    ret = gpio_request(EXT_GPS_LNA_EN,"GPS_LNA");
    if(ret)
    {
    	  INF_MSG("GPS_LNA failed!");
    }
//	mutex_init(&AT_port_sync.read_sync);
//	mutex_init(&AT_port_sync.write_sync);

	//register misc device
	return misc_register(&gps_sync_cdev);
}

static void __exit gps_sync_exit(void)
{
//	INF_MSG("start");
	INF_MSG("exit");
	//de-initialize
	misc_deregister(&gps_sync_cdev);
//	mutex_destroy(&AT_port_sync.read_sync);
//	mutex_destroy(&AT_port_sync.write_sync);
}

module_init(gps_sync_init);
module_exit(gps_sync_exit);

MODULE_AUTHOR( "Owen Huang <owenhuang@fihspec.com>" );
MODULE_DESCRIPTION( "For sync LNA and LDO for GPS" );
MODULE_LICENSE( "GPL" );
