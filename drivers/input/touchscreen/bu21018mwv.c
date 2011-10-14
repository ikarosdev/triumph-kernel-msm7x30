#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/bu21018mwv.h>
#include <linux/bu21018mwv_fw.h>
#include <linux/bu21018mwv_fw1.h>
#include "../../../arch/arm/mach-msm/smd_private.h"
#include "../../../arch/arm/mach-msm/proc_comm.h"

//
extern unsigned int fih_get_product_id(void);
extern unsigned int fih_get_product_phase(void);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bu21018mwv_ts_early_suspend(struct early_suspend *h);
static void bu21018mwv_ts_late_resume(struct early_suspend *h);
#endif

// virtual key threshold (only for PR1)
static int btn_th_on[4] = {TOUCH_BTN0_THON, TOUCH_BTN1_THON, TOUCH_BTN2_THON, TOUCH_BTN3_THON};
static int btn_th_off[4] = {TOUCH_BTN0_THOFF, TOUCH_BTN1_THOFF, TOUCH_BTN2_THOFF, TOUCH_BTN3_THOFF};

// dynamic debug message
static int btn_msg = 0;
static int coor_msg = 0;

// switch ioctl action
static int iosw = 0;

// report delay
static int rdelay = 10;

//
static int t_max_x, t_min_x, t_max_y, t_min_y;

// flag of HW type
static int hw_ver = HW_UNKNOW;

//
static int gLastkey = 0;
static int gPrevious = EVENT_NONE;
static int gKeypress = 0;

//
int bu21018mwv_active = 0;

// for FQC firmware version
static int msm_cap_touch_fw_version;

module_param_named(
    fw_version, msm_cap_touch_fw_version, int, S_IRUGO | S_IWUSR | S_IWGRP
);

static char I2C_ByteRead(char reg)
{
	char rxData[4] = {0};
	struct i2c_msg msgs[] = {
		{
		 .addr = bu21018mwv->client->addr,
		 .flags = 0,
		 .len = 1,
		 },
		{
		 .addr = bu21018mwv->client->addr,
		 .flags = I2C_M_RD,
		 .len = 1,
		 },
	};

	rxData[0] = reg;
	
	msgs[0].buf = rxData;
	msgs[1].buf = rxData;
	
	if (i2c_transfer(bu21018mwv->client->adapter, msgs, 2) < 0) {
		printk(KERN_ERR "%s: transfer error (REG %2x)\n", __func__, reg);
		return -EIO;
	} else
		return rxData[0];
}

static int I2C_ByteWrite(char reg, char data)
{
	char txData[2] = {0};
	struct i2c_msg msg[] = {
		{
		 .addr = bu21018mwv->client->addr,
		 .flags = 0,
		 .len = 2,
		 },
	};
	
	txData[0] = reg;
	txData[1] = data;
	
	msg[0].buf = txData;

	if (i2c_transfer(bu21018mwv->client->adapter, msg, 1) < 0) {
		printk(KERN_ERR "%s: transfer error (REG %2x)\n", __func__, reg);
		return -EIO;
	} else
		return 0;
}

static int bu21018mwv_get_ver(void)
{
    unsigned int ver[2];

    ver[0] = (unsigned int)I2C_ByteRead(0xFE);
    ver[1] = (unsigned int)I2C_ByteRead(0xFF);

    ver[0] = ver[0] << 8;
    ver[0] = ver[0] + ver[1];

    msm_cap_touch_fw_version = ver[0];

    printk(KERN_INFO "[Touch] bu21018mwv firmware version = 0x%x\n", ver[0]);

    return 0;
}

static int my_atox(const char *ptr)
{
	int value = 0;
	char ch = *ptr;

	ch = *(ptr+=2);

    for (;;)
    {
		if (ch >= '0' && ch <= '9')
			value = (value << 4) + (ch - '0');
		else if (ch >= 'A' && ch <= 'F')
			value = (value << 4) + (ch - 'A' + 10);
		else if (ch >= 'a' && ch <= 'f')
			value = (value << 4) + (ch - 'a' + 10);
		else
			return value;
		ch = *(++ptr);
    }
}

static int my_atoi(const char *name)
{
    int val = 0;

    for (;; name++) {
	switch (*name) {
	    case '0' ... '9':
		val = 10*val+(*name-'0');
		break;
	    default:
		return val;
	}
    }
}

static void bu21018mwv_read_ini(void)
{
    static struct file *filp = NULL;
    int i = 0;
    int j = 0;
    int k = 0;
	int c = 0;
	int bEOF = 0;
    int nRead = 0;
    unsigned char buf[512];
    unsigned char num[5];
    unsigned char *p = NULL;

    filp = filp_open("/data/touch.ini", O_RDONLY, 0);

    if (IS_ERR(filp))
    {
        filp = NULL;
        printk(KERN_ERR "[Touch] Open ini file error!\n");
        return;
    }

    p = kmalloc(1, GFP_KERNEL);
    
    for (;;)
    {
    	c = 0;
    	memset(buf, 0 , 512);
    	memset(num, 0 , 5);
    
    	do{
    		nRead = filp->f_op->read(filp, (unsigned char __user *)p, 1, &filp->f_pos);
    	
    		if (nRead != 1)
    		{
				bEOF = 1;
				break;
			}
		
			buf[c++] = *p;
    	}while(*p != '\n');
    
    	if (!bEOF)
    	{
    		buf[c] = '\0';
    	
    		if (strncmp("BTN_TH_ON=", buf, strlen("BTN_TH_ON=")) == 0)
    		{
    			j = 0;
    			k = 0;
    		
    			for(i=strlen("BTN_TH_ON=");i<c;i++)
    			{
    				if (buf[i] == ',' || buf[i] == '\n')
    				{
    					num[j] = '\0';
    					j = 0;
    					btn_th_on[k] = my_atoi(num);
    					printk(KERN_INFO "[Touch] btn_th_on[%d] = %d\n", k, btn_th_on[k]);
    					k++;
    				}
    				else
    					num[j++] = buf[i];
    			}
    		}
    		else if (strncmp("BTN_TH_OFF=", buf, strlen("BTN_TH_OFF=")) == 0)
    		{
    			j = 0;
    			k = 0;
    		
    			for(i=strlen("BTN_TH_OFF=");i<c;i++)
    			{
    				if (buf[i] == ',' || buf[i] == '\n')
    				{
    					num[j] = '\0';
    					j = 0;
    					btn_th_off[k] = my_atoi(num);
    					printk(KERN_INFO "[Touch] btn_th_off[%d] = %d\n", k, btn_th_off[k]);
    					k++;
    				}
    				else
    					num[j++] = buf[i];
    			}
    		}
		}
		else
			break;
	}

	// close ini file
	vfs_fsync(filp, filp->f_path.dentry, 1);
    filp_close(filp, NULL);
    filp = NULL;
}

