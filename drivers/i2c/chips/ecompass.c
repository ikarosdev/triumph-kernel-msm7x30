/* fihsensor.c - YAMAHA MS-3C compass driver
 * 
 * Copyright (C) 2007-2008 FIH Corporation.
 * Author: Tiger JT Lee
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/akm8976.h>
#include <linux/module.h>
#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
#include <linux/earlysuspend.h> 
#include <mach/vreg.h>
#include <mach/gpio.h>
#include "../../../arch/arm/mach-msm/proc_comm.h"

#define DEBUG 0

#ifdef CONFIG_PM
#undef CONFIG_PM
#endif

#if (0)
#define my_debug(flag, fmt, ...) \
	printk(flag pr_fmt(fmt), ##__VA_ARGS__)
#else
#define my_debug(flag, fmt, ...)
#endif

#define DRIVER_NAME	"[ecompass]"
#define DBG(f, s, x...) \
	my_debug(f, DRIVER_NAME "%s: " s, __func__, ## x)

static struct i2c_client *yas529_i2c_client = NULL;
static struct i2c_client *bma150_i2c_client = NULL;

static struct work_struct bma150_irq_work;

static int profile = 1;
//+++ FIH; Louis; 2010/11/9
static int iNVItem[6];
static int iNVWrite = 0;
module_param(profile, int, S_IRUGO);

struct fih_sensor_context {
	struct i2c_client *activeSlave;
/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
	unsigned char bIsIdle;
#endif
/* } FIH; Tiger; 2009/6/10 */
}; 
typedef struct fih_sensor_context fih_sensor_context_s;

#ifndef BMA150_I2C_ADDR
#define BMA150_I2C_ADDR			0x38
#endif
#ifndef YAS529_I2C_SLAVE_ADDR
#define YAS529_I2C_SLAVE_ADDR		0x2e
#endif
#define IOCTL_SET_ACTIVE_SLAVE		0x0706
#define IOCTL_GET_SENSOR_CONTROL_INFO	0x0707
#define IOCTL_SET_CHARGER_STATE		0x0708

static fih_sensor_context_s fihsensor_ctx = {
	.activeSlave = NULL,
/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
	.bIsIdle = false,
#endif
/* } FIH; Tiger; 2009/6/10 */
};



/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
#define BMA150_MODE_NORMAL	0
#define BMA150_MODE_SLEEP	2
#endif

#define WAKE_UP_REG		0x15
#define WAKE_UP_POS		0
#define WAKE_UP_LEN		1
#define WAKE_UP_MSK		0x01

#define SLEEP_REG		0x0a
#define SLEEP_POS		0
#define SLEEP_LEN		1
#define SLEEP_MSK		0x01

#define CHR_EN 			33

#define DISABLE			0
#define ENABLE  		1
#define ENABLE_INTERRUPT  	DISABLE 
#define BMA150_INT_PIN    	40 
#define YAS529_RST_PIN    	82  
#define CTRL14			0x14

/* IOCTL */
#define RANGE			0
#define BANDWIDTH		1
#define NVREAD			3
#define NVWRITE1     	4
#define NVWRITE2		5


