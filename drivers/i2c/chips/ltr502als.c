/* FIH, NicoleWeng, 2010/03/09 { */
#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/sysctl.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h> 
#include <asm/ioctl.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
//#include <linux/irq.h>		//set_irq_type
#include "../../../arch/arm/mach-msm/smd_private.h"	//FIHTDC-Div2-SW2-BSP, Ming, HWID

#define CONFIGREG 	0x00
#define POWER_UP   	0 << 2
#define POWER_DOWN 	2 << 2
#define RESET		3 << 2
#define POWER_MASK  	0x0C
#define ACTIVE_DLS      0
#define ACTIVE_DPS      1
#define ACTIVE_DLPS     2
#define IDLE		3
#define ACTIVE_MASK 	0x03

#define TCREG     	0x01
#define DLSCTROL  	0x02
#define DPSCTROL	0x04
#define INTREG    	0x03
#define DATAREG   	0x05
#define DLSWINDOW	0x08

#define DLS_INT_MASK  	0x01
#define DPS_INT_MASK  	0x02

#define DPS_DATA_MASK 	0x80
#define DLS_DATA_MASK 	0x3F


/* IOCTL */
#define INACTIVE_PS		0
#define ACTIVE_PS		1
#define ACTIVE_LS		4
#define INACTIVE_LS		3
#define READ_LS			5
#define READ_PS			6
#define POWEROFF_CHIP		7
#define POWERON_CHIP		8
#define INIT_CHIP		9
#define SET_PS_THRESHOLD 	10
#define GET_PS_THRESHOLD 	11
#define SET_PS_ACCURACY		12
#define SET_LS_LEVEL		13
#define SET_LS_LOWTHRESH	14
#define SET_LS_PERSIST		15
#define SET_PS_PERSIST		16
#define SET_INTEGRA_TIME	17
#define ACTIVE_PHONE_CALL 	18
#define INACTIVE_PHONE_CALL 	19

struct ltr502als_platform_data{
    struct i2c_client *client;   
};

static struct i2c_client *ltr502als_client = NULL;