static void re_init(void)
{
    static struct file *filp = NULL;
	int c = 0;
	int i = 0;
	int j = 0;
	int reg = 0;
	int val = 0;
	int bEOF = 0;
    int nRead = 0;
    unsigned char buf[512];
    unsigned char num[10];
    unsigned char *p = NULL;

    filp = filp_open("/data/tp_init", O_RDONLY, 0);

    if (IS_ERR(filp))
    {
        filp = NULL;
        printk(KERN_ERR "[Touch] Open tp_init file error!\n");
        return;
    }

    p = kmalloc(1, GFP_KERNEL);
    
   	I2C_ByteWrite(0xB1, 0x01);
	I2C_ByteWrite(0xC7, 0x00);
    
    for (;;)
    {
    	c = 0;
    	memset(buf, 0 , 512);
    	memset(num, 0 , 10);
    
    	do{
    		nRead = filp->f_op->read(filp, (unsigned char __user *)p, 1, &filp->f_pos);
    	
    		if (nRead != 1)
    		{
				bEOF = 1;
				break;
			}
		
			buf[c++] = *p;
    	}while(*p != '\n');
    	
    	if (!bEOF)
    	{
    		buf[c] = '\0';
    		j = 0;
    		
    		for(i=0;i<c;i++)
    		{
				if (buf[i] == ',')
				{
					num[j] = '\0';
					j = 0;
					reg = my_atox(num);
				}
				else if (buf[i] == '\n')
				{
					num[j] = '\0';
					j = 0;
					val = my_atox(num);
					I2C_ByteWrite(reg, val);
					printk(KERN_INFO "[Touch] tp_init (REG = 0x%x, VAL = 0x%x)\n", reg, val);
				}
				else
					num[j++] = buf[i];
    		}
    	}
    	else
    		break;
    }

	// close ini file
	vfs_fsync(filp, filp->f_path.dentry, 1);
    filp_close(filp, NULL);
    filp = NULL;

	I2C_ByteWrite(0xB1, 0x00);
	I2C_ByteWrite(0xC7, 0x01);
}

static void re_download_fw(void)
{
    static struct file *filp = NULL;
	int c = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int val = 0;
	int bEOF = 0;
    int nRead = 0;
    unsigned char buf[512];
    unsigned char num[10];
    unsigned char *p = NULL;

    filp = filp_open("/data/tp_fw", O_RDONLY, 0);

    if (IS_ERR(filp))
    {
        filp = NULL;
        printk(KERN_ERR "[Touch] Open tp_fw file error!\n");
        return;
    }

    p = kmalloc(1, GFP_KERNEL);
    
    printk(KERN_INFO "[Touch] Re-download firmware\n");
    
    I2C_ByteWrite(0xB5, 0x02);
	I2C_ByteWrite(0xF0, 0x00);
	I2C_ByteWrite(0xF1, 0x00);
    
    for (;;)
    {
    	c = 0;
    	memset(buf, 0 , 512);
    	memset(num, 0 , 10);
    
    	do{
    		nRead = filp->f_op->read(filp, (unsigned char __user *)p, 1, &filp->f_pos);
    	
    		if (nRead != 1)
    		{
				bEOF = 1;
				break;
			}
		
			buf[c++] = *p;
    	}while(*p != '\n');
    	
    	if (!bEOF)
    	{
    		buf[c] = '\0';
    		j = 0;
    		
   			for(i=0;i<c;i++)
   			{
   				if (buf[i] == ',')
   				{
   					num[j] = '\0';
   					val = my_atoi(num);
   					I2C_ByteWrite(0xF2, val);
   					k++;
   				}
   				else
   					num[j++] = buf[i];
    		}
    	}
    	else
    		break;
    }
    
    for (i=k;i<16384;i++)
    	I2C_ByteWrite(0xF2, 0xFF);
    
   	I2C_ByteWrite(0xB5, 0x00);
   	
	// close ini file
	vfs_fsync(filp, filp->f_path.dentry, 1);
    filp_close(filp, NULL);
    filp = NULL;
}

static void test_func(struct work_struct *work)
{
	switch (iosw)
	{
		case 0:
			bu21018mwv_read_ini();
			break;
		case 1:
			re_init();
			break;
		case 2:
			re_download_fw();
			re_init();
			break;
		default:
			break;
	}
}

static void pre_init_func(struct work_struct *work)
{
	unsigned char calib_chk = 0;
	
	if (hw_ver > HW_PR1)
	{
		del_timer(&bu21018mwv->timer);
    	// Get touch firmware version
   		bu21018mwv_get_ver();
	}
	else	// for PR1
	{
		calib_chk = I2C_ByteRead(0x90);
		
		if (calib_chk == 0)
		{
			I2C_ByteWrite(0xC8, 0x01);
			printk(KERN_INFO "[Touch] Software calibration FAIL.\n");
			mod_timer(&bu21018mwv->timer, jiffies + (HZ * PRE_INIT_TIME));
		}
		else
		{
			printk(KERN_INFO "[Touch] Software calibration SUCCESS.\n");
			del_timer(&bu21018mwv->timer);
		}
	}
}