#define BMA150_SET_BITSLICE(regvar, bitname, val) \
			(regvar & ~bitname##_MSK) | ((val<<bitname##_POS)&bitname##_MSK)

static int compass_read_reg(struct i2c_client *clnt, 
								unsigned char reg, unsigned char *data, unsigned char count);

static int compass_write_reg(struct i2c_client *clnt, 
								unsigned char reg, unsigned char *data, unsigned char count);
#define _OWEN_
#ifdef CONFIG_HAS_EARLYSUSPEND	
_OWEN_ struct early_suspend bma150_early_suspend;
_OWEN_ struct early_suspend yas529_early_suspend;

/*_OWEN_ static void compass_sleep_mode(void)
{
	//int ret;
	//char data;
	
	printk(KERN_INFO "[Ecompass]%s \n", __func__);

	//YAS529 enter sleep mode
	//gpio_direction_output(YAS529_RST_PIN, 0);

	//BMA150 enter sleep mode
	
	//ret = compass_read_reg(smb380_i2c_client, SLEEP_REG, &data, 1);
	//if (ret)
	//{
	//	printk(KERN_ERR "[Ecompass]BMA150 read sleep_reg failed!");
	//	return;
	//}

	//data = data | 0x01; //set bit0 to "1" (enter sleep mode)
	//ret = compass_write_reg(smb380_i2c_client, SLEEP_REG, &data, 1);
	//if (ret)
	//{
	//	printk(KERN_ERR "[Ecompass]BMA150 write sleep_reg failed!");
	//	return;
	//}
	
	
}*/
/*_OWEN_ static void compass_wakeup_mode(void)
{
	int ret;
	char data;
	
	printk(KERN_INFO "[Ecompass]%s \n", __func__);

	//YAS529 enter sleep mode
	//gpio_direction_output(YAS529_RST_PIN, 1);

	ret = compass_read_reg(smb380_i2c_client, SLEEP_REG, &data, 1);
	if (ret)
	{
		printk(KERN_ERR "[Ecompass]BMA150 read sleep_reg failed!");
		return;
	}

	data = data & 0xFE; //set bit0 to "1" (enter sleep mode)
	ret = compass_write_reg(smb380_i2c_client, SLEEP_REG, &data, 1);
	if (ret)
	{
		printk(KERN_ERR "[Ecompass]BMA150 write sleep_reg failed!");
		return;
	}
}*/

_OWEN_ static void bma150_early_suspend_func(struct early_suspend * h)
{
	//Read-modify-write 0x0A bit 0 -> "1", let chip in sleep mode, page 21
	char data;
	int ret;
	
	DBG(KERN_INFO, "\n");
	
	ret = compass_read_reg(bma150_i2c_client, SLEEP_REG, &data, 1);
	if (ret)
	{
		printk(KERN_ERR "BMA150 read sleep_reg failed!\n");
		return;
	}

	data = data | 0x01; //set bit0 to "1" (enter sleep mode)
	ret = compass_write_reg(bma150_i2c_client, SLEEP_REG, &data, 1);
	if (ret)
	{
		printk(KERN_ERR "BMA150 write sleep_reg failed!\n");
		return;
	}
}

_OWEN_ static void bma150_late_resume_func(struct early_suspend *h)
{
	//Read-modify-write 0x0A bit 0 -> "0", let chip in normal mode, page 21
	char data;
	int ret;
	
	DBG(KERN_INFO, "\n");
	
	ret = compass_read_reg(bma150_i2c_client, SLEEP_REG, &data, 1);
	if (ret)
	{
		printk(KERN_ERR "BMA150 read sleep_reg failed!\n");
		return;
	}

	data = data & 0xFE; //set bit0 to "1" (enter sleep mode)
	ret = compass_write_reg(bma150_i2c_client, SLEEP_REG, &data, 1);
	if (ret)
	{
		printk(KERN_ERR "BMA150 write sleep_reg failed!");
		return;
	}
}
/*_OWEN_ static void yas529_early_suspend_func(struct early_suspend * h)
{
	//TODO
	printk(KERN_INFO "[Ecompass]YAS529 enter early suspend mode\n");
	//gpio_direction_output(YAS529_RST_PIN, 0);//Div6D1-OH-Ecompass-Porting-00+
}

_OWEN_ static void yas529_late_resume_func(struct early_suspend *h)
{
	//TODO
	printk(KERN_INFO "[Ecompass]YAS529 enter late resume mode\n");
	//gpio_direction_output(YAS529_RST_PIN, 1);//Div6D1-OH-Ecompass-Porting-00+
}*/
#endif

static int compass_read_reg(struct i2c_client *clnt, unsigned char reg, unsigned char *data, unsigned char count)
{
	unsigned char tmp[10];
	
	if(10 < count)
		return -1;
	
	tmp[0] = reg;
	if(1 != i2c_master_send(clnt, tmp, 1))
	{
		printk(KERN_ERR "set REGISTER address error\n");
		return -EIO;
	}

	if(count != i2c_master_recv(clnt, tmp, count))
	{
		printk(KERN_ERR "read REGISTER content error\n");
		return -EIO;
	}

	strncpy(data, tmp, count);

	return 0;
}

static int compass_write_reg(struct i2c_client *clnt, unsigned char reg, unsigned char *data, unsigned char count)
{
	unsigned char tmp[2];

	while(count)
	{
		tmp[0] = reg++;
		tmp[1] = *(data++);

		if(2 != i2c_master_send(clnt, tmp, 2))
		{
			printk(KERN_ERR "error\n");
			return -EIO;
		}

		count--;
	}

	return 0;
}


#ifdef CONFIG_PM

static int enter_mode(unsigned char mode)
{
	unsigned char data1, data2;
	int ret;

	if(mode==BMA150_MODE_NORMAL || mode==BMA150_MODE_SLEEP) 
	{
		if((ret=compass_read_reg(bma150_i2c_client, WAKE_UP_REG, &data1, 1)))
			return ret;

		DBG(KERN_INFO, "WAKE_UP_REG (0x%x)\n", data1);

		data1 = BMA150_SET_BITSLICE(data1, WAKE_UP, mode);

		if((ret=compass_read_reg(bma150_i2c_client, SLEEP_REG, &data2, 1)))
			return ret;
	
		DBG(KERN_INFO, "SLEEP_REG (0x%x)\n", data2);		
	
		data2 = BMA150_SET_BITSLICE(data2, SLEEP, (mode>>1));

		if((ret=compass_write_reg(bma150_i2c_client, WAKE_UP_REG, &data1, 1)))
			return ret;
		if((ret=compass_write_reg(bma150_i2c_client, SLEEP_REG, &data2, 1)))
			return ret;
	}

	return 0;
}
#endif	
/* } FIH; Tiger; 2009/6/10 */

static struct vreg *verg_gp9_L12, *verg_gp7_L8;
static int compass_open(struct inode *inode, struct file *file)
{
	//int err;
	DBG(KERN_INFO, "\n");

	if(fihsensor_ctx.activeSlave != NULL)
	{
		DBG(KERN_ALERT, "warning: compass reopen\n");
		return 0;
	}

	//fihsensor_ctx.activeSlave = yas529_i2c_client;
	fihsensor_ctx.activeSlave = bma150_i2c_client;

/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
	if(fihsensor_ctx.bIsIdle == true)
	{
		if(enter_mode(BMA150_MODE_NORMAL)) {
			DBG(KERN_ERR, "enter_mode(BMA150_MODE_NORMAL) fail\n");
			return -EIO;
		}
		else {
			fihsensor_ctx.bIsIdle = false;
		}
	}
#endif
/* } FIH; Tiger; 2009/6/10 */

	return 0;
}

static int compass_release(struct inode *inode, struct file *file)
{
	DBG(KERN_INFO, "\n");

	if(fihsensor_ctx.activeSlave == NULL)
	{
		printk(KERN_ERR "warning: compass reclose\n");
	}

	fihsensor_ctx.activeSlave = NULL;

/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
	if(fihsensor_ctx.bIsIdle == false)
	{
		if(enter_mode(BMA150_MODE_SLEEP)) {
			DBG(KERN_ERR, "enter_mode(BMA150_MODE_SLEEP) fail\n");
		}
		else {
			fihsensor_ctx.bIsIdle = true;
		}
	}
#endif
/* } FIH; Tiger; 2009/6/10 */

	return 0;
}

static int compass_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -1;	
	u8 val = -1; 
	int Command = 0;
	void __user *argp = (void __user *)arg;	
	int value;
	int j;
	//static int prev_charger_state = -1; /* charger: low active */

	DBG(KERN_INFO, "cmd (0x%X)\n", cmd);

	switch (cmd) 
	{
		case IOCTL_SET_ACTIVE_SLAVE:

			DBG(KERN_INFO, "set active slave (0x%X)\n", (unsigned int)arg);

			switch (arg) 
			{
				case BMA150_I2C_ADDR:
					fihsensor_ctx.activeSlave = bma150_i2c_client;
					DBG(KERN_INFO, "active slave=0x%X (gsensor)\n", (unsigned int)fihsensor_ctx.activeSlave);
					break;

				case YAS529_I2C_SLAVE_ADDR:
					fihsensor_ctx.activeSlave = yas529_i2c_client;
					DBG(KERN_INFO, "active slave=0x%X (ecompass)\n", (unsigned int)fihsensor_ctx.activeSlave);
					break;
		
				default:
					break;
			}
			break;
		case RANGE:
			if(copy_from_user(&value, argp, sizeof(value)))
            	return ret;			
			value = value << 3;

			val = i2c_smbus_read_byte_data(bma150_i2c_client, CTRL14);

			Command = (val & 0xe7) | value;
            DBG(KERN_INFO, "RANGE addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", CTRL14, value, val, Command); 
			ret = i2c_smbus_write_byte_data(bma150_i2c_client, CTRL14, Command); 	
			break;
		case BANDWIDTH:
			if(copy_from_user(&value, argp, sizeof(value)))
            	return ret;			

			val = i2c_smbus_read_byte_data(bma150_i2c_client, CTRL14);

			Command = (val & 0xf8) | value;
            DBG(KERN_INFO, "BANDWIDTH addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", CTRL14, value, val, Command); 
			ret = i2c_smbus_write_byte_data(bma150_i2c_client, CTRL14, Command); 
			break;
		//+++ FIH; Louis; 2010/11/9
		case NVREAD:
		{
			if(copy_from_user(&value, argp, sizeof(value)))
            	value = (int)arg;
			if(value == 0)
			{
				proc_comm_compass_param_read(iNVItem);
				printk(KERN_ERR "[ecompass]NVRead:%d, %d, %d, %d, %d, %d\n", iNVItem[0], iNVItem[1], iNVItem[2], iNVItem[3], iNVItem[4], iNVItem[5]);
			}			
			else
			{
				printk(KERN_ERR "[ecompass]NVRead:%d, %d\n", iNVItem[value-1], value);		
				return iNVItem[value-1];
			}
			break;	
		}
		case NVWRITE1:
		{	
			if(copy_from_user(&value, argp, sizeof(value)))
            	value = (int)arg; 
			iNVWrite++;
			iNVItem[iNVWrite-1] = value;
			printk(KERN_ERR "[ecompass]NVWrite:%d, %d\n", iNVItem[iNVWrite-1], iNVWrite);
			break;	
		}	
		case NVWRITE2:
		{		
			if(copy_from_user(&value, argp, sizeof(value)))
            	value = (int)arg; 
			if(value == 0)
			{
				printk(KERN_ERR "[ecompass]NVWrite:%d, %d, %d, %d, %d, %d\n", iNVItem[0], iNVItem[1], iNVItem[2], iNVItem[3], iNVItem[4], iNVItem[5]);
				proc_comm_compass_param_write(iNVItem);	
				iNVWrite = 0;
			}
			else if(value == 1)
			{
				printk(KERN_ERR "[ecompass]NVWriteReset\n");	
				for(j=0; j<6; j++)
					iNVItem[j] = 0;	
			}			
			break;
		}
#if 0
		case IOCTL_GET_SENSOR_CONTROL_INFO:
		{
			int controlInfo = 0;
			//int voltage; 
			//printk(KERN_INFO "Compass: check_USB_type=%d, USB_Connect=%d\r\n", 
			//					check_USB_type, USB_Connect);

			if(bMagnetManualCalibration) {
				controlInfo |= MAGNET_MANUAL_CALIBRATION;
			}

			if(bMagnetStartCalibration) {
				controlInfo |= MAGNET_START_CALIBRATION;
				bMagnetStartCalibration = 0;
			}

			if(bMagnetCheckDistort) {
				controlInfo |= MAGNET_CHECK_DISTORTION;
			}

			if(check_USB_type == 1) { 
				/* USB charger */
				controlInfo |= CHARGER_INFO_500mA;
			}
			else if(check_USB_type == 2) {
				/* Wall charger */
				controlInfo |= CHARGER_INFO_1A;
			}

			return controlInfo;
		}
		break;
#endif

#if 0
		case IOCTL_SET_CHARGER_STATE:
			if(arg == 1) 
			{
				/* restore charging state */
				if(check_USB_type != 0) 
				{
					if(prev_charger_state != -1) 
					gpio_set_value(CHR_EN, prev_charger_state);		
				}
			}
			else if(arg == 0) 
			{
				/* disable charging state */
				if(check_USB_type != 0) 
				{
					prev_charger_state = gpio_get_value(CHR_EN);
					if(prev_charger_state == 0)
						gpio_set_value(CHR_EN, 1);	
				}
				else 
				{
					prev_charger_state = -1;
				}
			}
			break;
#endif	
		default:
			break;
	}

	return 0;
}