struct ltr502als_platform_data *ltr502als;
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
struct work_struct psensor_wake;//DIV5-BSP-CH-SF6-SENSOR-PORTING04++
#endif
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
static int power_use = 0;
static int ps_active = 0;
static int ls_active = 0;
int iPhoneCall = 0;
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
int psensor_control_flow = 0;
#endif
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[

//Div2D5-OH-Sensors-GPIO_Settings-00+{
#ifdef CONFIG_FIH_PROJECT_SF4V5
#define LTR520_ALS_INT 20
#endif
//Div2D5-OH-Sensors-GPIO_Settings-00+}

//Div2D5-OH-Sensors-SF6_GPIO_Settings-00+{
#ifdef CONFIG_FIH_PROJECT_SF4Y6
#define LTR520_ALS_INT 			49
//#define VREG_ALS_VLED_V2P8_EN	122 //Div2D5-OH-Sensors-SF6_GPIO_Settings-01-
#endif
//Div2D5-OH-Sensors-SF6_GPIO_Settings-00+}

//static struct vreg *verg_gp9_L12, *verg_gp7_L8, *verg_gp8_L20;

static int ltr502als_initchip(void);
extern void wake_up_p(void);//DIV5-BSP-CH-SF6-SENSOR-PORTING05++

#define _OWEN_
_OWEN_ struct early_suspend ALSPS_early_suspend;

//Div2D5-OwenHuang-BSP2030_FB0_Psensor_Phone_Call-00*{
#define DEBUG_LEVEL 1 
#if (DEBUG_LEVEL == 2) 
#define my_debug(flag, fmt, ...) \
	printk(flag pr_fmt(fmt), ##__VA_ARGS__)
#elif (DEBUG_LEVEL == 1)
#define my_debug(flag, fmt, ...) \
	if (strcmp(flag, KERN_NOTICE) == 0) \
		printk(flag pr_fmt(fmt), ##__VA_ARGS__)
#else
#define my_debug(flag, fmt, ...)
#endif
//Div2D5-OwenHuang-BSP2030_FB0_Psensor_Phone_Call-00*}

#define DRIVER_NAME	"[ltr502als]"
#define DBG(f, s, x...) \
	my_debug(f, DRIVER_NAME "%s: " s, __func__, ## x)

//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00+{
#if defined(CONFIG_FIH_PROJECT_FB400) || defined(CONFIG_FIH_PROJECT_SF4V5) //Div2D5-OwenHuang-BSP2030_SF5_Sensors_Porting-00*
static void ltr502als_active_ps(void);
static void ltr502als_active_ls(void);
static void ltr502als_inactive_ls(void);
static void ltr502als_inactive_ps(void);

//for yamah new driver architecture used
void LiteOn_enable_ls(void)
{
	ltr502als_active_ls();
}
EXPORT_SYMBOL(LiteOn_enable_ls);

void LiteOn_disable_ls(void)
{
	ltr502als_inactive_ls();
}
EXPORT_SYMBOL(LiteOn_disable_ls);

void LiteOn_enable_ps(void)
{
	ltr502als_active_ps();
}
EXPORT_SYMBOL(LiteOn_enable_ps);

void LiteOn_disable_ps(void)
{
	ltr502als_inactive_ps();
}
EXPORT_SYMBOL(LiteOn_disable_ps);
#endif
//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00+}

//Div2D5-OwenHuang-SF6_AKM8975C-Framework_Porting-01+{
#ifdef CONFIG_FIH_PROJECT_SF4Y6
int read_light_sensor(void)
{
	int val;
	val= i2c_smbus_read_byte_data(ltr502als->client, DATAREG);
	DBG(KERN_INFO, "READ_LS value=%d\n", (int)(val & DLS_DATA_MASK));
	return val >= 0 ? (int)(val & DLS_DATA_MASK) : (-1);
}
EXPORT_SYMBOL(read_light_sensor);

int read_proximity_sensor(void)
{
	int val;
	val= i2c_smbus_read_byte_data(ltr502als->client, DATAREG);
	DBG(KERN_INFO, "READ_PS value=%d\n", (int)(val & DPS_DATA_MASK)>>7);
	return val >= 0 ? (int)(val & DPS_DATA_MASK)>>7 : (-1);
}
EXPORT_SYMBOL(read_proximity_sensor);
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
int psensor_wake_control(void)
{
		return iPhoneCall;
}
EXPORT_SYMBOL(psensor_wake_control);
#endif
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
#endif
//Div2D5-OwenHuang-SF6_AKM8975C-Framework_Porting-01+}

static inline void ltr502als_poweroff(void)
{
	u8 val = -1;
	u8 ret = -1;
	
	if(power_use == 1)
	{		
		//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG );
		//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n", val);
		val = 0x8;
		ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val);
		DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n", ret, val);
		power_use--;
	}
	else
	{
		DBG(KERN_INFO, "power_use:%d\n", power_use);
	}	
}

static void ltr502als_poweron(void)
{
	u8 val = -1;
	u8 ret = -1;
			
	if(power_use == 0)
	{	
		//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG );
		//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
		//val = (val & (~POWER_MASK)) | POWER_UP;
		val = 0x2;
		ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val);
		DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val );
		power_use++;
	}
	else
	{
		DBG(KERN_INFO, "power_use:%d\n", power_use);
	}	
		
}

static void ltr502als_active_ps(void)
{
	u8 val = -1;
	u8 ret = -1;
	/*if (power_use == 0)
	{
		ltr502als_poweron();
	}*/
	if(ps_active == 0)
	{
		if(ls_active)
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG);
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
			//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
			#if defined(CONFIG_FIH_PROJECT_SF4Y6)
			val = 0x1;//0x2;disable dls as dps work.
			#else
			val = 0x2;
			#endif
			//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
			ret = i2c_smbus_write_byte_data(ltr502als->client, CONFIGREG, val);
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val);	
		}
		else
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG);
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
			val = 0x1;
			ret = i2c_smbus_write_byte_data(ltr502als->client, CONFIGREG, val);
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val);	
		}
		ps_active++;
	}
	else
	{
		DBG(KERN_INFO, "ps_active:%d\n", ps_active);
	}	
	
}
static void ltr502als_active_ls(void)
{
	u8 val = -1;
	u8 ret = -1;
	/*if (power_use == 0)
	{
		ltr502als_poweron();
	}*/
	if(ls_active == 0)
	{
		if(ps_active)
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG );
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n", val);
			//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
			#if defined(CONFIG_FIH_PROJECT_SF4Y6)
			if(iPhoneCall)
				val = 0x1;
			else val = 0x2;
			#else
			val = 0x2;
			#endif
			//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
			ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val );
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val );
		}
		else
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG );
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
			val = 0x0;
			ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val);
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val );	
		}
		/* FIH, Louis, 2010/08/13 { */
		//mdelay(700);
		ls_active++;
	}
	else
	{
		DBG(KERN_INFO, "ls_active:%d\n", ls_active);
	}	
}

static void ltr502als_inactive_ls(void)
{
	u8 val = -1;
	u8 ret = -1;	
	if(ls_active==1)
	{
		if(ps_active)
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG );
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
			val = 0x1;
			ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val );
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val );		
		}
		else
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG );
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
			val = 0x8;
			ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val);
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val );	
		}
		ls_active--;
	}
	else
	{
		DBG(KERN_INFO, "ls_active:%d\n", ls_active);
	}	
} 