static int check_channel(void)
{
	int i = 0, j = 0, rc = 0, val = 0;
	int dac_min[] = {140, 140, 135, 128, 120, 120, 120, 120, 120, 140, 140, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 120, 120, 120, 120, 120, 120, 120, 130, 130, 150, 150};
	int dac_max[] = {150, 150, 145, 145, 145, 145, 145, 145, 145, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 145, 145, 145, 145, 145, 145, 145, 155, 150, 180, 180};
	
	I2C_ByteWrite(0xC9, 0x01);
	
	for (i=0x3F; i<=0x5F; i++)
	{
		val = (unsigned int)I2C_ByteRead(i);
		
		if (val < dac_min[j] || val > dac_max[j])
		{
			rc = 1;
			printk(KERN_INFO "[Touch] REG(%x) = %d (%d ~ %d) ERROR!!!\n", i, val, dac_min[j], dac_max[j]);
		}
		else
			printk(KERN_INFO "[Touch] REG(%x) = %d (%d ~ %d)\n", i, val, dac_min[j], dac_max[j]);
			
		j++;
	}
	
	return rc;
}

static void read_info(void)
{
	int i = 0;
	
	for (i=0; i<36; i++)
		printk(KERN_INFO "[Touch] SIN(%d) = %d\n", i, (unsigned int)I2C_ByteRead(i));
		
	printk(KERN_INFO "[Touch] REG(0x90) = %d\n", (unsigned int)I2C_ByteRead(0x90));
	
	printk(KERN_INFO "[Touch] Current setting is %d\n", hw_ver);
		
	printk(KERN_INFO "[Touch] firmware version = 0x%x\n", msm_cap_touch_fw_version);
}

static void pre_init(unsigned long ptr)
{
    schedule_work(&bu21018mwv->pre_init_work);
}

static int chip_init(void)
{
	// initial
	if (I2C_ByteWrite(0xB1, 0x01))
		return 1;
		
	I2C_ByteWrite(0xC7, 0x00);
	
	if (hw_ver > HW_PR1)
	{
		I2C_ByteWrite(0xB3,0x00);
		I2C_ByteWrite(0xB4,0x00);
		I2C_ByteWrite(0xB7,0x00);
		I2C_ByteWrite(0xB8,0x00);
		I2C_ByteWrite(0xB9,0x0F);
		I2C_ByteWrite(0xBA,0x10);
		I2C_ByteWrite(0xBB,0x08);
		I2C_ByteWrite(0xBC,0x00);
		I2C_ByteWrite(0xBD,0x12);
		I2C_ByteWrite(0xBE,0x12);
		I2C_ByteWrite(0xBF,0x03);
		I2C_ByteWrite(0xC0,0x0F);
		I2C_ByteWrite(0xC1,0x0F);
		I2C_ByteWrite(0xC2,0x40);
		I2C_ByteWrite(0xC3,0x20);
		I2C_ByteWrite(0xC4,0x15);
		I2C_ByteWrite(0xC5,0x20);
		I2C_ByteWrite(0xC6,0x15);
		I2C_ByteWrite(0xC9,0x00);
		I2C_ByteWrite(0xD0,0xF8);
		I2C_ByteWrite(0xD1,0xFF);
		I2C_ByteWrite(0xD2,0xFF);
		I2C_ByteWrite(0xD3,0xFF);
		I2C_ByteWrite(0xD4,0x0F);
		I2C_ByteWrite(0xD5,0x00);
		I2C_ByteWrite(0xD6,0x30);
		I2C_ByteWrite(0xD7,0x00);
		I2C_ByteWrite(0xD8,0x00);
		I2C_ByteWrite(0xD9,0x0C);
		I2C_ByteWrite(0xDA,0x07);
		I2C_ByteWrite(0xDB,0x08);
		I2C_ByteWrite(0xDC,0xA3);
		I2C_ByteWrite(0xDD,0xA2);
		I2C_ByteWrite(0xDE,0x8D);
		I2C_ByteWrite(0xDF,0x8C);
		I2C_ByteWrite(0xE4,0x00);
		I2C_ByteWrite(0xE5,0x00);
		I2C_ByteWrite(0xEC,0x11);
		I2C_ByteWrite(0xED,0x0A);
		I2C_ByteWrite(0xE6,0x01);
		I2C_ByteWrite(0xE7,0x01);
		I2C_ByteWrite(0xE0,0x20);
		I2C_ByteWrite(0xE1,0x10);
		I2C_ByteWrite(0xE2,0x02);
		I2C_ByteWrite(0xE3,0x02);
		I2C_ByteWrite(0xEE,0x40);
		I2C_ByteWrite(0xEF,0x05);
	}
	else	// for PR1
	{
		I2C_ByteWrite(0xC7, 0x00);
		I2C_ByteWrite(0xB3, 0x00);
		I2C_ByteWrite(0xB4, 0x00);
		I2C_ByteWrite(0xB7, 0x00);
		I2C_ByteWrite(0xB8, 0x00);
		I2C_ByteWrite(0xB9, 0x0F);
		I2C_ByteWrite(0xBA, 0x10);
		I2C_ByteWrite(0xBB, 0x08);
		I2C_ByteWrite(0xBC, 0x00);
		I2C_ByteWrite(0xBD, 0x15);
		I2C_ByteWrite(0xBE, 0x10);  // 0x12
		I2C_ByteWrite(0xBF, 0x03);
		I2C_ByteWrite(0xC0, 0x0f);
		I2C_ByteWrite(0xC1, 0x0F);
		I2C_ByteWrite(0xC2, 0x40);
		I2C_ByteWrite(0xC3, 0x45);  // X Th_On
		I2C_ByteWrite(0xC4, 0x35);  // X Th_Off
		I2C_ByteWrite(0xC5, 0x45);  // Y Th_On
		I2C_ByteWrite(0xC6, 0x35);  // Y Th_Off0
		I2C_ByteWrite(0xC9, 0x00);
		I2C_ByteWrite(0xD0, 0xF8);
		I2C_ByteWrite(0xD1, 0xFF);
		I2C_ByteWrite(0xD2, 0xFF);
		I2C_ByteWrite(0xD3, 0xFF);
		I2C_ByteWrite(0xD4, 0x0F);
		I2C_ByteWrite(0xD5, 0x00);
		I2C_ByteWrite(0xD6, 0x30);
		I2C_ByteWrite(0xD7, 0x00);
		I2C_ByteWrite(0xD8, 0x00);
		I2C_ByteWrite(0xD9, 0x0C);
		I2C_ByteWrite(0xDA, 0x07);
		I2C_ByteWrite(0xDB, 0x00);  //Moving Avg
		I2C_ByteWrite(0xE4, 0x00);
		I2C_ByteWrite(0xE5, 0x00);
		I2C_ByteWrite(0xEC, 0x11);
		I2C_ByteWrite(0xED, 0x0A);
		I2C_ByteWrite(0xE7, 0x10);
		//add
		I2C_ByteWrite(0xE0, 0x15);  //BTN INT ON_TH For Btn0, Btn3
		I2C_ByteWrite(0xE1, 0x10);  //BTN INT ON_TH For Btn1, Btn2
		I2C_ByteWrite(0xE2, 0x12);
		I2C_ByteWrite(0xE3, 0x0F);
		I2C_ByteWrite(0xEE, 0x80);
		I2C_ByteWrite(0xEF, 0x08);
	}
	
	I2C_ByteWrite(0xB1, 0x00);
	I2C_ByteWrite(0xC7, 0x01);
	
	return 0;
}