static ssize_t compass_read(struct file *fp, char __user *buf,
                            size_t count, loff_t *pos)
{
	int ret = 0;
	char tmp[64];

	DBG(KERN_INFO, "\n");

/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
	if(fihsensor_ctx.bIsIdle == true && fihsensor_ctx.activeSlave == bma150_i2c_client)
	{
		DBG(KERN_INFO, "compass_read() in idle mode\n");
		return -EIO;
	}
#endif
/* } FIH; Tiger; 2009/6/10 */	
		
	if(fihsensor_ctx.activeSlave == NULL) 
	{
		printk(KERN_ERR "device invalid\n");
		ret = -ENODEV;	
	}
	else 
	{
		ret = i2c_master_recv(fihsensor_ctx.activeSlave, tmp, count);
		if(copy_to_user(buf, tmp, ret)) 
		{
			ret = -EFAULT;
		}
#if DEBUG		
		DBG(KERN_ERR, "result(%d, %d, %d, 0x%02x)\n", 
				(fihsensor_ctx.activeSlave == yas529_i2c_client), 
				count, ret, fihsensor_ctx.activeSlave->addr);

		{
			int i;
			for(i=0; i< count; i++) 
			{
				DBG(KERN_INFO, "data [%d, 0x%X]\n", i, tmp[i]); 
			}
		}
#endif
	}	
	
	return ret; 
}
                     