static void ltr502als_inactive_ps(void)
{
	u8 val = -1;
	u8 ret = -1;

	if(ps_active==1)
	{
		if(ls_active)
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG );
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
			val = 0x0;
			ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val);
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val);		
		}
		else
		{
			//val = i2c_smbus_read_byte_data( ltr502als->client, CONFIGREG);
			//DBG(KERN_INFO, "(READ)(CONFIG REG ret:0x%x)\n",val);
			val = 0x8;
			ret = i2c_smbus_write_byte_data(ltr502als->client,CONFIGREG, val);
			DBG(KERN_INFO, "(WRITE)(CONFIG REG ret:0x%x, val:0x%x)\n",ret, val);	
		}
		ps_active--;
	}
	else
	{
		DBG(KERN_INFO, " ps_active:%d\n", ps_active);
	}
	
} 
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
static void psensor_work_queue(struct work_struct *work)
{
	if(psensor_control_flow)
	{
		printk(KERN_INFO "[Colin]control\n");
		//if(read_proximity_sensor() == 0)
		{
			printk(KERN_INFO "[Colin]wakeup\n");
			wake_up_p();
		}
	}
}
#endif
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
//Proximitary_interrupt_handler
static irqreturn_t ltr502als_isr(int irq, void *dev_id)
{
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
	if(iPhoneCall == 1)
	{
		schedule_work(&psensor_wake);
	}
	DBG(KERN_NOTICE, "\n");
#endif
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
	//set_irq_type(gpio_to_irq(49), gpio_get_value(49) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING);
#if 0	
    	u8 val_INT = -1;
    	u8 val = -1;
	val_INT = i2c_smbus_read_byte_data(ltr502als->client, INTREG);
    	printk("[ltr502als](INTREG:0x%x)\n", val_INT);	 

	if(val_INT & DPS_INT_MASK)
    	{
		//read ps		
		val= i2c_smbus_read_byte_data(ltr502als->client, DATAREG);
		printk("[ltr502als](DATA:0x%x)\n",val);

		val = (val & DPS_DATA_MASK)>>7;
		printk("[ltr502als](DPS_DATA:0x%x)\n",val);
	}
	if(val_INT & DLS_INT_MASK)
	{
		//read as		
		val= i2c_smbus_read_byte_data(ltr502als->client, DATAREG);	
		printk("[ltr502als](DATA:0x%x)\n",val);
		val = val & DLS_DATA_MASK;
		printk("[ltr502als](DLS_DATA:0x%x)\n",val);
	}	
#endif  
	DBG(KERN_NOTICE, "\n");
	return IRQ_HANDLED;
}

/* 

initial ltr502als chip, and check ltr502als is available

1. Power on ltr502als.
2. Reset ltr502als, RSTN signal should be set "H->L->H".
3. CPU can access to register of ltr502als.
    Here is one way for confirmation of I2C device driver operation.
	3-1. A unique value is written into HXDA register (0xE1h) via I2C.
	3-2. Read out HXDA register (0xE1h) via I2C.
	3-3. Compare 3-1 data and 3-2 data.
	    These data are same, I2C function works well.

*/


static int ltr502als_initchip(void)
{
	u8 val = -1;
  	int ret = -1;
  	int Command = 0;
	
	ret = i2c_smbus_read_byte_data(ltr502als->client, CONFIGREG);
	if(ret < 0)
	{
		printk(KERN_ERR "[ltr502als]%s: light-sensor fail, value=%d\n", __func__, ret);
		return -1;
	}	
	else
		DBG(KERN_INFO, "light-sensor addr:0x%x value=%d\n", CONFIGREG, ret);
		
	Command = 0x10;
  	ret = i2c_smbus_write_byte_data(ltr502als->client, TCREG, Command);
	DBG(KERN_INFO, "write TCREG(ret:%d)\n",ret) ;

	val = i2c_smbus_read_byte_data(ltr502als->client, TCREG);
	DBG(KERN_INFO, "read TCREG:0x%x\n",val);

	                    	 
	val = i2c_smbus_read_byte_data(ltr502als->client, DPSCTROL);

	if ((fih_get_product_id()== Product_FB0 || fih_get_product_id()== Product_FD1) && (fih_get_product_phase()==Product_PR2p5 || fih_get_product_phase()==Product_PR230 || fih_get_product_phase()==Product_PR231))
	{			
		Command = (val & 0xC0) | 0x1e;	//change proximity threshold to 31
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
		DBG(KERN_NOTICE, "Proximity Setting for FBX(Threshold is 0x%X)\n", Command);
	}
//Div2D5-OwenHuang-FB0_Fine_Tune_Proximity_Threshold-00+{
	else if ((fih_get_product_id() == Product_FB0 || fih_get_product_id() == Product_FD1) && (fih_get_product_phase() >= Product_PR232))
	{
		Command = (val & 0xC0) | 0x17;	//change proximity threshold to 23
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
		DBG(KERN_NOTICE, "Proximity Setting for FBX(Threshold is 0x%X)\n", Command);
	}
	else if (fih_get_product_id() == Product_FB1)
	{
		Command = (val & 0xC0) | 0x17;	//change proximity threshold to 23
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
		DBG(KERN_NOTICE, "Proximity Setting for FB1(Threshold is 0x%X)\n", Command);
	}
	else if (fih_get_product_id() == Product_FB3)
	{
		Command = (val & 0xC0) | 0x17;	//change proximity threshold to 23
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
		DBG(KERN_NOTICE, "Proximity Setting for FB3(Threshold is 0x%X)\n", Command);
	}
//Div2D5-OwenHuang-FB0_Fine_Tune_Proximity_Threshold-00+}
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
	else if ((fih_get_product_id()==Product_SF6) && (fih_get_product_phase()==Product_PR2))
	{
		Command = (val & 0xC0) | 0x14;	//change proximity threshold to 20
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
	}
	else if ((fih_get_product_id()==Product_SF6) && (fih_get_product_phase()==Product_PR3))
	{
		Command = (val & 0xC0) | 0x14;	//change proximity threshold to 20
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
	}
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
//Div2D5-OwenHuang-BSP2030_SF5_Proximity_Threshold-00+{
	else if (fih_get_product_id() == Product_SF5)
	{
		Command = (val & 0xC0) | 0x15;	//change proximity threshold to 21
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
		DBG(KERN_NOTICE, "Proximity Setting for SF5(Threshold is 0x%X)\n", Command);
	}
//Div2D5-OwenHuang-BSP2030_SF5_Proximity_Threshold-00+}
	else
	{
		Command = (val & 0xC0) | 0x17;	//change proximity threshold to 23
		ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);	
	}
	return 0;
}



