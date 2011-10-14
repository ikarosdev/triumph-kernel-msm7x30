/*
 * include/linux/synaptics_i2c_t1320.h - platform data structure for f75375s sensor
 *
 * Copyright (C) 2008 Google, Inc.
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

#ifndef _LINUX_SYNAPTICS_I2C_T1320_H
#define _LINUX_SYNAPTICS_I2C_T1320_H

#define SYNAPTICS_TS_T1320_NAME	"synaptics-t1320-ts"
#define SYNAPTICS_TSKEY_NAME		"synaptics-t1320-tskey"
#define TOUCH_DEVICE_VREG			"gp9"
//vreg L12 - 3V
//vreg L8 - 1.8(I2C)

#define GPIO_TP_INT_N               42	// GPIO 42, TP INT pin, low active
#define GPIO_TP_RST_N               56	// GPIO 56, TP Reset pin

#define LOW                         0
#define HIGH                        1
#define FALSE                      0
#define TRUE                        1

#ifndef CONFIG_FIH_FTM
#define SYNAP_TS_MT	//multi touch
#endif

bool palm_detect_en = false; 
bool palm_detected = false;
bool TPL_PR2 = false;

#define TS_P0	0x01
#define TS_P1	0x04
#define TS_P2	0x10
#define TS_P3	0x40
#define TS_P4	0x01

int	idx_finger_state0;
int	idx_finger_state1;

int	idx_p0;
	#define idx_x0_hi		idx_p0
	#define idx_y0_hi		idx_p0+1
	#define idx_x0y0_lo	idx_p0+2
int	idx_p1;
	#define idx_x1_hi		idx_p1
	#define idx_y1_hi		idx_p1+1
	#define idx_x1y1_lo	idx_p1+2
int	idx_p2;
	#define idx_x2_hi		idx_p2
	#define idx_y2_hi		idx_p2+1
	#define idx_x2y2_lo	idx_p2+2
int	idx_p3;
	#define idx_x3_hi		idx_p3
	#define idx_y3_hi		idx_p3+1
	#define idx_x3y3_lo	idx_p3+2
int	idx_p4;
	#define idx_x4_hi		idx_p4
	#define idx_y4_hi		idx_p4+1
	#define idx_x4y4_lo	idx_p4+2

int idx_palm;
/*
enum{
int	idx_int_status=0;
int	idx_finger_state0=1;
int	idx_finger_state1=2;

int	idx_x0_hi=3;
int	idx_y0_hi=4;
int	idx_x0y0_lo=5;
int	idx_wxwy_0=6;
int	idx_z0=7;

int	idx_x1_hi=8;
int	idx_y1_hi=9;
int	idx_x1y1_lo=10;
int	idx_wxwy_1=11;
int	idx_z1=12;

int	idx_palm=13;
int	idx_button=14;

#define	idx_last	idx_button+1;
	
};
*/
int int_status;

int REG_F11_CTRL;		// Max_X, Max_Y
int REG_F11_DATA;		// X,Y Position

int REG_F19_CTRL;		// Noist filter
int REG_F19_DATA;		// Button Data

int REG_F01_CTRL;		// Sleep mode
int REG_F01_DATA;		// Device status, INT status
int REG_F01_QUERY;	// FW ver.

#define default_max_x 1040
#define default_max_y 1812

#define REG_MAX_X	REG_F11_CTRL+6
#define REG_MAX_Y	REG_F11_CTRL+8
#define REG_2D_DATA	REG_F11_DATA
#define REG_PALM_DET	REG_F11_DATA+11

#define REG_INT_STATUS	REG_F01_DATA+1	//bit[3:0] = Button:Abs0(touch):Status:Flash
#define REG_SLEEP_MODE	REG_F01_CTRL
#define REG_INT_MASK	REG_F01_CTRL+1
#define REG_FW_VER	REG_F01_QUERY+3

#define REG_BUTTON	REG_F19_DATA

#define REG_PROPERTIES	0xEF

#define BYTE_PER_GROUP	6

#define CMD_OFFSET		0x01
#define CTRL_OFFSET		0x02
#define DATA_OFFSET	0x03

#define F34_FLASH_QUERY_BASE	REG_PROPERTIES-	BYTE_PER_GROUP		// read this reg. to get F34 QUERY start address
#define F34_FLASH_CMD_BASE	F34_FLASH_QUERY_BASE+CMD_OFFSET	// read this reg. to get F34 CMD start address
#define F34_FLASH_CTRL_BASE	F34_FLASH_QUERY_BASE+CTRL_OFFSET	// read this reg. to get F34 CTRL start address
#define F34_FLASH_DATA_BASE	F34_FLASH_QUERY_BASE+DATA_OFFSET	// read this reg. to get F34 DATA start address

#define F01_RMI_QUERY_BASE		F34_FLASH_QUERY_BASE-BYTE_PER_GROUP	// read this reg. to get F01 QUERY start address
#define F01_RMI_CMD_BASE		F01_RMI_QUERY_BASE+CMD_OFFSET			// read this reg. to get F01 CMD start address
#define F01_RMI_CTRL_BASE		F01_RMI_QUERY_BASE+CTRL_OFFSET			// read this reg. to get F01 CTRL start address
#define F01_RMI_DATA_BASE		F01_RMI_QUERY_BASE+DATA_OFFSET		// read this reg. to get F01 DATA start address

#define F11_2D_QUERY_BASE		F01_RMI_QUERY_BASE-BYTE_PER_GROUP	// Send I2C cmd, read this reg. , get F11 QUERY start address
#define F11_2D_CMD_BASE		F11_2D_QUERY_BASE+CMD_OFFSET		// Send I2C cmd, read this reg. , get F11 CMD start address
#define F11_2D_CTRL_BASE		F11_2D_QUERY_BASE+CTRL_OFFSET		// Send I2C cmd, read this reg. , get F11 CTRL start address
#define F11_2D_DATA_BASE		F11_2D_QUERY_BASE+DATA_OFFSET		// Send I2C cmd, read this reg. , get F11 DATA start address

#define F19_0D_QUERY_BASE		F11_2D_QUERY_BASE-BYTE_PER_GROUP	// Send I2C cmd, read this reg. , get F19 QUERY start address
#define F19_0D_CMD_BASE		F19_0D_QUERY_BASE+CMD_OFFSET		// Send I2C cmd, read this reg. , get F19 CMD start address
#define F19_0D_CTRL_BASE		F19_0D_QUERY_BASE+CTRL_OFFSET		// Send I2C cmd, read this reg. , get F19 CTRL start address
#define F19_0D_DATA_BASE		F19_0D_QUERY_BASE+DATA_OFFSET		// Send I2C cmd, read this reg. , get F19 DATA start address

enum {
	SYNAPTICS_FLIP_X = 1UL << 0,
	SYNAPTICS_FLIP_Y = 1UL << 1,
	SYNAPTICS_SWAP_XY = 1UL << 2,
	SYNAPTICS_SNAP_TO_INACTIVE_EDGE = 1UL << 3,
};

#endif /* _LINUX_SYNAPTICS_I2C_T1320_H */