static ssize_t compass_write(struct file *fp, const char __user *buf,
                             size_t count, loff_t *pos)
{
	int ret = 0;
	char tmp[64];

	DBG(KERN_INFO, "\n");
	
/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
	if(fihsensor_ctx.bIsIdle == true && fihsensor_ctx.activeSlave == bma150_i2c_client)
	{
		DBG(KERN_INFO, "in idle mode\n");
		return -EIO;
	}
#endif
/* } FIH; Tiger; 2009/6/10 */

	if(fihsensor_ctx.activeSlave == NULL) 
	{
		printk(KERN_ERR "device invalid\n");
		ret = -ENODEV;	
	}
	else 
	{
		if(copy_from_user(tmp, buf, count))
		{
			ret = -EFAULT;
		}
		else
		{
#if DEBUG
			int i;
			for(i=0; i< count; i++) 
			{
				DBG(KERN_INFO "data [%d, 0x%X, %d]\n", i, tmp[i], count); 
			}
#endif			
			ret = i2c_master_send(fihsensor_ctx.activeSlave, tmp, count);
		}
		DBG(KERN_INFO, "active slave=0x%X\r\n", (unsigned int)fihsensor_ctx.activeSlave);

		DBG(KERN_ERR, "result(%d, %d, %d, 0x%02x)\n", 
				(fihsensor_ctx.activeSlave == yas529_i2c_client), 
				count, ret, fihsensor_ctx.activeSlave->addr);
	}

	return ret; 
}                                                  
                            