static int ltr502als_open(struct inode *inode, struct file *file)
{
	DBG(KERN_INFO, "\n");
	return 0;
}

static ssize_t ltr502als_read(struct file *file, char __user *buffer, size_t size, loff_t *f_ops)
{	
	char *st;
	//char st[2];
	u8 value;
	DBG(KERN_INFO, "\n");
	st=kmalloc(sizeof(char)*2,GFP_KERNEL);		 
	value= i2c_smbus_read_byte_data(ltr502als->client, DATAREG);
	st[1] = (value & DPS_DATA_MASK)>>7;
	st[0] = value & DLS_DATA_MASK;

	DBG(KERN_INFO, "ltr502als_read: PS level = %d\n", st[1]);	
	DBG(KERN_INFO, "ltr502als_read: ALS level = %d\n", st[0]);
	if(copy_to_user(buffer, st, sizeof(char)*2))
	{
		kfree(st);
		printk(KERN_ERR "[ltr502als]%s: [line:%d] copy_to_user failed\n", __func__, __LINE__);
		return -EFAULT;
	}
	kfree(st);
	return 0; 
}

static ssize_t ltr502als_write(struct file *file, const char __user *buffer, size_t size, loff_t *f_ops)
{
	DBG(KERN_INFO, "\n");
	return 0;
}

