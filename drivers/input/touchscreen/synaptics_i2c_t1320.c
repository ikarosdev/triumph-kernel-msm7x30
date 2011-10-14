/* drivers/input/keyboard/synaptics_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/synaptics_i2c_t1320.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include "../../../arch/arm/mach-msm/smd_private.h"
#include "../../../arch/arm/mach-msm/proc_comm.h"


#define DBG_MSG(msg, ...) printk("[TOUCH] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define ERR_MSG(msg, ...) printk("[TOUCH_ERR] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define PWR_MSG(msg, ...) printk("[TOUCH_PWR] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define TRK_MSG(msg, ...) printk("[TOUCH_INFO] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)

extern unsigned int fih_get_product_id(void);
extern unsigned int fih_get_product_phase(void);

static struct workqueue_struct *synaptics_wq;

struct synaptics_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *key_input_dev;	
	struct work_struct  work;
	uint32_t flags;
	struct early_suspend early_suspend;
};

bool continue_read = true;
bool Finger0_pressed, Finger1_pressed = false;
bool allow_key_event = true;
int BUTTON_STATUS=0;
int FingerStatus=0;
int INT_STATUS=0;
int num_keys=0;
int button_data;
uint16_t LastX1, LastY1, LastX2, LastY2, LastX3, LastY3, LastX4, LastY4, LastX5, LastY5;
int BUF_LEN;
	
#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif

static int synaptics_poweron_reset(struct i2c_client *client)
{
	int i = 0;
	struct vreg *device_vreg;
	int rc = 0;
	int ret;
	
	device_vreg = vreg_get(0, TOUCH_DEVICE_VREG);
	if (!device_vreg) {
		ERR_MSG("vreg get failed \n");
		return 1;
	}

	vreg_set_level(device_vreg, 3000);
	rc = vreg_enable(device_vreg);
	DBG_MSG("Power status(3V) = %d", rc);
			
	gpio_set_value(GPIO_TP_RST_N, LOW);            
	mdelay(20);
	gpio_set_value(GPIO_TP_RST_N, HIGH);    
	mdelay(250);

	i=0;
ts_init_f01_ctrl:	
	//Get F01_CTRL reg. start addr. , use this addr. to set Sleep Mode, and Interrupt Enable Mask
	REG_F01_CTRL = i2c_smbus_read_byte_data(client, F01_RMI_CTRL_BASE);		// get F01 Ctrl start addr.
	if(REG_F01_CTRL < 0){
		ERR_MSG("Get F01_CTRL fail\n");	
		mdelay(20);
		i++;
		if(i>10)
			return 1;
		goto ts_init_f01_ctrl;
	}
	DBG_MSG("F01 CTRL START:0x%x ", REG_F01_CTRL);

	ret = i2c_smbus_write_byte_data(client, REG_INT_MASK, 0);		// disable interrupt
	if (ret < 0)
		ERR_MSG("Disable INT failed !!\n");
	
	i=0;
ts_init_f01_data:	
	//Get F01_DATA reg. start addr. , use this addr. to get INT Status
	REG_F01_DATA = i2c_smbus_read_byte_data(client, F01_RMI_DATA_BASE);	// get F01 Data start addr.
	if(REG_F01_DATA < 0){
		ERR_MSG("Get F01_DATA fail\n");	
		mdelay(20);
		i++;
		if(i>10)
			return 1;
		goto ts_init_f01_data;
	}
	DBG_MSG("F01 DATA START:0x%x, REG_INT_STATUS:0x%x ", REG_F01_DATA, REG_INT_STATUS);

	i=0;
ts_init_f01_query:	
	//Get F01_QUERY reg. start addr. , use this addr. to get FW Version
	REG_F01_QUERY = i2c_smbus_read_byte_data(client, F01_RMI_QUERY_BASE);	// get F01 Query start addr.
	if(REG_F01_QUERY < 0){
		ERR_MSG("Get F01_DATA fail\n");	
		mdelay(20);
		i++;
		if(i>10)
			return 1;
		goto ts_init_f01_query;
	}
	ret = i2c_smbus_read_byte_data(client, REG_FW_VER);
	DBG_MSG("F01 QUERY START:0x%x, FW Version = %d ", REG_F01_QUERY, ret);
	if(ret != 0)
		palm_detect_en = true;

	i=0;
ts_init_f11_ctrl:
	//Get F11_CTRL reg. start addr. , use this addr. to get Max_X, Max_Y (in the later synaptics_ts_probe function)
	REG_F11_CTRL = i2c_smbus_read_byte_data(client, F11_2D_CTRL_BASE);	// get F11 2D CTRL start addr.
	if(REG_F11_CTRL < 0)	{
		ERR_MSG("Get F11_CTRL fail\n");	
		mdelay(20);
		i++;
		if(i>10)
			return 1;
		goto ts_init_f11_ctrl;
	}
	DBG_MSG("F11 CTRL START:0x%x, REG_MAX_X:0x%x, REG_MAX_Y:0x%x ", REG_F11_CTRL, REG_MAX_X, REG_MAX_Y);

	i=0;
ts_init_f11_data:
	//Get F11_DATA reg. start addr. , use this addr. to get X1/Y1, X2/Y2 and Button Data
	REG_F11_DATA = i2c_smbus_read_byte_data(client, F11_2D_DATA_BASE);	// get F11 2D DATA start addr.
	if(REG_F11_DATA < 0)	{
		ERR_MSG("Get F11_DATA fail\n");	
		mdelay(20);
		i++;
		if(i>10)
			return 1;
		goto ts_init_f11_data;
	}
	DBG_MSG("F11 DATA START:0x%x ", REG_F11_DATA);
	
	i=0;
ts_init_f19_ctrl:	
	//Get F19_CTRL reg. start addr. , use this addr. to set Noise Fileter Enabled (in the later synaptics_ts_probe function)
	REG_F19_CTRL = i2c_smbus_read_byte_data(client, F19_0D_CTRL_BASE);	// get F19 0D CTRL start addr.
	if(REG_F19_CTRL < 0){
		ERR_MSG("Get F19_CTRL fail\n");	
		mdelay(20);
		i++;
		if(i>10)
			return 1;
		goto ts_init_f19_ctrl;
	}
	DBG_MSG("F19 CTRL START:0x%x ", REG_F19_CTRL);

	i=0;
ts_init_f19_data:	
	//Get F19_DATA reg. start addr. , use this addr. to get Button Data
	REG_F19_DATA = i2c_smbus_read_byte_data(client, F19_0D_DATA_BASE);	// get F19 0D DATA start addr.
	if(REG_F19_DATA < 0){
		ERR_MSG("Get F19_DATA fail\n");	
		mdelay(20);
		i++;
		if(i>10)
			return 1;
		goto ts_init_f19_data;
	}
	DBG_MSG("F19 DATA START:0x%x ", REG_F19_DATA);
	
	for (i=0; i<10; i++)
	{
		ret = i2c_smbus_read_byte_data(client, REG_INT_STATUS);		// read this, INT must return to high
		if(ret < 0)
			ERR_MSG("Read INT Stauts fail ! #%d", i);
		else
		{
			mdelay(20);
			if (gpio_get_value(GPIO_TP_INT_N) == HIGH)
			{	
				DBG_MSG("T1320 reset OK - %d ", i);
				return 0;
			}
		}
	}	

	DBG_MSG("T1320 reset Failed\n");
	return 1;
}

static inline void input_report(struct input_dev *dev, unsigned short  x, unsigned short  y, int down, int pressure) 
{
#ifdef SYNAP_TS_MT
	if(down)
		input_report_abs(dev, ABS_MT_TOUCH_MAJOR, 255);
	else	
		input_report_abs(dev, ABS_MT_TOUCH_MAJOR, 0);		
	input_report_abs(dev, ABS_MT_POSITION_X, x);
	input_report_abs(dev, ABS_MT_POSITION_Y, y);
	input_mt_sync(dev);
#else	
	input_report_abs(dev, ABS_X, x);
	input_report_abs(dev, ABS_Y, y);
	input_report_abs(dev, ABS_PRESSURE, pressure);
	input_report_key(dev, BTN_TOUCH, down);
	input_sync(dev);
#endif	
	
}

static void synaptics_ts_work_func(struct work_struct *work)
{
	int i,ret;
	struct i2c_msg msg[2];
	uint8_t start_reg;
	uint8_t buf[BUF_LEN];
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data, work);
//	int buf_len = idx_palm;

	//DBG_MSG("+");
	
	if((gpio_get_value(GPIO_TP_INT_N) == LOW) || continue_read)
	{
		//buf[idx_int_status] = i2c_smbus_read_byte_data(ts->client, REG_INT_STATUS);		//F01 : INT Status
		int_status = i2c_smbus_read_byte_data(ts->client, REG_INT_STATUS);		//F01 : INT Status
		
		//if(buf[idx_int_status] == 0)		//buf[0] : Interrupt Status 
		if(int_status == 0)		//buf[0] : Interrupt Status 
		{
			if(continue_read)
			{	
				//buf[idx_int_status] = INT_STATUS;
				int_status = INT_STATUS;
				
				DBG_MSG("Restore INT Status = %d", INT_STATUS);
			}	
		}
		
		// ===========
		// Touch Event
		// ===========		
		//if(buf[idx_int_status] & 0x04)
		if(int_status & 0x04)
		{
			msg[0].addr = ts->client->addr;
			msg[0].flags = 0;
			msg[0].len = 1;
			msg[0].buf = &start_reg;
			start_reg = REG_2D_DATA;		//F11 : 2D Data
			msg[1].addr = ts->client->addr;
			msg[1].flags = I2C_M_RD;
			msg[1].len = BUF_LEN;
			msg[1].buf = buf;
			ret = i2c_transfer(ts->client->adapter, msg, 2);
			if (ret < 0) {
				ERR_MSG("Read reg %x - %d bytes : i2c_transfer failed", start_reg, BUF_LEN);
			}
			else
			{
				printk("0x%x ", int_status);					
				for(i=0; i<BUF_LEN; i++)
					printk("[%d]%x ", i, buf[i]);	
				printk("\n");

				if(palm_detect_en && (buf[idx_palm] & 0x01))		// if palm detect enabled & palm detected
				{
					palm_detected = true;
					DBG_MSG("PALM Detected (T) !! Do nothing... ");
				}
				else
				{
#ifdef SYNAP_TS_MT
					if(int_status & 0x08)		//8-4-C-4 //4-C-4-4
					{	
						i2c_smbus_read_byte_data(ts->client, REG_BUTTON);
						if(BUTTON_STATUS & 0x01)
						{	
							BUTTON_STATUS &= ~0x01;
							DBG_MSG("+Back Key Up ");
							input_report_key(ts->key_input_dev, KEY_BACK, 0);
						}	
						if(BUTTON_STATUS & 0x02)
						{	
							BUTTON_STATUS &= ~0x02;							
							DBG_MSG("+Menu Key Up ");
							input_report_key(ts->key_input_dev, KEY_MENU, 0);
						}	
						if(BUTTON_STATUS & 0x04)
						{	
							BUTTON_STATUS &= ~0x04;
							DBG_MSG("+Home Key Up ");
							input_report_key(ts->key_input_dev, KEY_HOME, 0);	
						}	
					}
						
					allow_key_event = false;
					if(buf[idx_finger_state0] & 0x01)		// Finger 0
					{	
						LastX1 = ((buf[idx_x0_hi] << 4) | ( buf[idx_x0y0_lo] & 0x0F));
						LastY1 = ((buf[idx_y0_hi] << 4) | (( buf[idx_x0y0_lo] & 0xF0)>>4));
						input_report(ts->input_dev, LastX1, LastY1, 1, 256);
						DBG_MSG("P0(%d,%d)DN ", LastX1, LastY1);
					}
					if(buf[idx_finger_state0] & 0x04)		// Finger 1
					{	
						LastX2 = (( buf[idx_x1_hi] << 4) |  (buf[idx_x1y1_lo] & 0x0F));
						LastY2 = (( buf[idx_y1_hi] << 4) |  ((buf[idx_x1y1_lo] & 0xF0)>>4));									
						input_report(ts->input_dev, LastX2, LastY2, 1, 256);
						DBG_MSG("P1(%d,%d)DN ", LastX2, LastY2);						
					}
					if(TPL_PR2)
					{
						if(buf[idx_finger_state0] & 0x10)		// Finger 2
						{	
							LastX3 = (( buf[idx_x2_hi] << 4) |  (buf[idx_x2y2_lo] & 0x0F));
							LastY3 = (( buf[idx_y2_hi] << 4) |  ((buf[idx_x2y2_lo] & 0xF0)>>4));									
							input_report(ts->input_dev, LastX3, LastY3, 1, 256);
							DBG_MSG("P2(%d,%d)DN ", LastX3, LastY3);						
						}
						
						if(buf[idx_finger_state0] & 0x40)		// Finger 3
						{	
							LastX4 = (( buf[idx_x3_hi] << 4) |  (buf[idx_x3y3_lo] & 0x0F));
							LastY4 = (( buf[idx_y3_hi] << 4) |  ((buf[idx_x3y3_lo] & 0xF0)>>4));									
							input_report(ts->input_dev, LastX4, LastY4, 1, 256);
							DBG_MSG("P3(%d,%d)DN ", LastX4, LastY4);						
						}
					
						if(buf[idx_finger_state1] & 0x01)		// Finger 4
						{	
							LastX5 = (( buf[idx_x4_hi] << 4) |  (buf[idx_x4y4_lo] & 0x0F));
							LastY5 = (( buf[idx_y4_hi] << 4) |  ((buf[idx_x4y4_lo] & 0xF0)>>4));
							input_report(ts->input_dev, LastX5, LastY5, 1, 256);
							DBG_MSG("P4(%d,%d)DN ", LastX5, LastY5);												
						}
					}
					if((buf[idx_finger_state0]==0x0)&&(buf[idx_finger_state1]==0x0))
					{
						DBG_MSG("Touch Up ");						
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
						input_mt_sync(ts->input_dev);
						
						allow_key_event = true;
					}						
					input_sync(ts->input_dev);
					
//				if(FingerStatus != buf[idx_finger_state0])	// Finger State Changed
//				{
//					if(buf[idx_finger_state0] == 0x01)	// (No Finger --> Finger0) or (Finger0 & Finger1 -->  Finger0)
//					{
//						if(FingerStatus == 0)	// (No Finger --> Finger0) 	--> Send Finger0 Down
//						{
//							DBG_MSG("P0-X1= %d ,Y1= %d -DN ", LastX1, LastY1);
//							Finger0_pressed = true;
//							input_report(ts->input_dev, LastX1, LastY1, 1, 256);
//						}	
//						else if(FingerStatus == 5)	// (Finger0 & Finger1 -->  Finger0) --> Send Finger1 Up & Finger0 Down
//						{
//							DBG_MSG("P0-X1= %d ,Y1= %d -DN ", LastX1, LastY1);
//							Finger0_pressed = true;
//							input_report(ts->input_dev, LastX1, LastY1, 1, 256);
//						}	
//						else if(FingerStatus == 4)	// (Finger1 --> Finger0) --> Send Finger0 Down && Finger1 Up
//						{
//							DBG_MSG("P0-X1= %d ,Y1= %d -DN ", LastX1, LastY1);
//							Finger0_pressed = true;
//							input_report(ts->input_dev, LastX1, LastY1, 1, 256);
//						}	
//					}
//					else if(buf[idx_finger_state0] == 0x05)	// (Finger0 --> Finger0 & Finger 1) or (Finger1 --> Finger0 & Finger1) or (No Finger --> Finger0 & Finger1) or (No Finger --> Finger0 & Finger1)
//					{
//						DBG_MSG("P0-X1= %d ,Y1= %d -DN ", LastX1, LastY1);
//						Finger0_pressed = true;
//						input_report(ts->input_dev, LastX1, LastY1, 1, 256);
//
//						DBG_MSG("P1-X2= %d ,Y2= %d -DN ", LastX2, LastY2);
//						Finger1_pressed = true;
//						input_report(ts->input_dev, LastX2, LastY2, 1, 256);	
//					}
//					else if(buf[idx_finger_state0] == 0x04)	// (Finger0 & Finger1 --> Finger1)
//					{
//						if(FingerStatus == 5)	// (Finger0 & Finger1 --> Finger1) --> Send Finger0 Up & Finger1 Down
//						{
//							DBG_MSG("P1-X2 = %d ,Y2= %d -DN ", LastX2, LastY2);
//							Finger1_pressed = true;
//							input_report(ts->input_dev, LastX2, LastY2, 1, 256);	
//						}
//						else
//						{
//							ERR_MSG("!!! CheckCheckCheck !!!\n");	
//						}		
//					}	
//					else if(buf[idx_finger_state0] == 0x0)	// (Finger0 --> No Finger) or (Finger0 & Finger 1 --> No Finger) or (Finger 1 --> No Finger)
//					{
//						DBG_MSG("Touch Up ");						
//						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
//						input_mt_sync(ts->input_dev);
//					}	
//					input_sync(ts->input_dev);
//				}
//				else
#else
					if(buf[idx_finger_state0] & 0x01)		// Finger 0
					{	
						Finger0_pressed = true;
						LastX1 = ((buf[idx_x0_hi] << 4) | ( buf[idx_x0y0_lo] & 0x0F));
						LastY1 = ((buf[idx_y0_hi] << 4) | (( buf[idx_x0y0_lo] & 0xF0)>>4));
						input_report(ts->input_dev, LastX1, LastY1, 1, 256);
						DBG_MSG("*P0(%d,%d)DN ", LastX1, LastY1);						
					}
					if(buf[idx_finger_state0] == 0)		// touch event, but no finger detected --> send touch up event
					{
						if(Finger0_pressed)	
						{
							DBG_MSG("*P0(%d,%d)UP ", LastX1, LastY1);
							Finger0_pressed = false;
							input_report(ts->input_dev, LastX1, LastY1, 0, 0);	
						}	
					}
#endif					
//				{	
//					if(buf[idx_finger_state0] & 0x01)		// Finger 0
//					{	
//						DBG_MSG("P0-X1= %d , Y1= %d -DN ", LastX1, LastY1);
//						Finger0_pressed = true;
//						input_report(ts->input_dev, LastX1, LastY1, 1, 256);
//					}	
//
//					if(buf[idx_finger_state0] & 0x04)		// Finger 1
//					{	
//						DBG_MSG("P1-X2= %d ,Y2= %d -DN ", LastX2, LastY2);
//						Finger1_pressed = true;
//						input_report(ts->input_dev, LastX2, LastY2, 1, 256);	
//					}	
//			
//					if(buf[idx_finger_state0] == 0)		// touch event, but no finger detected --> send touch up event
//					{
//						#ifdef SYNAP_TS_MT
//						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
//						input_mt_sync(ts->input_dev);
//						#else
//					
//						if(Finger0_pressed)	
//						{
//							DBG_MSG("P0-X1= %d ,Y1= %d -UP ", LastX1, LastY1);						
//							Finger0_pressed = false;
//							input_report(ts->input_dev, LastX1, LastY1, 0, 0);	
//						}	
//				
//						if(Finger1_pressed)	
//						{
//							DBG_MSG("P1-X2= %d ,Y2= %d -UP ", LastX2, LastY2);		
//							Finger1_pressed = false;
//							input_report(ts->input_dev, LastX2, LastY2, 0, 0);	
//						}
//						#endif						
//					}
//					#ifdef SYNAP_TS_MT
//						input_sync(ts->input_dev);
//					#endif
//				}
					FingerStatus = buf[idx_finger_state0];				
				}
			}
		}
		

		// ===========
		// Key Event
		// ===========		
		else if(int_status & 0x08)
		{
			if(palm_detect_en)
				if(!palm_detected)
				{
					buf[idx_palm] = i2c_smbus_read_byte_data(ts->client, REG_PALM_DET);
					if(buf[idx_palm] & 0x01)
						palm_detected = true;
				}

			button_data = i2c_smbus_read_byte_data(ts->client, REG_BUTTON);
				
			if(palm_detected)	
				DBG_MSG("PALM Detected (K) !!");
			else
			{
				ret = button_data;		//i2c_smbus_read_word_data(ts->client, 0x20);
				num_keys=0;
				for(i=0; i<3; i++)
				{
					if(ret & 0x01)	
						num_keys++;
					ret = ret	>> 1;
				}
			
				if(num_keys >=2)
					DBG_MSG("==IGNORE KEYS== (0x%x)\n", ret);
				else
				{
					if(allow_key_event)
					{	
						if(button_data & 0x01)
						{	
							if(!(BUTTON_STATUS & 0x01))
							{		
								BUTTON_STATUS |= 0x01;	
								DBG_MSG("Back Key Down(0x%x)", int_status);
								input_report_key(ts->key_input_dev, KEY_BACK, 1);
								//input_sync(ts->input_dev);
							}	
						}	
						if(button_data & 0x02)
						{	
							if(!(BUTTON_STATUS & 0x02))
							{								
								BUTTON_STATUS |= 0x02;							
								DBG_MSG("Menu Key Down(0x%x)", int_status);
								input_report_key(ts->key_input_dev, KEY_MENU, 1);
								//input_sync(ts->input_dev);						
							}	
						}	
						if(button_data & 0x04)
						{	
							if(!(BUTTON_STATUS & 0x04))
							{		
								BUTTON_STATUS |= 0x04;							
								DBG_MSG("Home Key Down(0x%x)", int_status);
								input_report_key(ts->key_input_dev, KEY_HOME, 1);
								//input_sync(ts->input_dev);						
							}	
						}	
							
						if(button_data == 0)
						{
							if(BUTTON_STATUS & 0x01)
							{	
								BUTTON_STATUS &= ~0x01;
								DBG_MSG("Back Key Up(0x%x)", int_status);							
								input_report_key(ts->key_input_dev, KEY_BACK, 0);
								//input_sync(ts->input_dev);							
							}	
							if(BUTTON_STATUS & 0x02)
							{	
								BUTTON_STATUS &= ~0x02;							
								DBG_MSG("Menu Key Up(0x%x)", int_status);														
								input_report_key(ts->key_input_dev, KEY_MENU, 0);
								//input_sync(ts->input_dev);							
							}	
							if(BUTTON_STATUS & 0x04)
							{	
								BUTTON_STATUS &= ~0x04;
								DBG_MSG("Home Key Up(0x%x)", int_status);														
								input_report_key(ts->key_input_dev, KEY_HOME, 0);	
								//input_sync(ts->input_dev);							
							}	
						}	
					}
					else	//8-4-8-4	//4-8-4-8
					{
						if(BUTTON_STATUS & 0x01)
						{	
							BUTTON_STATUS &= ~0x01;
							DBG_MSG("*Back Key Up(0x%x/0x%x)",int_status, button_data);							
							input_report_key(ts->key_input_dev, KEY_BACK, 0);
						}	
						if(BUTTON_STATUS & 0x02)
						{	
							BUTTON_STATUS &= ~0x02;							
							DBG_MSG("*Menu Key Up(0x%x/0x%x)",int_status, button_data);														
							input_report_key(ts->key_input_dev, KEY_MENU, 0);
						}	
						if(BUTTON_STATUS & 0x04)
						{	
							BUTTON_STATUS &= ~0x04;
							DBG_MSG("*Home Key Up(0x%x/0x%x)",int_status, button_data);														
							input_report_key(ts->key_input_dev, KEY_HOME, 0);	
						}
					}	
				}				
			}
		}
	}
	else
	{
		ret= gpio_get_value(GPIO_TP_INT_N);
		printk("!!! XXX INT=%d, continue_read=%d\n", ret, continue_read);
	}

	palm_detected = false;
	INT_STATUS = i2c_smbus_read_byte_data(ts->client, REG_INT_STATUS);	
	if(INT_STATUS)
	{
		printk("Continue Read - REG_INT_STATUS = %d, BUTTON_STATUS = %d\n", INT_STATUS, BUTTON_STATUS);
		continue_read = true;
		queue_work(synaptics_wq, &ts->work);
	}
	else
	{	
		continue_read = false;	
		printk("Complete read - exit >>> \n");
	}		
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;

	//disable_irq_nosync(ts->client->irq);
	queue_work(synaptics_wq, &ts->work);
	return IRQ_HANDLED;
}

static int synaptics_ts_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	int ret=0; 
	int i=0;
	uint16_t max_x, max_y = 0;

	if (fih_get_product_id() == Product_SF5 && fih_get_product_phase() == Product_PR2)
	{
		DBG_MSG("PR2 or later devices, 5 PTS Enabled\n");
		TPL_PR2 = true;
		BUF_LEN = 28;
		idx_finger_state0=0;
		idx_finger_state1=1;
		idx_p0=2;
		idx_p1=7;
		idx_p2=12;
		idx_p3=17;
		idx_p4=22;
		idx_palm=27;
	}
	else
	{
		BUF_LEN = 12;
		idx_finger_state0=0;
		idx_p0=1;
		idx_p1=6;
		idx_palm=11;
	}	
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ERR_MSG("need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	// Set GPIO
	gpio_tlmm_config(GPIO_CFG(GPIO_TP_INT_N, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_TP_RST_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);	

	// Set I2C client data
	ts->client = client;
	i2c_set_clientdata(client, ts);

	// Power-on reset - Do power-on reset & get register start address for each group.
	if(synaptics_poweron_reset(ts->client))
		goto err_power_failed;

	ret = i2c_smbus_read_word_data(ts->client, REG_MAX_X);		// 2D Max X Position
	if (ret < 0) {
		ERR_MSG("i2c_smbus_read_word_data failed - 0x%x\n", REG_MAX_X);
		ret = default_max_x;
		//goto err_detect_failed;
	}
	DBG_MSG("Max X Position = %d", ret);
	max_x = ret;
		
	ret = i2c_smbus_read_word_data(ts->client, REG_MAX_Y);		// 2D Max Y Position
	if (ret < 0) {
		ERR_MSG("i2c_smbus_read_word_data failed - 0x%x\n", REG_MAX_Y);
		ret = default_max_y;
		//goto err_detect_failed;
	}
	DBG_MSG("Max Y Position = %d", ret);
	max_y = ret;
	
// input_allocate_device - touch
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		ERR_MSG("Failed to allocate touch input device\n");
		goto err_input_dev_alloc_failed;
	}

// input_allocate_device - key
	ts->key_input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		ERR_MSG("Failed to allocate key input device\n");
		goto err_input_dev_alloc_failed;
	}
	
	ts->input_dev->name = SYNAPTICS_TS_T1320_NAME;

	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_SYN, ts->input_dev->evbit);
		
	printk(KERN_INFO "synaptics_ts_probe: max_x %d, max_y %d\n", max_x, max_y);

#ifndef SYNAP_TS_MT
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(BTN_2, ts->input_dev->keybit);
	input_set_abs_params(ts->input_dev, ABS_X, 0, max_x, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0, max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_HAT0X, 0, max_x, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_HAT0Y, 0, max_y, 0, 0);
#else	
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
#endif

	ts->key_input_dev-> name = SYNAPTICS_TSKEY_NAME;
	set_bit(EV_KEY, ts->key_input_dev->evbit);	
	set_bit(KEY_HOME, ts->key_input_dev->keybit);
	set_bit(KEY_MENU, ts->key_input_dev->keybit);
	set_bit(KEY_BACK, ts->key_input_dev->keybit);
	
	INIT_WORK(&ts->work, synaptics_ts_work_func);

// Register input device - touch 
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s touch input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}

// Register input device - keys
	ret = input_register_device(ts->key_input_dev);
	if (ret) {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s key input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	
	client->irq = MSM_GPIO_TO_INT(GPIO_TP_INT_N);

	ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING, client->name, ts);
	if (ret)
		dev_err(&client->dev, "request_irq failed\n");
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	for(i=0; i<10; i++)
	{
		ret = i2c_smbus_write_byte_data(ts->client, REG_F19_CTRL, 4);	// enable noise filter for buttons
		if(ret < 0)
			ERR_MSG("Enable SW noise filter failed \n");
		else
		{	
			DBG_MSG("SW noise filter enabled - %d.", i);
			break;
		}	
	}
	for(i=0; i<10; i++)
	{
		ret = i2c_smbus_write_byte_data(client, REG_INT_MASK, 0x0F);	//enable INT mask
		if(ret < 0)
			ERR_MSG("Enable INT mask failed \n");
		else
		{	
			DBG_MSG("INT mask enabled - %d.", i);
			break;
		}
	}
	printk(KERN_INFO "%s: Start touchscreen %s in Complete !! \n", __func__, ts->input_dev->name);

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);
	input_free_device(ts->key_input_dev);
err_input_dev_alloc_failed:
//err_detect_failed:
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);
	input_unregister_device(ts->input_dev);
	input_unregister_device(ts->key_input_dev);	
	kfree(ts);
	return 0;
}

static int synaptics_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	disable_irq(client->irq);

	ret = cancel_work_sync(&ts->work);
	
	ret = i2c_smbus_write_byte_data(client, REG_INT_MASK, 0);		// disable interrupt
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data 0x22 failed\n");

	ret = i2c_smbus_write_byte_data(client, REG_SLEEP_MODE, 0x01);	// deep sleep
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data 0x21 failed\n");

	return 0;
}

static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	int count=0;

clear_sleep_mode:
	ret = i2c_smbus_write_byte_data(client, REG_SLEEP_MODE, 0);
	if (ret < 0)
	{	
		printk(KERN_ERR "synaptics_ts_resume: write REG_SLEEP_MODE failed\n");
		count++;
		if(count < 10)
			goto clear_sleep_mode;	
	}

	enable_irq(client->irq);

	count=0;
enable_int:
	ret = i2c_smbus_write_byte_data(client, REG_INT_MASK, 0x0F);
	if (ret < 0)
	{	
		printk(KERN_ERR "synaptics_ts_resume: write REG_INT_MASK failed\n");
		count++;
		if(count < 10)
			goto enable_int;	
	}

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{ SYNAPTICS_TS_T1320_NAME, 0 },
	{ }
};

static struct i2c_driver synaptics_ts_driver = {
	.probe		= synaptics_ts_probe,
	.remove		= synaptics_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_ts_suspend,
	.resume		= synaptics_ts_resume,
#endif
	.id_table	= synaptics_ts_id,
	.driver = {
		.name	= SYNAPTICS_TS_T1320_NAME,
	},
};

static int __devinit synaptics_ts_init(void)
{
	int ret = 0;
	DBG_MSG("+");	

	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq)
		return -ENOMEM;
			
	ret = i2c_add_driver(&synaptics_ts_driver);
	if(ret)
		DBG_MSG("i2c add driver - Failed !!\r\n");
	else
		DBG_MSG("i2c add driver - Success !!\r\n");	
	
	DBG_MSG("-");	
			
	return ret;
}

static void __exit synaptics_ts_exit(void)
{
	DBG_MSG("+");
	i2c_del_driver(&synaptics_ts_driver);

	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
	DBG_MSG("-");		
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_DESCRIPTION("Synaptics Touchscreen Driver");
MODULE_LICENSE("GPL");