static struct file_operations compass_fops = {
	.owner =   THIS_MODULE,
	.open =    compass_open,
	.release = compass_release,
	.ioctl =   compass_ioctl,
	.read =    compass_read,
	.write =   compass_write,	
};

static struct miscdevice compass_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "compass",
	.fops = &compass_fops,
};

static int yas529_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	int rc;
	DBG(KERN_INFO, "\n");

	//config gpio and pmic
	verg_gp7_L8 = vreg_get(NULL, "gp7");

	if (IS_ERR(verg_gp7_L8)) {
		DBG(KERN_INFO, "gp7 vreg get failed (%ld)\n", PTR_ERR(verg_gp7_L8));
	}	
	
	verg_gp9_L12 = vreg_get(NULL, "gp9");
	if (IS_ERR(verg_gp9_L12)) {
		DBG(KERN_INFO, "gp9 vreg get failed (%ld)\n", PTR_ERR(verg_gp9_L12));
	}	


	rc = vreg_set_level(verg_gp7_L8, 1800);
	if (rc) {
		DBG(KERN_INFO, "vreg LDO8 set level failed (%d)\n", rc);
	}	

	rc = vreg_enable(verg_gp7_L8);
	if (rc) {
		DBG(KERN_INFO, "LDO8 vreg enable failed (%d)\n", rc);
	}	

	rc = vreg_set_level(verg_gp9_L12, 3000);
	if (rc) {
		DBG(KERN_INFO, "vreg LDO12 set level failed (%d)\n", rc);
	}	

	rc = vreg_enable(verg_gp9_L12);
	if (rc) {
		DBG(KERN_INFO, "LDO12 vreg enable failed (%d)\n", rc);
	}	

	rc = gpio_tlmm_config(GPIO_CFG(82, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);

	err = gpio_request(82, "YAS529_RST_PIN");
	if (err)
	{
		printk(KERN_ERR "YAS529 request gpio failed!\n");
		return err;
	}
	gpio_direction_output(82, 1);
	gpio_set_value(82, 1);


#if 0
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
#endif

	yas529_i2c_client = client;

	err = misc_register(&compass_device);
	if (err) {
		printk(KERN_ERR "compass_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	
	return 0;

exit_misc_device_register_failed:
	printk(KERN_ERR "fail\n");
	return err;
}

static int yas529_remove(struct i2c_client *client)
{
	//de-register /dev/comapss
	DBG(KERN_INFO, "\n");
	misc_deregister(&compass_device);
	
	return 0;
}

/*static int yas529_suspend(struct device *dev)
{
	//TODO: add suspend function
	DBG(KERN_INFO, "\n");
	return 0;
}

static int yas529_resume(struct device *dev)
{	
	//TODO: add resume function
	DBG(KERN_INFO, "\n");
	return 0;
}*/

static const struct i2c_device_id yas529_id[] = {
	{ "YAS529", 0 }, 
	{ }
};

/*struct dev_pm_ops yas529_driver_pm = {
     	.suspend 	= yas529_suspend,
     	.resume       	= yas529_resume,
};*/

static struct i2c_driver yas529_driver = {
	.probe 		= yas529_probe,
	.remove 	= yas529_remove,
	.id_table 	= yas529_id,
	.driver = {
		   .name = "YAS529", 
		   //.pm = &yas529_driver_pm,
	},
};

static int BMAI2C_RxData(char *rxData, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = bma150_i2c_client->addr,		// 0x38
		 .flags = 0,				// w:0	
		 .len = 1,				// len of register adress 
		 .buf = rxData,				// ex. 15h ==> wakeup
		 },
		{
		 .addr = bma150_i2c_client->addr,		// 0x38
		 .flags = I2C_M_RD,			// r:1
		 .len = length,				// len of data
		 .buf = rxData,				// data
		 },
	};

	if (i2c_transfer(bma150_i2c_client->adapter, msgs, 2) < 0) {
		printk(KERN_ERR "BMAI2C_RxData: transfer error\n");
		return -EIO;
	} else
		return 0;
}