static int ltr502als_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	u8 val;
    int Command = 0;
    void __user *argp = (void __user *)arg;
    int value;

	DBG(KERN_INFO, "cmd(%d)\n", cmd);
	switch (cmd)
	{
		case INIT_CHIP:			
			ret = ltr502als_initchip();
			DBG(KERN_INFO, "INIT_CHIP ret=%d\n", ret);
			return ret;
		case POWEROFF_CHIP:
			ltr502als_poweroff();
			DBG(KERN_INFO, "POWEROFF_CHIP\n");
			break;
		case POWERON_CHIP:
			ltr502als_poweron();
			DBG(KERN_INFO, "POWERON_CHIP\n");
			break;
		case ACTIVE_PS:
			ltr502als_active_ps();
			DBG(KERN_INFO, "ACTIVE_PS\n");
			break;
		case ACTIVE_LS:
			ltr502als_active_ls();
			DBG(KERN_INFO, "ACTIVE_LS\n");
			break;
		case INACTIVE_PS:
			ltr502als_inactive_ps();
			DBG(KERN_INFO, "INACTIVE_PS\n");
			break;
		case INACTIVE_LS:
			ltr502als_inactive_ls();
			DBG(KERN_INFO, "INACTIVE_LS\n");
			break;
		case READ_PS:
			val= i2c_smbus_read_byte_data(ltr502als->client, DATAREG);
			DBG(KERN_INFO, "READ_PS value=%d\n", (int)(val & DPS_DATA_MASK)>>7);
			//printk(KERN_INFO "READ_PS value=%d\n", (int)(val & DPS_DATA_MASK)>>7);
			return (int)(val & DPS_DATA_MASK)>>7;
		case READ_LS:
			val= i2c_smbus_read_byte_data(ltr502als->client, DATAREG);
			DBG(KERN_INFO, "READ_LS value=%d\n", (int)(val & DLS_DATA_MASK));
			return (int)(val & DLS_DATA_MASK);
        case SET_PS_THRESHOLD:
        {                	
			if(copy_from_user(&value, argp, sizeof(value)))	
				value = (unsigned int)arg;
	                    	 
			val = i2c_smbus_read_byte_data(ltr502als->client, DPSCTROL);
			
			Command = (val & 0xC0) | value;
			DBG(KERN_INFO, "SET_PS_THRESHOLD addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", DPSCTROL, value, val, Command); 
			ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command);
			break;
        }
        case GET_PS_THRESHOLD:
        {
			val = i2c_smbus_read_byte_data(ltr502als->client, DPSCTROL);
			DBG(KERN_INFO, "GET_PS_THRESHOLD addr:0x%x, buf[0]:0x%x\n", DPSCTROL, val);  
            return (int)(val & 0x1f);
        }
		case SET_PS_ACCURACY:
		{
			if(copy_from_user(&value, argp, sizeof(value)))
                    		value = (unsigned int)arg; 			
			value = value << 6;

			val = i2c_smbus_read_byte_data(ltr502als->client, DPSCTROL);

            Command = (val & 0x1f) | value;
            DBG(KERN_INFO, "SET_PS_ACCURACY addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", DPSCTROL, value, val, Command); 
			ret = i2c_smbus_write_byte_data(ltr502als->client, DPSCTROL, Command); 
			break;	
		}
		case SET_LS_LEVEL:
		{
			if(copy_from_user(&value, argp, sizeof(value)))
                    		value = (unsigned int)arg;			
			value = value << 5;

			val = i2c_smbus_read_byte_data(ltr502als->client, DLSCTROL);

			Command = (val & 0x1f) | value;
			DBG(KERN_INFO, "SET_LS_LEVEL addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", DLSCTROL, value, val, Command); 
    		ret = i2c_smbus_write_byte_data(ltr502als->client, DLSCTROL, Command);
			break;	
		}
		case SET_LS_LOWTHRESH:
		{
			if(copy_from_user(&value, argp, sizeof(value)))
                    		value = (unsigned int)arg; 			

			val = i2c_smbus_read_byte_data(ltr502als->client, DLSCTROL);

			Command = (val & 0xe0) | value;
			DBG(KERN_INFO, "SET_LS_LOWTHRESH addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", DLSCTROL, value, val, Command); 
    			ret = i2c_smbus_write_byte_data(ltr502als->client, DLSCTROL, Command);
			break;	
		}
		case SET_PS_PERSIST:
		{
			if(copy_from_user(&value, argp, sizeof(value)))
                    		value = (unsigned int)arg; 			
			value = value << 4;

			val = i2c_smbus_read_byte_data(ltr502als->client, TCREG);

			Command = (val & 0x07) | value;
			DBG(KERN_INFO, "SET_PS_PERSIST addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", TCREG, value, val, Command); 
    			ret = i2c_smbus_write_byte_data(ltr502als->client, TCREG, Command);
			break;
		}
		case SET_LS_PERSIST:
		{
			if(copy_from_user(&value, argp, sizeof(value)))
                    		value = (unsigned int)arg;			

			val = i2c_smbus_read_byte_data(ltr502als->client, TCREG);

			Command = (val & 0x34) | value;
			DBG(KERN_INFO, "SET_LS_PERSIST addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", TCREG, value, val, Command); 
    			ret = i2c_smbus_write_byte_data(ltr502als->client, TCREG, Command);
			break;
		}
		case SET_INTEGRA_TIME:
		{
			if(copy_from_user(&value, argp, sizeof(value)))
                    		value = (unsigned int)arg;			
			value = value << 2;

			val = i2c_smbus_read_byte_data(ltr502als->client, TCREG);

			Command = (val & 0x33) | value;
			DBG(KERN_INFO, "SET_INTEGRA_TIME addr:0x%x, value:%d, buf[0]:0x%x, buf[1]:0x%x\n", TCREG, value, val, Command); 
    			ret = i2c_smbus_write_byte_data(ltr502als->client, TCREG, Command);
			break;
		}
		case ACTIVE_PHONE_CALL:	
			
		//Div2D5-OwenHuang-FBx_Psensor_Pending_IRQ-00+{
		#ifdef CONFIG_NEW_YAMAHA_SENSORS
			//ltr502als_inactive_ls();
			Command = 0x07; //light senosr trigger time is 3.2 sec, proximity sensor 200ms
  			ret = i2c_smbus_write_byte_data(ltr502als->client, TCREG, Command);
			if (ret)
			{
				DBG(KERN_NOTICE, "i2c failed\n");
			}
		#endif
		//Div2D5-OwenHuang-FBx_Psensor_Pending_IRQ-00+}	
		
			enable_irq(ltr502als->client->irq);
			enable_irq_wake(ltr502als->client->irq);
			iPhoneCall = 1;
			//printk(KERN_INFO "[ltr502als]ACTIVE_PHONE_CALL\n");
			DBG(KERN_NOTICE, "ACTIVE_PHONE_CALL\n");
			break;
		case INACTIVE_PHONE_CALL:
			disable_irq_wake(ltr502als->client->irq);
			disable_irq(ltr502als->client->irq);
			iPhoneCall = 0;
			
		//Div2D5-OwenHuang-FBx_Psensor_Pending_IRQ-00+{
		#ifdef CONFIG_NEW_YAMAHA_SENSORS
			Command = 0x10; //light senosr trigger time is 100ms, proximity sensor 200ms
  			ret = i2c_smbus_write_byte_data(ltr502als->client, TCREG, Command);
			if (ret)
			{
				DBG(KERN_NOTICE, "i2c failed\n");
			}
			//ltr502als_active_ls();
		#endif
		//Div2D5-OwenHuang-FBx_Psensor_Pending_IRQ-00+}
		
			//printk(KERN_INFO "[ltr502als]INACTIVE_PHONE_CALL\n");
			DBG(KERN_NOTICE, "INACTIVE_PHONE_CALL\n");
			break;		
		default:
			DBG(KERN_INFO, "Proximity sensor ioctl default : NO ACTION!!\n");
			break;
	}

	return 0;
}