#define FW_BLOCK_SIZE 32

static int fw_download(void)
{
	int i = 0;
	int retry = 3;
	int rc = 0;
	int block = CODESIZE / FW_BLOCK_SIZE;
	int bytes = CODESIZE % FW_BLOCK_SIZE;
	int offset;
	
	// firmware download
	for (i=0; i<retry; i++)
	{
		rc = I2C_ByteWrite(0xB5, 0x02);
		if (rc == 0) break;
	}
	
	if (rc != 0) return 1;	// I2C fail abort
	
	I2C_ByteWrite(0xF0, 0x00);
	I2C_ByteWrite(0xF1, 0x00);
	
	if (hw_ver > HW_PR1)
	{
		for (i = 0; i < block; i++)
		{
     		offset = i * FW_BLOCK_SIZE;
			i2c_smbus_write_i2c_block_data(bu21018mwv->client, 0xf2, FW_BLOCK_SIZE, &program[offset]);
		}
		
		offset = block * FW_BLOCK_SIZE;
		
		for (i = 0; i < bytes; i++)
		{
       		i2c_smbus_write_byte_data(bu21018mwv->client, 0xf2, program[offset + i]);
		}
	}
	else	// for PR1
	{
		for (i=0; i<16384; i++)
		{
			if (i < CODESIZE1)
				I2C_ByteWrite(0xF2, program1[i]);
			else
				I2C_ByteWrite(0xF2, 0xFF);
		}
	}

	I2C_ByteWrite(0xB5, 0x00);
	
	return 0;
}

static int read_btns(unsigned int * data)
{	
	static unsigned char btn_on_flag = 0;
	s32 btn_id;
	
	btn_id = I2C_ByteRead(0x90);
	
	if (btn_msg)
		printk(KERN_INFO "[Touch] key id = %d\n", btn_id);
	
    if (btn_id != 0)
    {
		btn_on_flag = TRUE;

		if (hw_ver == HW_FD1_PR3)
		{
			if (btn_id == 1)
            	data[0] = V_KEY_MENU;
			else if (btn_id == 2)
            	data[0] = V_KEY_HOME;
			else if (btn_id == 3)
            	data[0] = V_KEY_SEARCH;
			else if(btn_id == 4)
				data[0] = V_KEY_BACK;
		}
		else if (hw_ver == HW_FD1_PR4)
		{
			if (btn_id == 1)
            	data[0] = V_KEY_MENU;
			else if (btn_id == 2)
            	data[0] = V_KEY_HOME;
			else if (btn_id == 3)
            	data[0] = V_KEY_BACK;
			else if(btn_id == 4)
				data[0] = V_KEY_SEARCH;
		}
		else
		{
			if (btn_id == 1)
            	data[0] = V_KEY_BACK;
			else if (btn_id == 2)
            	data[0] = V_KEY_MENU;
			else if (btn_id == 3)
            	data[0] = V_KEY_HOME;
			else if(btn_id == 4)
				data[0] = V_KEY_SEARCH;
		}
	}
	else
	{
        btn_on_flag = FALSE;
	}

	return btn_on_flag;
}

static int read_btns_pr1(unsigned int * data)
{	
	static unsigned char btn_on_flag = 0;
	s32 btns_value[4], btn_id;
	int i;

	btns_value[0] = I2C_ByteRead(RAW_SIN35_REG);
	btns_value[1] = I2C_ByteRead(RAW_SIN34_REG);
	btns_value[2] = I2C_ByteRead(RAW_SIN13_REG);
	btns_value[3] = I2C_ByteRead(RAW_SIN12_REG);

	if (btn_msg)
    	printk(KERN_INFO "[Touch] btn[0]=%d,btn[1]=%d,btn[2]=%d,btn[3]=%d\n", btns_value[0], btns_value[1], btns_value[2], btns_value[3]);
    
	btn_id = 0;
	
	for (i=0; i<4; i++)
	{
		if (btns_value[btn_id] < btns_value[i])
			btn_id = i;
	}
	
	//if (((btns_value[btn_id] > TOUCH_BTN_THON)&&(btn_on_flag == 0))||
	//((btns_value[btn_id] > TOUCH_BTN_THOFF)&&(btn_on_flag == 1)))
    if ((((btns_value[0] > btn_th_on[0])&&(btn_on_flag == 0))||
        ((btns_value[0] > btn_th_off[0])&&(btn_on_flag == 1)))||
        (((btns_value[1] > btn_th_on[1])&&(btn_on_flag == 0))||
        ((btns_value[1] > btn_th_off[1])&&(btn_on_flag == 1)))||
        (((btns_value[2] > btn_th_on[2])&&(btn_on_flag == 0))||
        ((btns_value[2] > btn_th_off[2])&&(btn_on_flag == 1)))||
        (((btns_value[3] > btn_th_on[3])&&(btn_on_flag == 0))||
        ((btns_value[3] > btn_th_off[3])&&(btn_on_flag == 1))))
    {
		btn_on_flag = TRUE;

		if (btn_id == 0)
            data[0] = V_KEY_BACK;
		else if (btn_id == 1)
            data[0] = V_KEY_MENU;
		else if (btn_id == 2)
            data[0] = V_KEY_HOME;
		else
			data[0] = V_KEY_SEARCH;
	}
	else
	{
        btn_on_flag = FALSE;
	}

	return btn_on_flag;
}