static int BMAI2C_TxData(char *txData, int length)
{

	struct i2c_msg msg[] = {
		{
		 .addr = bma150_i2c_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	if (i2c_transfer(bma150_i2c_client->adapter, msg, 1) < 0) {
		printk(KERN_ERR "BMAI2C_TxData: transfer error\n");
		return -EIO;
	} else
		return 0;
}


static int BMA150_register_config(void)
{
	char buffer[4];

	//chip id, alversion, ml version
	memset(buffer, 0, 4); 
	buffer[0] = 0x0;
	BMAI2C_RxData(buffer, 4);
	DBG(KERN_INFO, "bma150 chip id:0x%x\n", buffer[0] & 0x07);

	memset(buffer, 0, 4); 
	buffer[0] = 0x1;
	BMAI2C_RxData(buffer, 4);
	DBG(KERN_INFO, "al_version:0x%x, ml_version:0x%x\n", (buffer[0] & 0xf0)>>4 , buffer[0] & 0x0f);

	//15h
	memset(buffer, 0, 4);
	buffer[0] = 0x15;
	BMAI2C_RxData(buffer, 4);
	DBG(KERN_INFO, "bma150 15h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		
	//memset(buffer, 0, 4);
	buffer[0] = 0x15;
	buffer[1] = 0x60;
	BMAI2C_TxData(buffer,2);

	memset(buffer, 0, 4);
	buffer[0] = 0x15;
	BMAI2C_RxData(buffer, 4);
	DBG(KERN_INFO, "bma150 15h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
#if 0
	//14h
	buffer[0] = 0x14;
	BMAI2C_RxData(buffer, 4);
	printk(KERN_INFO "bma150 14h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		
	memset(buffer, 0, 4);
	buffer[0] = 0x14;
	buffer[1] = 0x08;
	BMAI2C_TxData(buffer,2);

	memset(buffer, 0, 4);
	buffer[0] = 0x14;
	BMAI2C_RxData(buffer, 4);
	printk(KERN_INFO "bma150 14h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

	//11h
	buffer[0] = 0x11;
	BMAI2C_RxData(buffer, 4);
	printk(KERN_INFO "bma150 11h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n",
		buffer[0], buffer[1], buffer[2], buffer[3]);
		
	//memset(buffer, 0, 4);
	buffer[0] = 0x11;
	buffer[1] |= 0x40;
	BMAI2C_TxData(buffer,2);



	memset(buffer, 0, 4);
	buffer[0] = 0x11;
	BMAI2C_RxData(buffer, 4);
	printk(KERN_INFO "bma150 11h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n",
		buffer[0], buffer[1], buffer[2], buffer[3]);

	//10h
	buffer[0] = 0x10;
	BMAI2C_RxData(buffer, 4);
	printk(KERN_INFO "bma150 10h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n",
		buffer[0], buffer[1], buffer[2], buffer[3]);
		
	memset(buffer, 0, 4);
	buffer[0] = 0x10;
	buffer[1] = 0x3;
	BMAI2C_TxData(buffer,2);

	memset(buffer, 0, 4);
	buffer[0] = 0x10;
	BMAI2C_RxData(buffer, 4);
	printk(KERN_INFO "bma150 10h buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n",
		buffer[0], buffer[1], buffer[2], buffer[3]);

#endif	
	//0bh
	memset(buffer, 0, 4);
	buffer[0] = 0x0b;
	BMAI2C_RxData(buffer, 4);
	DBG(KERN_INFO, "bma150 0bh buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

	//memset(buffer, 0, 4);
	buffer[1] = 0xc0 | buffer[0]; //any_motion | alert
	buffer[0] = 0x0b;
	BMAI2C_TxData(buffer,2);

	memset(buffer, 0, 4);
	buffer[0] = 0x0b;
	BMAI2C_RxData(buffer, 4);
	DBG(KERN_INFO, "bma150 0bh buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

	memset(buffer, 0, 4);
	buffer[0] = 0x0a;
	BMAI2C_RxData(buffer, 4);
	DBG(KERN_INFO, "bma150 0ah buffer[0]:0x%x, buffer[1]:0x%x, buffer[2]:0x%x, buffer[3]:0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
	
	return 0;
}

static void bma150_work_func(struct work_struct *work)
{
	DBG(KERN_INFO, "BMA150 interrupt handler! irq:%d\n", bma150_i2c_client->irq);
	//enable_irq(smb380_i2c_client->irq);
}

static irqreturn_t bma150_irq_handler(int irq, void *dev_id)
{
	//TODO
	
	//disable_irq(smb380_i2c_client->irq);
	//schedule_work(&smb380_irq_work);

	return IRQ_HANDLED;
}

/*
static struct msm_gpio smb380_irq_gpios[] = {
    { GPIO_CFG(BMA150_INT_PIN, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), "smb380_irq" },
};
*/

static int bma150_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	DBG(KERN_INFO, "\n");

#if 0
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
#endif

	bma150_i2c_client = client;

	err =  BMA150_register_config();
	if (err)
	{
		printk(KERN_ERR "Register Configuration Failed!\n");
		return err;
	}

	INIT_WORK(&bma150_irq_work, bma150_work_func);

	//Configure GPIO

	gpio_tlmm_config(GPIO_CFG(BMA150_INT_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	err = gpio_request(BMA150_INT_PIN, "BMA_interrupt_pin");
	if (err)
	{
		printk(KERN_ERR "BMA150 request gpio failed!\n");
		return err;
	}

	err = gpio_direction_input(BMA150_INT_PIN);
	if (err)
	{
		printk(KERN_ERR "BMA150 set gpio direction failed!\n");
		return err;
	}
	
	DBG(KERN_ERR, "BMA150 request irq!\n");
	//Request IRQ
	
	err = request_irq(client->irq, bma150_irq_handler, IRQF_TRIGGER_FALLING, "compass", bma150_i2c_client);

	if (err)
	{
		printk(KERN_ERR "BMA150 request irq failed\n");
		free_irq(BMA150_INT_PIN, bma150_i2c_client);
		return err;
	}



/* FIH; Tiger; 2009/6/10 { */
/* implement suspend/resume */
#ifdef CONFIG_PM
	if(enter_mode(BMA150_MODE_SLEEP)) {
		DBG(KERN_ERR, "enter_mode(BMA150_MODE_SLEEP) fail\n");
	}
	else {
		fihsensor_ctx.bIsIdle = true;
	}
#endif
/* } FIH; Tiger; 2009/6/10 */

//Div6D1-OH-GSandES-SuspendResume-00+{
#ifdef CONFIG_HAS_EARLYSUSPEND
	//todo: register early suspend
	bma150_early_suspend.level = EARLY_SUSPEND_LEVEL_ECOMPASS_DRV;
	bma150_early_suspend.suspend = bma150_early_suspend_func;
	bma150_early_suspend.resume = bma150_late_resume_func;
	register_early_suspend(&bma150_early_suspend);
#endif
//Div6D1-OH-GSandES-SuspendResume-00+}

	//compass_sleep_mode();//Div6D1-OH-Sensors-ModeRules-00+

	return 0;

//exit_check_functionality_failed:
	//printk(KERN_ERR "[Ecompass]smb380_probe() fail\r\n");
	//return err;
}

static int bma150_remove(struct i2c_client *client)
{
	//i2c_detach_client(client);
	DBG(KERN_ERR, "\n");

	//Div6D1-OH-GS-Interrupt-00+{
	#if 1 
	free_irq(BMA150_INT_PIN, bma150_i2c_client); 
	#endif
	//Div6D1-OH-GS-Interrupt-00+}
	
	return 0;
}

/*static int bma150_suspend(struct device *dev)
{
// FIH; Tiger; 2009/6/10 {
// implement suspend/resume 
#ifdef CONFIG_PM
	DBG(KERN_INFO, "\n");

	// always execute it to recover state error
	//if(fihsensor_ctx.bIsIdle == false)
	{
		if(enter_mode(BMA150_MODE_SLEEP)) {
			DBG(KERN_ERR, "enter_mode(BMA150_MODE_SLEEP) fail\n");
		}
		else {
			fihsensor_ctx.bIsIdle = true;
		}
	}
#endif
// } FIH; Tiger; 2009/6/10

	DBG(KERN_INFO, "\n");
	return 0;
}*/

/*static int bma150_resume(struct device *dev)
{
// FIH; Tiger; 2009/6/10 { 
// implement suspend/resume 
#ifdef CONFIG_PM
	DBG(KERN_INFO, "\n");

	// always execute it to recover state error
	if(fihsensor_ctx.activeSlave)
	//if(fihsensor_ctx.activeSlave && fihsensor_ctx.bIsIdle==true)
	{
		if(enter_mode(BMA150_MODE_NORMAL)) {
			DBG(KERN_ERR, "enter_mode(BMA150_MODE_NORMAL) fail\n");
		}
		else {
			fihsensor_ctx.bIsIdle = false;
		}	
	}
#endif
// } FIH; Tiger; 2009/6/10 

	DBG(KERN_INFO, "\n");

	return 0;
}*/

static const struct i2c_device_id bma150_id[] = {
	{ "BMA150", 0 },
	{ }
};

/*struct dev_pm_ops bma150_driver_pm = {
     .suspend = bma150_suspend,
     .resume  = bma150_resume,
};*/

static struct i2c_driver bma150_driver = {
	.probe =      bma150_probe,
	.remove =     bma150_remove,
	.id_table =   bma150_id,
	.driver = {
		   .name = "BMA150", 
		   //.pm = &bma150_driver_pm,
	},
};

static int __init fihsensor_init(void)
{
	int ret;
	//int hwid = FIH_READ_HWID_FROM_SMEM();
	DBG(KERN_INFO, "\n");
	
	if((ret=i2c_add_driver(&yas529_driver)))
	{
		DBG(KERN_INFO, "YAMAHA MS-3C compass driver init fail\n");	
	}
	else if((ret = i2c_add_driver(&bma150_driver)))
	{
		i2c_del_driver(&yas529_driver);
		DBG(KERN_INFO, "BMA150 driver init fail\n");	
	}
	else
	{	
		DBG(KERN_INFO, "FIH sensor driver init success\n");	
	}

	return ret;
}

static void __exit fihsensor_exit(void)
{
	DBG(KERN_ALERT, "Un-Register Ecompass/Gsensor driver!\n");
	
	i2c_del_driver(&yas529_driver);
	i2c_del_driver(&bma150_driver);
}

module_init(fihsensor_init);
module_exit(fihsensor_exit);

MODULE_AUTHOR("Tiger JT Lee <TigerJTLee@fihtdc.com>");
MODULE_DESCRIPTION("YAMAHA MS-3C compass driver");
MODULE_LICENSE("GPL");