static int ltr502als_release(struct inode *inode, struct file *filp)
{
    DBG(KERN_INFO, "\n");
	return 0;
}

static const struct file_operations ltr502als_dev_fops = {
	.owner = THIS_MODULE,
	.open = ltr502als_open,
	.read = ltr502als_read,
	.write = ltr502als_write,
	.ioctl = ltr502als_ioctl,
	.release = ltr502als_release,
};
static struct miscdevice ltr502als_dev =
{
        .minor = MISC_DYNAMIC_MINOR,
        .name = "ltr502als_alsps",
	.fops = &ltr502als_dev_fops,
};

static int ltr502als_remove(struct i2c_client *client)
{
	int irq;

	if((irq = client->irq)>0)
		free_irq(irq, ltr502als);
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
	#if defined(CONFIG_FIH_PROJECT_SF4Y6)
	flush_work(&psensor_wake);
	#endif
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]

	kfree(ltr502als);
	i2c_set_clientdata(client, NULL);
	return 0;
}
_OWEN_ static void ALSPS_early_suspend_func(struct early_suspend * h)
{	
	if(iPhoneCall == 0)
	{
		int ret = -1;
		u8 val;
    		int Command = 0;
 
		val = i2c_smbus_read_byte_data(ltr502als->client, CONFIGREG);
		Command = (val & 0xf3) | 0x08;	//power down mode
		
		ret = i2c_smbus_write_byte_data(ltr502als->client, CONFIGREG, Command);
		DBG(KERN_INFO, "addr:0x%x, value:0x%x, command:0x%x, return:%d\n", CONFIGREG, val, Command, ret);
	}
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
	else
	{
		DBG(KERN_INFO, "[SF6]early_suspend control flow\n");
		psensor_control_flow = 1;
	}
#endif
//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
	return;
}
_OWEN_ static void ALSPS_late_resume_func(struct early_suspend *h)
{
	if(iPhoneCall == 0)
	{
		int ret = -1;
    		int Command = 0;
    
		DBG(KERN_INFO, "\n");

		if(ps_active && ls_active) {
			Command = 0x02;
		
			ret = i2c_smbus_write_byte_data(ltr502als->client, CONFIGREG, Command);
			DBG(KERN_INFO, "addr:0x%x, command:0x%x, return:%d\n", CONFIGREG, Command, ret);		
		}
		else if(ps_active && !ls_active) {
			Command = 0x01;
		
			ret = i2c_smbus_write_byte_data(ltr502als->client, CONFIGREG, Command);
			DBG(KERN_INFO, "addr:0x%x, command:0x%x, return:%d\n", CONFIGREG, Command, ret);		
		}
		else if(!ps_active && ls_active) {
			Command = 0x00;
		
			ret = i2c_smbus_write_byte_data(ltr502als->client, CONFIGREG, Command);
			DBG(KERN_INFO, "addr:0x%x, command:0x%x, return:%d\n", CONFIGREG, Command, ret);		
		}
		else {
			Command = 0x08;
		
			ret = i2c_smbus_write_byte_data(ltr502als->client, CONFIGREG, Command);
			DBG(KERN_INFO, "addr:0x%x, command:0x%x, return:%d\n", CONFIGREG, Command, ret);	
		}
	}
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
	#if defined(CONFIG_FIH_PROJECT_SF4Y6)
	else 
	{
		DBG(KERN_INFO, "[SF6]late resume control flow\n");
		psensor_control_flow = 0;
	}
	#endif
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
	return;		
}
/*static int ltr502als_suspend(struct device *dev)
{	
	return 0;
}

static int ltr502als_resume(struct device *dev)
{
	return 0;
}*/