static int get_coordinate(unsigned int * data)
{
	unsigned int xy[4];//xy0, xy1, xy2, xy3;
#ifdef BU21018MWV_MT
	int ret = 0;
#endif
	
	// get coordinate
	xy[0] = (unsigned int)I2C_ByteRead(0x87) * 256 + I2C_ByteRead(0x88);
	xy[1] = (unsigned int)I2C_ByteRead(0x89) * 256 + I2C_ByteRead(0x8A);
	xy[2] = (unsigned int)I2C_ByteRead(0x8B) * 256 + I2C_ByteRead(0x8C);
	xy[3] = (unsigned int)I2C_ByteRead(0x8D) * 256 + I2C_ByteRead(0x8E);

    // point 1
    if ((xy[1] != 0) && (xy[0] != 0))
	{
		data[1] = abs(t_max_y - xy[0]);
		data[0] = xy[1];
        
        if (coor_msg)
			printk(KERN_INFO "%s: x1 = %d, y1 = %d\n", __func__, data[0], data[1]);

#ifdef BU21018MWV_MT
		ret = 1;
#else
		return TRUE;
#endif
	}
	else
	{
		data[0] = 0;
		data[1] = 0;
		
#ifndef BU21018MWV_MT
		return FALSE;  // no touch
#endif
    }
    
    // point 2
#ifdef BU21018MWV_MT
    if ((xy[2] != 0) && (xy[3] != 0))
	{
		data[3] = abs(t_max_y - xy[2]);
		data[2] = xy[3];
        
        if (coor_msg)
			printk(KERN_INFO "%s: x2 = %d, y2 = %d\n", __func__, data[2], data[3]);

		ret = 2;
	}
	else
	{
		data[2] = 0;
		data[3] = 0;
    }
#else
	data[2] = 0;	//xy2;
	data[3] = 0;	//xy3;
#endif

#ifdef BU21018MWV_MT
    return ret;
#endif
}

static void bu21018mwv_isr_workqueue(struct work_struct *work)
{
	unsigned int data[4] = {0};
	int getKey = FALSE;
	int getTouch = FALSE;
	int retry = 10;
	
	struct bu21018mwv_info *ts = container_of(work, struct bu21018mwv_info, wqueue);

    if (btn_msg || coor_msg)
    	printk(KERN_INFO "[Touch] %s: 0xA0 = %d\n", __func__, I2C_ByteRead(0xA0));
	
	if (ts->suspend_state) return;
//	disable_irq(ts->irq);
	
	getTouch = get_coordinate(data);
	getKey = read_btns(data);
		
	if (getTouch && (gKeypress == 0))
	{
#ifdef BU21018MWV_MT
		if (data[0] != 0 && data[1] != 0)
		{
			input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(ts->touch_input, ABS_MT_POSITION_X, data[0]);
			input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, data[1]);
			input_mt_sync(ts->touch_input);
		}
		
		if (getTouch > 1)
		{
			input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(ts->touch_input, ABS_MT_POSITION_X, data[2]);
			input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, data[3]);
			input_mt_sync(ts->touch_input);
		}
#else
		input_report_abs(ts->touch_input, ABS_X, data[0]);
		input_report_abs(ts->touch_input, ABS_Y, data[1]);
		input_report_abs(ts->touch_input, ABS_PRESSURE, 255);
		input_report_key(ts->touch_input, BTN_TOUCH, 1);
#endif
		input_sync(ts->touch_input);
		gPrevious = EVENT_TOUCH;
	}
	else
	{
		if (gPrevious == EVENT_TOUCH)
		{
#ifdef BU21018MWV_MT
		input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(ts->touch_input);
#else
		input_report_abs(ts->touch_input, ABS_PRESSURE, 0);
		input_report_key(ts->touch_input, BTN_TOUCH, 0);
#endif
			input_sync(ts->touch_input);
		}
	}
		
	if (getKey)
	{
#ifdef BU21018MWV_MT
		input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(ts->touch_input);
#else
		input_report_abs(ts->touch_input, ABS_PRESSURE, 0);
		input_report_key(ts->touch_input, BTN_TOUCH, 0);
#endif
		input_sync(ts->touch_input);

		if (data[0] == V_KEY_BACK)
			input_report_key(ts->keyevent_input, KEY_BACK, 1);
		else if (data[0] == V_KEY_MENU)
			input_report_key(ts->keyevent_input, KEY_MENU, 1);
		else if (data[0] == V_KEY_HOME)
			input_report_key(ts->keyevent_input, KEY_HOME, 1);
		else if (data[0] == V_KEY_SEARCH)
			input_report_key(ts->keyevent_input, KEY_SEARCH, 1);
                
		input_sync(ts->keyevent_input);
		gLastkey = data[0];
		gPrevious = EVENT_KEY;
		gKeypress = 1;
	}
	else
	{
		if (gPrevious == EVENT_KEY)
		{
        	if (gLastkey == V_KEY_BACK)
		    	input_report_key(ts->keyevent_input, KEY_BACK, 0);
        	else if (gLastkey == V_KEY_MENU)
            	input_report_key(ts->keyevent_input, KEY_MENU, 0);
        	else if (gLastkey == V_KEY_HOME)
            	input_report_key(ts->keyevent_input, KEY_HOME, 0);
        	else if (gLastkey == V_KEY_SEARCH)
            	input_report_key(ts->keyevent_input, KEY_SEARCH, 0);
        
			input_sync(ts->keyevent_input);
			gKeypress = 0;
		}
	}
    
//	enable_irq(ts->irq);

	// clear interrupt
	do
	{
		if (retry--)
			I2C_ByteWrite(0xBC, 0xFF);
		else
			break;
	}while(I2C_ByteRead(0xA0));
}

static void bu21018mwv_isr_workqueue_pr1(struct work_struct *work)
{
	unsigned int data[4] = {0};
	int getKey = FALSE;
	int getTouch = FALSE;
	int previous = EVENT_NONE;
	int reported = FALSE;
    int vKey = V_KEY_NONE;
	
	struct bu21018mwv_info *ts = container_of(work, struct bu21018mwv_info, wqueue);

    if (btn_msg || coor_msg)
    	printk(KERN_INFO "[Touch] %s: 0xA0 = %d\n", __func__, I2C_ByteRead(0xA0));
	
	disable_irq(ts->irq);
	
	do
	{
		getTouch = get_coordinate(data);
		getKey = read_btns_pr1(data);
		
		if (getTouch)
		{
#ifdef BU21018MWV_MT
			input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(ts->touch_input, ABS_MT_POSITION_X, data[0]);
			input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, data[1]);
			input_mt_sync(ts->touch_input);
			
			if (getTouch > 1)
			{
				input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 255);
				input_report_abs(ts->touch_input, ABS_MT_POSITION_X, data[2]);
				input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, data[3]);
				input_mt_sync(ts->touch_input);
			}
#else
			input_report_abs(ts->touch_input, ABS_X, data[0]);
			input_report_abs(ts->touch_input, ABS_Y, data[1]);
			input_report_abs(ts->touch_input, ABS_PRESSURE, 255);
			input_report_key(ts->touch_input, BTN_TOUCH, 1);
#endif
			input_sync(ts->touch_input);
			previous = EVENT_TOUCH;
		}
		
		if (getKey)
		{
			if (!reported)
			{
                if (data[0] == V_KEY_BACK)
				    input_report_key(ts->keyevent_input, KEY_BACK, 1);
                else if (data[0] == V_KEY_MENU)
				    input_report_key(ts->keyevent_input, KEY_MENU, 1);
                else if (data[0] == V_KEY_HOME)
				    input_report_key(ts->keyevent_input, KEY_HOME, 1);
                else if (data[0] == V_KEY_SEARCH)
				    input_report_key(ts->keyevent_input, KEY_SEARCH, 1);
                
				input_sync(ts->keyevent_input);
                vKey = data[0];
                reported = TRUE;
				previous = EVENT_KEY;
			}
		}

        msleep(rdelay);
	}while (getKey || getTouch);
                
	if (previous == EVENT_TOUCH)
	{
#ifdef BU21018MWV_MT
		input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(ts->touch_input);
#else
		input_report_abs(ts->touch_input, ABS_PRESSURE, 0);
		input_report_key(ts->touch_input, BTN_TOUCH, 0);
#endif
		input_sync(ts->touch_input);
	}
	
	if (previous == EVENT_KEY)
	{
        if (vKey == V_KEY_BACK)
		    input_report_key(ts->keyevent_input, KEY_BACK, 0);
        else if (vKey == V_KEY_MENU)
            input_report_key(ts->keyevent_input, KEY_MENU, 0);
        else if (vKey == V_KEY_HOME)
            input_report_key(ts->keyevent_input, KEY_HOME, 0);
        else if (vKey == V_KEY_SEARCH)
            input_report_key(ts->keyevent_input, KEY_SEARCH, 0);
        
		input_sync(ts->keyevent_input);
	}
    
	enable_irq(ts->irq);

	// clear interrupt
	do
	{
		I2C_ByteWrite(0xBC, 0xFF);
	}while(I2C_ByteRead(0xA0));
}

static irqreturn_t bu21018mwv_isr(int irq, void * handle)
{
    struct bu21018mwv_info *ts = handle;

    if (!ts->suspend_state)
        schedule_work(&ts->wqueue);

    return IRQ_HANDLED;
}