static int ltr502als_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int irq = 0, ret = -EINVAL, rc;	
	DBG(KERN_INFO, "\n");
	//DBG(KERN_INFO, "(addr:0x%x)\n",client->addr);
	//DBG(KERN_INFO, "(name:%s)\n",client->name);
	//DBG(KERN_INFO, "(irq:%d)\n",client->irq);

	/*verg_gp7_L8 = vreg_get(NULL, "gp7");
	if (IS_ERR(verg_gp7_L8)) {
		printk(KERN_INFO "[ltr502als]%s: gp7 vreg get failed (%ld)\n", __func__, PTR_ERR(verg_gp7_L8));
	}	
	
	verg_gp9_L12 = vreg_get(NULL, "gp9");
	if (IS_ERR(verg_gp9_L12)) {
		printk(KERN_INFO "[ltr502als]%s: gp9 vreg get failed (%ld)\n", __func__, PTR_ERR(verg_gp9_L12));
	}

	verg_gp8_L20 = vreg_get(NULL, "gp13");
	if (IS_ERR(verg_gp8_L20)) {
		printk(KERN_INFO "[ltr502als]%s: gp8 vreg get failed (%ld)\n", __func__, PTR_ERR(verg_gp8_L20));
	}
	

	mdelay(400);
	
	rc = vreg_set_level(verg_gp7_L8, 0);
	if (rc) {
		printk(KERN_INFO "[ltr502als]%s: vreg LDO8 set level failed (%d)\n", __func__, rc);
	}	

	rc = vreg_set_level(verg_gp9_L12, 0);
	if (rc) {
		printk("[ltr502als]%s: vreg LDO12 set level failed (%d)\n", __func__, rc);
	}

	rc = vreg_set_level(verg_gp8_L20, 0);	
	if (rc) {
		printk(KERN_INFO "[ltr502als]%s: vreg LDO20 set level failed (%d)\n", __func__, rc);
	}

	vreg_disable(verg_gp7_L8);
	vreg_disable(verg_gp9_L12);
	vreg_disable(verg_gp8_L20);

	mdelay(400);


	rc = vreg_set_level(verg_gp7_L8, 1800);
	if (rc) {
		printk(KERN_INFO "[ltr502als]%s: vreg LDO8 set level failed (%d)\n", __func__, rc);
	}	

	rc = vreg_set_level(verg_gp9_L12, 3000);
	if (rc) {
		printk(KERN_INFO "[ltr502als]%s: vreg LDO12 set level failed (%d)\n", __func__, rc);
	}
	
	rc = vreg_enable(verg_gp7_L8);
	if (rc) {
		printk(KERN_INFO "[ltr502als]%s: LDO8 vreg enable failed (%d)\n", __func__, rc);
	}	

	rc = vreg_enable(verg_gp9_L12);
	if (rc) {
		printk(KERN_INFO "[ltr502als]%s: LDO12 vreg enable failed (%d)\n", __func__, rc);
	}	

	mdelay(400);
*/

//Div2D5-OH-Sensors-GPIO_Settings-00+{
#ifdef CONFIG_FIH_PROJECT_SF4V5
	rc = gpio_request(LTR520_ALS_INT, "als_int"); 
	if (rc)
	{
		printk(KERN_ERR "[LTR502ALS] Request gpio failed!\n");
		return -EFAULT;
	}
	
	rc = gpio_tlmm_config(GPIO_CFG(LTR520_ALS_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	if (rc)
	{
		printk(KERN_ERR "[LTR502ALS] Configure gpio failed!\n");
		return -EFAULT;
	}

	gpio_direction_input(LTR520_ALS_INT);
//Div2D5-OH-Sensors-SF6_GPIO_Settings-00+{
#elif defined(CONFIG_FIH_PROJECT_SF4Y6)
	
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
	//Init work queue for psensor
	INIT_WORK(&psensor_wake,psensor_work_queue);
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
	//configure INT GPIO
	rc = gpio_request(LTR520_ALS_INT, "als_int"); 
	if (rc)
	{
		printk(KERN_ERR "[LTR502ALS] Request gpio LTR520_ALS_INT failed!\n");
		return -EFAULT;
	}
	
	rc = gpio_tlmm_config(GPIO_CFG(LTR520_ALS_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);//DIV5-BSP-CH-SF6-SENSOR-PORTING00++
	if (rc)
	{
		printk(KERN_ERR "[LTR502ALS] Configure gpio LTR520_ALS_INT failed!\n");
		return -EFAULT;
	}

	gpio_direction_input(LTR520_ALS_INT);
	//Div2D5-OH-Sensors-SF6_GPIO_Settings-01-{
	/*
	//configure enable pin GPIO122
	rc = gpio_request(VREG_ALS_VLED_V2P8_EN, "ALS_EN");
	if (rc)
	{
		printk(KERN_ERR "[LTR502ALS] Request gpio VREG_ALS_VLED_V2P8_EN failed!\n");
		return -EFAULT;
	}

	rc = gpio_tlmm_config(GPIO_CFG(VREG_ALS_VLED_V2P8_EN, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA),
		GPIO_ENABLE);
	if (rc)
	{
		printk(KERN_ERR "[LTR502ALS] Configure gpio VREG_ALS_VLED_V2P8_EN failed!\n");
		return -EFAULT;
	}
	
	gpio_set_value(VREG_ALS_VLED_V2P8_EN, 1); //enable pin pull high
	*/
	//Div2D5-OH-Sensors-SF6_GPIO_Settings-01-}
//Div2D5-OH-Sensors-SF6_GPIO_Settings-00+}
#else
	rc = gpio_tlmm_config(GPIO_CFG(49, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);

	gpio_direction_input(49);
#endif
//Div2D5-OH-Sensors-GPIO_Settings-00+}

	ltr502als_client = client;
	
	DBG(KERN_INFO, "i2c addr:%d, adapter:%d\n", (int)ltr502als_client->addr, (int)ltr502als_client->adapter);

	/* allocate memory */
	ltr502als = kzalloc(sizeof(struct ltr502als_platform_data), GFP_KERNEL);
	if (!ltr502als)
	{
		printk(KERN_ERR "[ltr502als]%s: error\n", __func__);
		return -ENOMEM;
	}
	memset(ltr502als, 0, sizeof(struct ltr502als_platform_data));

	ltr502als->client = client;
	dev_set_drvdata (&client->dev, ltr502als);
	

	if (0 != misc_register(&ltr502als_dev))
	{
		printk(KERN_ERR "[ltr502als]%s: ltr502als_dev register failed.\n", __func__);
		return 0;
	}
	else
	{
		DBG(KERN_INFO, "ltr502als_dev register ok.\n");
	}
	
	ret = request_irq(client->irq, ltr502als_isr, IRQF_TRIGGER_FALLING, "ltr502als", ltr502als); 
	if (ret) 
	{
		printk(KERN_ERR "[ltr502als]%s: Can't allocate irq %d\n", __func__, client->irq);
		goto fail_irq;
	}
	DBG(KERN_INFO, "[line:%d] Requst PRO_INT pin client->irq:%d\n", __LINE__ , client->irq);
	
	disable_irq(client->irq);
	mdelay(200);
	ret = ltr502als_initchip();
	if(ret < 0)
	{
		printk(KERN_ERR "[ltr502als]%s: ltr502als_initchip failed.\n", __func__);
		//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00*{
		//return -1;
		ret = -EIO;
		goto fail_init_chip;
		//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00*}
	}
	ALSPS_early_suspend.level = EARLY_SUSPEND_LEVEL_LTR502ALS_DRV;
	ALSPS_early_suspend.suspend = ALSPS_early_suspend_func;
	ALSPS_early_suspend.resume = ALSPS_late_resume_func;
	register_early_suspend(&ALSPS_early_suspend);

	printk(KERN_INFO "[ltr502als]%s: probe result(%d)\n", __func__, ret); ///Div2D5-OwenHuang-BSP2030_FB0_FQC_ALS-00+
	
	return ret;

//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00+{
fail_init_chip:
	misc_deregister(&ltr502als_dev);
//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00+}
fail_irq:
	free_irq(irq, ltr502als);
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++[
	#if defined(CONFIG_FIH_PROJECT_SF4Y6)
	flush_work(&psensor_wake);
	#endif
	//DIV5-BSP-CH-SF6-SENSOR-PORTING04++]
	return ret;

}

static const struct i2c_device_id ltr502als_id[] = {
	{ "ltr502als", 0 }, 
	{ }
};

/*struct dev_pm_ops ltr502als_driver_pm = {
     .suspend	= ltr502als_suspend,
     .resume	= ltr502als_resume,
};*/

static struct i2c_driver ltr502als_driver = {
	.probe 		= ltr502als_probe,
	.remove 	= ltr502als_remove,
	//.suspend	= ltr502als_suspend,
	//.resume		= ltr502als_resume,
	.id_table 	= ltr502als_id,
	.driver = {
		.name = "ltr502als",
		//.pm = &ltr502als_driver_pm, 
	},
};

static int __init ltr502als_i2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&ltr502als_driver);
	if(ret) 
        	printk(KERN_ERR "[ltr502als]%s: Driver registration failed\n", __func__);
	else
		DBG(KERN_INFO, "ltr502als add I2C driver ok\n");

	return ret;
}

static void __exit ltr502als_i2c_exit(void)
{
	i2c_del_driver(&ltr502als_driver);	
	misc_deregister(&ltr502als_dev);
	
}


module_init(ltr502als_i2c_init);
module_exit(ltr502als_i2c_exit);

MODULE_DESCRIPTION("ltr502als_i2c Sennsor driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NicoleWeng");
/*} FIH, NicoleWeng, 2010/03/09  */