static int bu21018mwv_misc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int value;
    
	switch (cmd)
	{
		case READ_TH:
			iosw = 0;
			schedule_work(&bu21018mwv->test_work);
			break;
		case READ_INIT:
			iosw = 1;
			schedule_work(&bu21018mwv->test_work);
			break;
		case READ_FW:
			iosw = 2;
			schedule_work(&bu21018mwv->test_work);
			break;
		case READ_REG:
			read_info();
			break;
		case BTN_MSG:
			if (btn_msg)
				btn_msg = 0;
			else
				btn_msg = 1;
			
			printk(KERN_INFO "[Touch] Virtual key debug message %s\n", btn_msg ? "ON" : "OFF");
			break;
		case COOR_MSG:
			if (coor_msg)
				coor_msg = 0;
			else
				coor_msg = 1;
			
			printk(KERN_INFO "[Touch] Coordinate debug message %s\n", coor_msg ? "ON" : "OFF");
			break;
        case RPT_DELAY:
            if (copy_from_user(&value, argp, sizeof(value)))
                value = (unsigned int)arg;
            
            rdelay = value;
            
            printk(KERN_INFO "[Touch] Set report delay value = %d\n", value);
            break;
        case 7:
    		if (gpio_tlmm_config(GPIO_CFG(56, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
        		printk(KERN_ERR "[Touch] %s: gpio_tlmm_config: 56 failed.\n", __func__);
    		else
    		{
        		gpio_set_value(56, 1);
        		printk(KERN_INFO "[Touch] %s: touch power down.\n", __func__);
        	}
            break;
        case 8:
			if (gpio_tlmm_config(GPIO_CFG(56, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
        		printk(KERN_ERR "[Touch] %s: gpio_tlmm_config: 56 failed.\n", __func__);
    		else
    		{
        		gpio_set_value(56, 0);
        		printk(KERN_INFO "[Touch] %s: touch power on.\n", __func__);
        	}
        
			msleep(1);
		
			chip_init();
            break;
        case 9:
        	if (!check_channel())
        		printk(KERN_INFO "[Touch] %s: Test pass!.\n", __func__);
        	else
        	{
        		printk(KERN_INFO "[Touch] %s: Test fail!.\n", __func__);
        		return 1;
        	}
        	break;
		case 65002:
			return msm_cap_touch_fw_version;
			break;
		default:
			printk(KERN_ERR "[Touch] %s: unknow IOCTL.\n", __func__);
			break;
	}
	
	return 0;
}

static struct file_operations bu21018mwv_misc_fops = {
	.owner =   THIS_MODULE,
	.ioctl =   bu21018mwv_misc_ioctl,
};

static struct miscdevice bu21018mwv_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "bu21018mwv",
	.fops = &bu21018mwv_misc_fops,
};

static int bu21018mwv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct input_dev *touch_input;
	struct input_dev *keyevent_input;
	struct vreg *vreg_ldo12;
	int rc;

	// Read HWID
	if (fih_get_product_id() == Product_FB0 && fih_get_product_phase() == Product_PR1)
	{
		// FB0 PR1 but touch is PR2 (Check NV 3314)
		if (proc_comm_touch_id_read() == 1)
		{
			hw_ver = HW_MIX;
			printk(KERN_INFO "[Touch] Touch setting for PR2 and upper.\n");
		}
		// FB0 PR1
		else
		{
			hw_ver = HW_PR1;
			printk(KERN_INFO "[Touch] Touch setting for PR1.\n");
		}
	}
	// FD1 PR3
	else if ((fih_get_product_id() == Product_FD1 && fih_get_product_phase() == Product_PR3) ||
	        (fih_get_product_id() == Product_FD1 && fih_get_product_phase() == Product_PR231))
	{
		hw_ver = HW_FD1_PR3;
		printk(KERN_INFO "[Touch] Touch setting for FD1 PR235.\n");
	}
	else if ((fih_get_product_id() == Product_FD1 && fih_get_product_phase() == Product_PR4) ||
			(fih_get_product_id() == Product_FD1 && fih_get_product_phase() == Product_PR5))
	{
	    hw_ver = HW_FD1_PR4;
	    printk(KERN_INFO "[Touch] Touch setting for FD1 PR4.\n");
	}
	else if (fih_get_product_id() == Product_FD1)
	{
	    hw_ver = HW_FD1_PR4;
	    printk(KERN_INFO "[Touch] Virtual key mapping for FD1.\n");
	}
	// PR2
	else
	{
		hw_ver = HW_PR2;
		printk(KERN_INFO "[Touch] Touch setting for PR2 and upper.\n");
	}
	
    // Check I2C
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        printk(KERN_ERR "[Touch] %s: Check I2C functionality failed.\n", __func__);
        return -ENODEV;
    }

    bu21018mwv = kzalloc(sizeof(struct bu21018mwv_info), GFP_KERNEL);
    
    if (bu21018mwv == NULL)
    {
        printk(KERN_ERR "[Touch] %s: Can not allocate memory.\n", __func__);
        return -ENOMEM;
    }

    // Confirm Touch Chip
    bu21018mwv->client = client;
    
    i2c_set_clientdata(client, bu21018mwv);
    
    bu21018mwv->client->addr = 0x5C;
    
	// Initial power domain
	vreg_ldo12 = vreg_get(NULL, "gp9");

	if (IS_ERR(vreg_ldo12))
    {
		printk(KERN_ERR "%s: gp9 vreg get failed (%ld)\n", __func__, PTR_ERR(vreg_ldo12));
    	goto err1;
	}
	
	rc = vreg_set_level(vreg_ldo12, 3000);
    
	if (rc)
    {
		printk(KERN_ERR "%s: vreg LDO12 set level failed (%d)\n", __func__, rc);
    	goto err1;
	}
			
	rc = vreg_enable(vreg_ldo12);
    
	if (rc)
    {
        printk(KERN_ERR "%s: LDO12 vreg enable failed (%d)\n", __func__, rc);
    	goto err1;
    }
    
    // Reset chip
	if (hw_ver > HW_MIX)
	{
		if (gpio_tlmm_config(GPIO_CFG(56, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
			printk(KERN_ERR "[Touch] %s: gpio_tlmm_config: 56 failed.\n", __func__);
		else
		{
			gpio_set_value(56, 0);
			msleep(1);
		}
	}

    // Firmware download
    if (fw_download())
    {
    	printk(KERN_ERR "[Touch] %s: firmware download fail.\n", __func__);
    	goto err1;
    }
    
    // Initial chip
    if (chip_init())
    {
    	printk(KERN_ERR "[Touch] %s: chip initial fail.\n", __func__);
    	goto err1;
    }
    
	//
	touch_input = input_allocate_device();
	
    if (touch_input == NULL)
	{
		printk(KERN_ERR "[Touch] %s: Can not allocate memory for touch input device.\n", __func__);
		goto err1;
	}
	
	keyevent_input = input_allocate_device();
	
    if (keyevent_input == NULL)
	{
		printk(KERN_ERR "[Touch] %s: Can not allocate memory for key input device.\n", __func__);
		goto err2;
	}
	
	touch_input->name  = "bu21018mwv";
	touch_input->phys  = "bu21018mwv/input0";
	set_bit(EV_KEY, touch_input->evbit);
	set_bit(EV_ABS, touch_input->evbit);
	set_bit(EV_SYN, touch_input->evbit);
	set_bit(BTN_TOUCH, touch_input->keybit);
	set_bit(KEY_MENU, touch_input->keybit);

	if (hw_ver > HW_PR1)
	{
		t_max_x = TS_MAX_X;
		t_min_x = TS_MIN_X;
		t_max_y = TS_MAX_Y;
		t_min_y = TS_MIN_Y;
	}
	else
	{
		t_max_x = 673;
		t_min_x = 33;
		t_max_y = 1130;
		t_min_y = 33;
	}
	
#ifdef BU21018MWV_MT
    input_set_abs_params(touch_input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(touch_input, ABS_MT_POSITION_X, t_min_x, t_max_x, 0, 0);
    input_set_abs_params(touch_input, ABS_MT_POSITION_Y, t_min_y, t_max_y, 0, 0);
#else
	input_set_abs_params(touch_input, ABS_X, t_min_x, t_max_x, 0, 0);
	input_set_abs_params(touch_input, ABS_Y, t_min_y, t_max_y, 0, 0);
	input_set_abs_params(touch_input, ABS_PRESSURE, 0, 255, 0, 0);
#endif
	
    keyevent_input->name  = "bu21018mwvkey";
    keyevent_input->phys  = "bu21018mwv/input1";
    set_bit(EV_KEY, keyevent_input->evbit);
    set_bit(KEY_MENU, keyevent_input->keybit);
    set_bit(KEY_HOME, keyevent_input->keybit);
    set_bit(KEY_BACK, keyevent_input->keybit);
    set_bit(KEY_SEARCH, keyevent_input->keybit);
    
	bu21018mwv->touch_input = touch_input;
	
	if (input_register_device(bu21018mwv->touch_input))
	{
		printk(KERN_ERR "[Touch] %s: Can not register touch input device.\n", __func__);
		goto err3;
	}
    
	bu21018mwv->keyevent_input = keyevent_input;
	
	if (input_register_device(bu21018mwv->keyevent_input))
	{
		printk(KERN_ERR "[Touch] %s: Can not register key input device.\n", __func__);
		goto err4;
	}
	
	if (misc_register(&bu21018mwv_misc_dev))
	{
		printk(KERN_ERR "[Touch] %s: Can not register misc device.\n", __func__);
		goto err5;
	}

	// Init Work Queue and Register IRQ.
	if (hw_ver > HW_PR1)
    	INIT_WORK(&bu21018mwv->wqueue, bu21018mwv_isr_workqueue);
    else
    	INIT_WORK(&bu21018mwv->wqueue, bu21018mwv_isr_workqueue_pr1);
    
    gpio_tlmm_config(GPIO_CFG(42, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    
    bu21018mwv->irq = MSM_GPIO_TO_INT(42);
    
    if (request_irq(bu21018mwv->irq, bu21018mwv_isr, IRQF_TRIGGER_FALLING, bu21018mwv->client->dev.driver->name, bu21018mwv))
    {
        printk(KERN_ERR "[Touch] %s: Request IRQ failed.\n", __func__);
        goto err6;
    }
    
    bu21018mwv->suspend_state = 0;

    INIT_WORK(&bu21018mwv->pre_init_work, pre_init_func);
    INIT_WORK(&bu21018mwv->test_work, test_func);

    // timer for read firmware version (PR2), calibration (PR1)
    init_timer(&bu21018mwv->timer);
    bu21018mwv->timer.expires = jiffies + (HZ * PRE_INIT_TIME);
    bu21018mwv->timer.function = pre_init;
    add_timer(&bu21018mwv->timer);

#ifdef CONFIG_HAS_EARLYSUSPEND
	bu21018mwv->es.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 20;
	bu21018mwv->es.suspend = bu21018mwv_ts_early_suspend;
	bu21018mwv->es.resume = bu21018mwv_ts_late_resume;
	register_early_suspend(&bu21018mwv->es);
#endif

	bu21018mwv_active = 1;

    return 0;
    
err6:
	misc_deregister(&bu21018mwv_misc_dev);
err5:
    input_unregister_device(bu21018mwv->keyevent_input);
err4:
    input_unregister_device(bu21018mwv->touch_input);
err3:
    input_free_device(keyevent_input);
err2:
    input_free_device(touch_input);
err1:
    dev_set_drvdata(&client->dev, 0);
    kfree(bu21018mwv);
    printk(KERN_ERR "[Touch] %s: Failed.\n", __func__);
    return -1;
}

static int bu21018mwv_remove(struct i2c_client * client)
{
	printk(KERN_INFO "[Touch] %s\n", __func__);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&bu21018mwv->es);
#endif

	misc_deregister(&bu21018mwv_misc_dev);
	input_unregister_device(bu21018mwv->keyevent_input);
	input_unregister_device(bu21018mwv->touch_input);
    dev_set_drvdata(&client->dev, 0);
    kfree(bu21018mwv);
    return 0;
}

static int bu21018mwv_suspend(struct i2c_client *client, pm_message_t state)
{
    return 0;
}

static int bu21018mwv_resume(struct i2c_client *client)
{
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bu21018mwv_ts_early_suspend(struct early_suspend *h)
{
    printk(KERN_INFO "[Touch] %s\n", __func__);
    
    bu21018mwv->suspend_state = 1;
    I2C_ByteWrite(0xBB, 0x00);
    I2C_ByteWrite(0xBC, 0xFF);
    disable_irq(bu21018mwv->irq);

	if (hw_ver > HW_MIX)
	{
    	if (gpio_tlmm_config(GPIO_CFG(56, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
        	printk(KERN_ERR "[Touch] %s: gpio_tlmm_config: 56 failed.\n", __func__);
    	else
        	gpio_set_value(56, 1);
	}
}

static void bu21018mwv_ts_late_resume(struct early_suspend *h)
{
    printk(KERN_INFO "[Touch] %s\n", __func__);

	if (hw_ver > HW_MIX)
	{
		if (gpio_tlmm_config(GPIO_CFG(56, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
        	printk(KERN_ERR "[Touch] %s: gpio_tlmm_config: 56 failed.\n", __func__);
    	else
        	gpio_set_value(56, 0);
        
		msleep(1);
		
		if (chip_init())
		{
			printk(KERN_ERR "[Touch] %s: chip initial failed.\n", __func__);
			return;
		}
	}
    
    bu21018mwv->suspend_state = 0;
    I2C_ByteWrite(0xBB, 0x08);
    enable_irq(bu21018mwv->irq);
}
#endif

static const struct i2c_device_id bu21018mwv_id[] = {
    { "bu21018mwv", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, bu21018mwv_id);

static struct i2c_driver bu21018mwv_i2c_driver = {
   .driver = {
      .name   = "bu21018mwv",
   },
   .id_table   = bu21018mwv_id,
   .probe      = bu21018mwv_probe,
   .remove     = bu21018mwv_remove,
   .suspend    = bu21018mwv_suspend,
   .resume     = bu21018mwv_resume,
};

static int __init bu21018mwv_init( void )
{
    printk(KERN_INFO "[Touch] %s\n", __func__);
    return i2c_add_driver(&bu21018mwv_i2c_driver);
}

static void __exit bu21018mwv_exit( void )
{
    printk(KERN_INFO "[Touch] %s\n", __func__);
    i2c_del_driver(&bu21018mwv_i2c_driver);
}

module_init(bu21018mwv_init);
module_exit(bu21018mwv_exit);

MODULE_DESCRIPTION("BU21018MWV Touchscreen driver");
MODULE_AUTHOR("FIH Div2 SW2 BSP");
MODULE_LICENSE("GPL");
