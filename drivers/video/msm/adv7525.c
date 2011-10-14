/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
 */

#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/adv7525.h>
#include <linux/time.h>
#include "msm_fb.h"

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#include <linux/earlysuspend.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <mach/gpio.h>
#if DEBUG
#define VIBRATOR_FILE "/sys/class/timed_output/vibrator/enable"
#endif
#define ENABLE_WAKEUP 0
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */

static struct i2c_client *hclient;
static struct i2c_client *eclient;

struct msm_panel_info *pinfo;
static bool power_on = FALSE;	/* For power on/off */

static u8 reg[256];	/* HDMI panel registers */
#ifndef CONFIG_FIH_FTM
static u8 ereg[256];	/* EDID Memory  */
#endif

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#define WIDTH_720P  1280
#define HEIGHT_720P 720
#define WIDTH_VGA   640
#define HEIGHT_VGA  480
#define MAX_EDID_TRIES 5
#define MAX_SUPPROT_MODES 5
int fqc_hpd_state = 0;
int extension_screen_resolution = 0x0000;    ///Over scanned 0%
char b_vendor_id[5], current_vendor_id[5];
uint32 video_format;
#ifndef CONFIG_FIH_FTM
static bool dsp_mode_found = FALSE;
#endif
static bool read_edid = TRUE;
static bool hdmi_dvi_mode = FALSE; ///TRUE:DVI FALSE:HDMI
extern bool hdmi_online;
static int plugin_counter= 0;
static int change_hdmi_counter = 0;
bool chip_power_on = FALSE;
#ifdef ENABLE_WAKEUP
static int hdmi_wakeup_enabled;
#endif
static int clk_enable_counter = 0;
int	hdmi_support_mode_lut[MAX_SUPPROT_MODES] = {HDMI_VFRMT_1280x720p60_16_9,
    HDMI_VFRMT_1280x720p50_16_9,HDMI_VFRMT_720x480p60_16_9,
    HDMI_VFRMT_720x576p50_16_9,HDMI_VFRMT_640x480p60_4_3};    
    
static struct clk *tv_enc_clk;
static struct clk *tv_dac_clk;
static struct clk *hdmi_clk;    
static struct clk *mdp_tv_clk;
static void hdmi_default_video_format(void);


#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
struct proc_dir_entry *hdmi_hdcp;
static struct work_struct hdcp_handle_work;
uint32 enable_hdcp = 1;  ///0:disable HDCP 1:enable HDCP;
static int hdcp_started;
static int hdcp_active;
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
struct hdmi_data {
	struct msm_hdmi_platform_data *pd;
	int (*intr_detect)(void);
	void (*setup_int_power)(int);			
	struct work_struct work;
};
static struct hdmi_data *dd;
static struct work_struct handle_work;

/* static int online ; */
static struct timer_list hpd_timer;
static unsigned int monitor_plugin;

/* EDID variables */
static struct kobject *hdmi_edid_kobj;

static int horizontal_resolution = WIDTH_720P ;
static int vertical_resolution = HEIGHT_720P;
#ifndef CONFIG_FIH_FTM
static struct msm_fb_data_type *mfd;
#endif

/* HDMI hotplug detection using kobject */
static struct hdmi_state *hdmi_state_obj;
struct proc_dir_entry *ent, *ov_screen;

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
/* LUT is sorted from lowest Active H to highest Active H - ease searching */
#ifndef CONFIG_FIH_FTM
static struct hdmi_edid_video_mode_property_type
	hdmi_edid_disp_mode_lut[] = {

	/* All 640 H Active */
	{HDMI_VFRMT_640x480p60_4_3, 640, 480, FALSE, 800, 160, 525, 45,
	 31465, 59940, 25175, 59940, TRUE},
	{HDMI_VFRMT_640x480p60_4_3, 640, 480, FALSE, 800, 160, 525, 45,
	 31500, 60000, 25200, 60000, TRUE},

	/* All 720 H Active */
	{HDMI_VFRMT_720x576p50_4_3,  720, 576, FALSE, 864, 144, 625, 49,
	 31250, 50000, 27000, 50000, TRUE},
	{HDMI_VFRMT_720x480p60_4_3,  720, 480, FALSE, 858, 138, 525, 45,
	 31465, 59940, 27000, 59940, TRUE},
	{HDMI_VFRMT_720x480p60_4_3,  720, 480, FALSE, 858, 138, 525, 45,
	 31500, 60000, 27030, 60000, TRUE},
	{HDMI_VFRMT_720x576p100_4_3, 720, 576, FALSE, 864, 144, 625, 49,
	 62500, 100000, 54000, 100000, TRUE},
	{HDMI_VFRMT_720x480p120_4_3, 720, 480, FALSE, 858, 138, 525, 45,
	 62937, 119880, 54000, 119880, TRUE},
	{HDMI_VFRMT_720x480p120_4_3, 720, 480, FALSE, 858, 138, 525, 45,
	 63000, 120000, 54054, 120000, TRUE},
	{HDMI_VFRMT_720x576p200_4_3, 720, 576, FALSE, 864, 144, 625, 49,
	 125000, 200000, 108000, 200000, TRUE},
	{HDMI_VFRMT_720x480p240_4_3, 720, 480, FALSE, 858, 138, 525, 45,
	 125874, 239760, 108000, 239000, TRUE},
	{HDMI_VFRMT_720x480p240_4_3, 720, 480, FALSE, 858, 138, 525, 45,
	 126000, 240000, 108108, 240000, TRUE},

	/* All 1280 H Active */
	{HDMI_VFRMT_1280x720p50_16_9,  1280, 720, FALSE, 1980, 700, 750, 30,
	 37500, 50000, 74250, 50000, FALSE},
	{HDMI_VFRMT_1280x720p60_16_9,  1280, 720, FALSE, 1650, 370, 750, 30,
	 44955, 59940, 74176, 59940, FALSE},
	{HDMI_VFRMT_1280x720p60_16_9,  1280, 720, FALSE, 1650, 370, 750, 30,
	 45000, 60000, 74250, 60000, FALSE},
	{HDMI_VFRMT_1280x720p100_16_9, 1280, 720, FALSE, 1980, 700, 750, 30,
	 75000, 100000, 148500, 100000, FALSE},
	{HDMI_VFRMT_1280x720p120_16_9, 1280, 720, FALSE, 1650, 370, 750, 30,
	 89909, 119880, 148352, 119880, FALSE},
	{HDMI_VFRMT_1280x720p120_16_9, 1280, 720, FALSE, 1650, 370, 750, 30,
	 90000, 120000, 148500, 120000, FALSE},

	/* All 1440 H Active */
	{HDMI_VFRMT_1440x576i50_4_3, 1440, 576, TRUE,  1728, 288, 625, 24,
	 15625, 50000, 27000, 50000, TRUE},
	{HDMI_VFRMT_720x288p50_4_3,  1440, 288, FALSE, 1728, 288, 312, 24,
	 15625, 50080, 27000, 50000, TRUE},
	{HDMI_VFRMT_720x288p50_4_3,  1440, 288, FALSE, 1728, 288, 313, 25,
	 15625, 49920, 27000, 50000, TRUE},
	{HDMI_VFRMT_720x288p50_4_3,  1440, 288, FALSE, 1728, 288, 314, 26,
	 15625, 49761, 27000, 50000, TRUE},
	{HDMI_VFRMT_1440x576p50_4_3, 1440, 576, FALSE, 1728, 288, 625, 49,
	 31250, 50000, 54000, 50000, TRUE},
	{HDMI_VFRMT_1440x480i60_4_3, 1440, 480, TRUE,  1716, 276, 525, 22,
	 15734, 59940, 27000, 59940, TRUE},
	{HDMI_VFRMT_1440x240p60_4_3, 1440, 240, FALSE, 1716, 276, 262, 22,
	 15734, 60054, 27000, 59940, TRUE},
	{HDMI_VFRMT_1440x240p60_4_3, 1440, 240, FALSE, 1716, 276, 263, 23,
	 15734, 59826, 27000, 59940, TRUE},
	{HDMI_VFRMT_1440x480p60_4_3, 1440, 480, FALSE, 1716, 276, 525, 45,
	 31469, 59940, 54000, 59940, TRUE},
	{HDMI_VFRMT_1440x480i60_4_3, 1440, 480, TRUE,  1716, 276, 525, 22,
	 15750, 60000, 27027, 60000, TRUE},
	{HDMI_VFRMT_1440x240p60_4_3, 1440, 240, FALSE, 1716, 276, 262, 22,
	 15750, 60115, 27027, 60000, TRUE},
	{HDMI_VFRMT_1440x240p60_4_3, 1440, 240, FALSE, 1716, 276, 263, 23,
	 15750, 59886, 27027, 60000, TRUE},
	{HDMI_VFRMT_1440x480p60_4_3, 1440, 480, FALSE, 1716, 276, 525, 45,
	 31500, 60000, 54054, 60000, TRUE},
	{HDMI_VFRMT_1440x576i100_4_3, 1440, 576, TRUE,  1728, 288, 625, 24,
	 31250, 100000, 54000, 100000, TRUE},
	{HDMI_VFRMT_1440x480i120_4_3, 1440, 480, TRUE,  1716, 276, 525, 22,
	 31469, 119880, 54000, 119880, TRUE},
	{HDMI_VFRMT_1440x480i120_4_3, 1440, 480, TRUE,  1716, 276, 525, 22,
	 31500, 120000, 54054, 120000, TRUE},
	{HDMI_VFRMT_1440x576i200_4_3, 1440, 576, TRUE,  1728, 288, 625, 24,
	 62500, 200000, 108000, 200000, TRUE},
	{HDMI_VFRMT_1440x480i240_4_3, 1440, 480, TRUE,  1716, 276, 525, 22,
	 62937, 239760, 108000, 239000, TRUE},
	{HDMI_VFRMT_1440x480i240_4_3, 1440, 480, TRUE,  1716, 276, 525, 22,
	 63000, 240000, 108108, 240000, TRUE},

	/* All 1920 H Active */
	{HDMI_VFRMT_1920x1080p24_16_9, 1920, 1080, FALSE, 2750, 830, 1125,
	 45, 26973, 23976, 74176, 24000, FALSE},
	{HDMI_VFRMT_1920x1080p24_16_9, 1920, 1080, FALSE, 2750, 830, 1125,
	 45, 27000, 24000, 74250, 24000, FALSE},
	{HDMI_VFRMT_1920x1080p25_16_9, 1920, 1080, FALSE, 2640, 720, 1125,
	 45, 28125, 25000, 74250, 25000, FALSE},
	{HDMI_VFRMT_1920x1080p30_16_9, 1920, 1080, FALSE, 2200, 280, 1125,
	 45, 33716, 29970, 74176, 30000, FALSE},
	{HDMI_VFRMT_1920x1080p30_16_9, 1920, 1080, FALSE, 2200, 280, 1125,
	 45, 33750, 30000, 74250, 30000, FALSE},
	{HDMI_VFRMT_1920x1080p50_16_9, 1920, 1080, FALSE, 2640, 720, 1125,
	 45, 56250, 50000, 148500, 50000, FALSE},
	{HDMI_VFRMT_1920x1080i50_16_9, 1920, 1080, TRUE,  2304, 384, 1250,
	 85, 31250, 50000, 72000, 50000, FALSE},
	{HDMI_VFRMT_1920x1080i60_16_9, 1920, 1080, TRUE,  2200, 280, 1125,
	 22, 33716, 59940, 74176, 59940, FALSE},
	{HDMI_VFRMT_1920x1080p60_16_9, 1920, 1080, FALSE, 2200, 280, 1125,
	 45, 67433, 59940, 148352, 59940, FALSE},
	{HDMI_VFRMT_1920x1080i60_16_9, 1920, 1080, TRUE,  2200, 280, 1125,
	 22, 33750, 60000, 74250, 60000, FALSE},
	{HDMI_VFRMT_1920x1080p60_16_9, 1920, 1080, TRUE,  2200, 280, 1125,
	 45, 67500, 60000, 148500, 60000, FALSE},
	{HDMI_VFRMT_1920x1080i100_16_9, 1920, 1080, TRUE,  2640, 720, 1125,
	 22, 56250, 100000, 148500, 100000, FALSE},
	{HDMI_VFRMT_1920x1080i120_16_9, 1920, 1080, TRUE,  2200, 280, 1125,
	 22, 67432, 119880, 148352, 119980, FALSE},
	{HDMI_VFRMT_1920x1080i120_16_9, 1920, 1080, TRUE,  2200, 280, 1125,
	 22, 67500, 120000, 148500, 120000, FALSE},

	/* All 2880 H Active */
	{HDMI_VFRMT_2880x576i50_4_3, 2880, 576, TRUE,  3456, 576, 625, 24,
	 15625, 50000, 54000, 50000, TRUE},
	{HDMI_VFRMT_2880x288p50_4_3, 2880, 576, FALSE, 3456, 576, 312, 24,
	 15625, 50080, 54000, 50000, TRUE},
	{HDMI_VFRMT_2880x288p50_4_3, 2880, 576, FALSE, 3456, 576, 313, 25,
	 15625, 49920, 54000, 50000, TRUE},
	{HDMI_VFRMT_2880x288p50_4_3, 2880, 576, FALSE, 3456, 576, 314, 26,
	 15625, 49761, 54000, 50000, TRUE},
	{HDMI_VFRMT_2880x576p50_4_3, 2880, 576, FALSE, 3456, 576, 625, 49,
	 31250, 50000, 108000, 50000, TRUE},
	{HDMI_VFRMT_2880x480i60_4_3, 2880, 480, TRUE,  3432, 552, 525, 22,
	 15734, 59940, 54000, 59940, TRUE},
	{HDMI_VFRMT_2880x240p60_4_3, 2880, 480, FALSE, 3432, 552, 262, 22,
	 15734, 60054, 54000, 59940, TRUE},
	{HDMI_VFRMT_2880x240p60_4_3, 2880, 480, FALSE, 3432, 552, 263, 23,
	 15734, 59940, 54000, 59940, TRUE},
	{HDMI_VFRMT_2880x480p60_4_3, 2880, 480, FALSE, 3432, 552, 525, 45,
	 31469, 59940, 108000, 59940, TRUE},
	{HDMI_VFRMT_2880x480i60_4_3, 2880, 480, TRUE,  3432, 552, 525, 22,
	 15750, 60000, 54054, 60000, TRUE},
	{HDMI_VFRMT_2880x240p60_4_3, 2880, 240, FALSE, 3432, 552, 262, 22,
	 15750, 60115, 54054, 60000, TRUE},
	{HDMI_VFRMT_2880x240p60_4_3, 2880, 240, FALSE, 3432, 552, 262, 23,
	 15750, 59886, 54054, 60000, TRUE},
	{HDMI_VFRMT_2880x480p60_4_3, 2880, 480, FALSE, 3432, 552, 525, 45,
	 31500, 60000, 108108, 60000, TRUE}, 
};
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
static ssize_t hdmi_attr_show(struct kobject *kobj, struct attribute *attr,
								char *buf)
{
	return sprintf(buf, "%d RES=%dx%d", \
		hdmi_state_obj->hdmi_connection_state, horizontal_resolution,
						vertical_resolution);
}

static ssize_t hdmi_attr_store(struct kobject *kobj, struct attribute *attr,
						const char *buf, size_t len)
{
	return 0;
}

static struct sysfs_ops hdmi_sysfs_ops = {
	.show = hdmi_attr_show,
	.store = hdmi_attr_store,
};
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#if DEBUG
static void enable_vibrator(void)
{
    mm_segment_t oldfs;
    struct file			*gfilp = NULL;
    char temp[]="5000";    
    
    oldfs=get_fs();
    set_fs(KERNEL_DS);
    gfilp = filp_open(VIBRATOR_FILE, O_RDWR | O_LARGEFILE, 0);         	
    gfilp->f_op->write(gfilp,(char __user *)temp,strlen(temp),&gfilp->f_pos);
    filp_close(gfilp, NULL);      
}
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
static void hdmi_release(struct kobject *kobj)
{
	kobject_uevent(&hdmi_state_obj->kobj, KOBJ_REMOVE);
	kfree(hdmi_state_obj);
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
	kfree(hdmi_edid_kobj);	
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */	
}

static struct kobj_attribute hdmi_attr =
	__ATTR(hdmi_state_obj, 0644, NULL, NULL);

static struct attribute *hdmi_attrs[] = {
	&hdmi_attr.attr,
	NULL,
};

static struct kobj_type hdmi_ktype = {
	.sysfs_ops = &hdmi_sysfs_ops,
	.release = hdmi_release,
	.default_attrs = hdmi_attrs,
};
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */


static int
proc_calc_metrics(char *page, char **start, off_t off,
                 int count, int *eof, int len)
{
    if (len <= off+count) *eof = 1;
    *start = page + off;
    len -= off;
    if (len > count) len = count;
    if (len < 0) len = 0;
    return len;
}

static int write_proc_edid_modes(struct file *file, const char __user *buf,
                       unsigned long count, void *data)
{
     char cmd[10];  
     int item = 0; 
     count = 1;
    if (copy_from_user(cmd, buf, count)) {
        return -EFAULT;
    } else {
        sscanf(cmd, "%d",  &item);     
	    video_format = item -1;
	}
	 printk("%s:Change EDID mode to (%d)\n",
			__func__, video_format);
			
	return count;
}
static int read_proc_edid_modes(char *page, char **start, off_t off,
                       int count, int *eof, void *data)	
{
	int len;
	
	len = snprintf(page, PAGE_SIZE, "%d", video_format+1);
	 printk("%s:read_proc_edid_modes (%d)\n",
			__func__, video_format);	
	return proc_calc_metrics(page, start, off, count, eof, len);
}

static int write_proc_ov_screen(struct file *file, const char __user *buf,
                       unsigned long count, void *data)
{
     char cmd[10];  
     int fid; 
	 
    if (copy_from_user(cmd, buf, count)) {
        return -EFAULT;
    } else {
        sscanf(cmd, "%d",  &fid);     
        printk("%s: Requested ov_screen = %d\n", __func__, fid);
	    extension_screen_resolution = fid;
	}			
	return count;
}
static int read_proc_ov_screen(char *page, char **start, off_t off,
                       int count, int *eof, void *data)	
{
	int len;	
	len = snprintf(page, PAGE_SIZE, "%d", extension_screen_resolution);
	return proc_calc_metrics(page, start, off, count, eof, len);
}
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
static int write_proc_hdcp(struct file *file, const char __user *buf,
                       unsigned long count, void *data)
{
     char cmd[10];  
     int item = 0; 
     count = 1;
    if (copy_from_user(cmd, buf, count)) {
        return -EFAULT;
    } else {
        sscanf(cmd, "%d",  &item);     
        printk("%s: hdcp = %d\n", __func__, item);
	    enable_hdcp = item;
	}			
	return count;
}
static int read_proc_hdcp(char *page, char **start, off_t off,
                       int count, int *eof, void *data)	
{
	int len;
	len = snprintf(page, PAGE_SIZE, "%d", enable_hdcp);	
	return proc_calc_metrics(page, start, off, count, eof, len);
}
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
/* create HDMI kobject and initialize */
static int create_hdmi_state_kobj(void)
{
	int ret;
	hdmi_state_obj = kzalloc(sizeof(struct hdmi_state), GFP_KERNEL);
	if (!hdmi_state_obj)
		return -1;
	hdmi_state_obj->kobj.kset =
		kset_create_and_add("hdmi_kset", NULL, kernel_kobj);

	ret = kobject_init_and_add(&hdmi_state_obj->kobj,
			&hdmi_ktype, NULL, "hdmi_kobj");
	if (ret) {
		kobject_put(&hdmi_state_obj->kobj);
		return -1;
	}
	hdmi_state_obj->hdmi_connection_state = 0;
	ret = kobject_uevent(&hdmi_state_obj->kobj, KOBJ_ADD);
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
    ent = create_proc_entry("edid_modes", 0666, NULL);
    ent->write_proc = write_proc_edid_modes;
    ent->read_proc = read_proc_edid_modes;
    
    ov_screen = create_proc_entry("hdmi_extension_screen", 0666, NULL);
    ov_screen->write_proc = write_proc_ov_screen;
    ov_screen->read_proc = read_proc_ov_screen;    
      			
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
    hdmi_hdcp = create_proc_entry("hdcp", 0666, NULL);
    hdmi_hdcp->write_proc = write_proc_hdcp;
    hdmi_hdcp->read_proc = read_proc_hdcp;  
#endif					
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */		
	printk(KERN_DEBUG "kobject uevent returned %d \n", ret);
	return 0;
}

/* Change HDMI state */
static int change_hdmi_state(int online)
{
	int ret;
	if (!hdmi_state_obj)
		return -1;
	if (online)
		ret = kobject_uevent(&hdmi_state_obj->kobj, KOBJ_ONLINE);
	else
		ret = kobject_uevent(&hdmi_state_obj->kobj, KOBJ_OFFLINE);
	printk(KERN_DEBUG "kobject uevent returned %d \n", ret);
	hdmi_state_obj->hdmi_connection_state = online;
	return 0;
}


/*
 * Read a value from a register on ADV7525 device
 * If sucessfull returns value read , otherwise error.
 */
static u8 adv7525_read_reg(struct i2c_client *client, u8 reg)
{
	int err, i = 0;
	struct i2c_msg msg[2];
	u8 reg_buf[] = { reg };
	u8 data_buf[] = { 0 };

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = reg_buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data_buf;

///	err = i2c_transfer(client->adapter, msg, 2);
	while( (err = i2c_transfer(client->adapter, msg, 2)) < 0 )
	{
		if( ++i > 5 )
		{
	        printk("%s: Error: adv7525_read_reg read I2C error\n", __func__);
			return FALSE;
		}
	    msleep(50);
	}
	if (err < 0)
		return err;

	return *data_buf;
}

/*
 * Write a value to a register on adv7525 device.
 * Returns zero if successful, or non-zero otherwise.
 */
static int adv7525_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int err, i = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;
	data[0] = reg;
	data[1] = val;

///	err = i2c_transfer(client->adapter, msg, 1);
	while( (err = i2c_transfer(client->adapter, msg, 1)) < 0 )
	{
		if( ++i > 5 )
		{
	        printk("%s: Error: adv7525_write_reg write I2C error\n", __func__);
			return FALSE;
		}
	    msleep(50);
	}	
	
	if (err >= 0)
		return 0;
	return err;
}
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
static boolean modify_pack_memory_address(void)
{
	/* Initialize the variables used to read/write the ADV7525 chip. */
	memset(&reg, 0xff, sizeof(reg));
		
	reg[0xE3] = 0x00;   ///override disable
    adv7525_write_reg(hclient, 0xE3, reg[0xE3]); 
    reg[0xE3] = 0x08;   ///override enable
    adv7525_write_reg(hclient, 0xE3, reg[0xE3]);   
    reg[0x45] = 0x80;   ///Modify I2C address 0x70 -> 0x80; 
    adv7525_write_reg(hclient, 0x45, reg[0x45]);
    if(adv7525_read_reg(hclient, 0x45) == 0x80)
        return TRUE;
    else
        return FALSE;                
} 
/*
 * hdmi_dvi_selection
 * 0 = DVI
 * 1 = HDMI
 */
void hdmi_dvi_selection( bool enable )
{
    unsigned long reg0xaf = adv7525_read_reg(hclient, 0xaf);
    if(enable)/* Set the HDMI select bit. */
	   set_bit(1, &reg0xaf);
    else	           
	   clear_bit(1, &reg0xaf);/* Set the DVI select bit. */	   
 	adv7525_write_reg(hclient, 0xaf, (u8)reg0xaf);	
}

#ifndef CONFIG_FIH_FTM  
static void set_avi_if(void)
{	
	   reg[0x55] = adv7525_read_reg(hclient, 0x55);  	
	   reg[0x56] = adv7525_read_reg(hclient, 0x56); 
   	   reg[0x57] = adv7525_read_reg(hclient, 0x57); 
       reg[0x58] = adv7525_read_reg(hclient, 0x58); 
  	   reg[0x59] = adv7525_read_reg(hclient, 0x59); 	
	   
	   reg[0x44] = adv7525_read_reg(hclient, 0x44); 	   
	   reg[0x44] |= 0x18;   ///Enable AVI & Audio InfoFrame		
	      
    if(video_format == HDMI_VFRMT_1280x720p60_16_9){
 	   reg[0x55] = 0x12;	  
  	   reg[0x56] = 0x28;	    
 	   reg[0x57] = 0x04;	
 	   reg[0x58] = 0x04;	        	   	   	   
	   reg[0x59] = 0x00;	   
	}else if(video_format == HDMI_VFRMT_1280x720p50_16_9){
 	   reg[0x55] = 0x12;	  
  	   reg[0x56] = 0x28;	    
 	   reg[0x57] = 0x04;	
 	   reg[0x58] = 0x13;	        	   	   	   
	   reg[0x59] = 0x00;	   
	}else if(video_format == HDMI_VFRMT_720x480p60_16_9){
 	   reg[0x55] = 0x12;	  
  	   reg[0x56] = 0x28;	    ///16:9
 	   reg[0x57] = 0x04;	
 	   reg[0x58] = 0x02;	        	   	   	   
	   reg[0x59] = 0x00;		   	
    }else if(video_format == HDMI_VFRMT_720x576p50_16_9){
 	   reg[0x55] = 0x02;	  
  	   reg[0x56] = 0x28;	    
 	   reg[0x57] = 0x05;	
 	   reg[0x58] = 0x12;	        	   	   	   
	   reg[0x59] = 0x00;		   
	}else if(video_format == HDMI_VFRMT_640x480p60_4_3){
 	   reg[0x55] = 0x12;	  
  	   reg[0x56] = 0x18;	   ///4:3 
 	   reg[0x57] = 0x04;	
 	   reg[0x58] = 0x01;	        	   	   	   
	   reg[0x59] = 0x00;		   
	}		
	adv7525_write_reg(hclient, 0x44, reg[0x44]);	   	
    adv7525_write_reg(hclient, 0x55, reg[0x55]);	
    adv7525_write_reg(hclient, 0x56, reg[0x56]);	
    adv7525_write_reg(hclient, 0x57, reg[0x57]);
    adv7525_write_reg(hclient, 0x58, reg[0x58]);
    adv7525_write_reg(hclient, 0x59, reg[0x59]);

}

static void set_audio_if(void)
{
	reg[0x73] = adv7525_read_reg(hclient, 0x73);   
	reg[0x77] = adv7525_read_reg(hclient, 0x77);
	reg[0x78] = adv7525_read_reg(hclient, 0x78); 	 	
	reg[0x73] = 0x01;
	reg[0x77] = 0x00;	
	reg[0x78] = 0x00;
	adv7525_write_reg(hclient, 0x73, reg[0x73]);
	adv7525_write_reg(hclient, 0x77, reg[0x77]);  
	adv7525_write_reg(hclient, 0x78, reg[0x78]);
}
static void set_di_params(void)
{
    u8 vic_out = 0;
	
	reg[0x3b] = adv7525_read_reg(hclient, 0x3b);   
	reg[0x3b] |= 0x40;
	adv7525_write_reg(hclient, 0x3b, reg[0x3b]); 
	
	vic_out = video_format + 1;
	reg[0x3c] = adv7525_read_reg(hclient, 0x3c); 
	reg[0x3c] = vic_out;
	adv7525_write_reg(hclient, 0x3c, reg[0x3c]);			
}

static void check_support_format(void)
{
	uint32	support_ndx         = 0;	    
   while (support_ndx < MAX_SUPPROT_MODES){	        		
    if(video_format == hdmi_support_mode_lut[support_ndx]){
       dsp_mode_found = TRUE;               
       break;
      }
     ++support_ndx;
    }    
}

static boolean hdmi_is_dvi_mode(void)
{
     bool dvi_mode  = TRUE;
     int i;
     for(i=0x84; i<0xff ;i++)
     {
       if(ereg[i] == 0x03){
           i++;
	      if(ereg[i] == 0x0c){
              i++;
	         if(ereg[i] == 0x00){
               i = 0xff;
	           dvi_mode = FALSE;
             }
	       }
        }	 
      }  
#if 0
        if(ereg[0x81] == 0x03)
           dvi_mode = FALSE;
#endif
	return dvi_mode;
}

static boolean check_edid_header(const uint8 *edid_buf)
{
	return (edid_buf[0] == 0x00) && (edid_buf[1] == 0xff)
		&& (edid_buf[2] == 0xff) && (edid_buf[3] == 0xff)
		&& (edid_buf[4] == 0xff) && (edid_buf[5] == 0xff)
		&& (edid_buf[6] == 0xff) && (edid_buf[7] == 0x00);
}

static void hdmi_edid_extract_vendor_id(const uint8 *in_buf)
{

	uint32 id_codes = ((uint32)in_buf[8] << 8) + in_buf[9];

	current_vendor_id[0] = 'A' - 1 + ((id_codes >> 10) & 0x1F);
	current_vendor_id[1] = 'A' - 1 + ((id_codes >> 5) & 0x1F);
	current_vendor_id[2] = 'A' - 1 + (id_codes & 0x1F);
	current_vendor_id[3] = 0;
}

static void hdmi_edid_detail_desc(const uint8 *data_buf, uint32 *disp_mode)
{
	boolean	aspect_ratio_4_3    = FALSE;
	boolean	interlaced          = FALSE;
	uint32	active_h            = 0;
	uint32	active_v            = 0;
	uint32	blank_h             = 0;
	uint32	blank_v             = 0;
	uint32	ndx                 = 0;
	uint32	max_num_of_elements = 0;
	uint32	img_size_h          = 0;
	uint32	img_size_v          = 0;
	
	/* See VESA Spec */
	/* EDID_TIMING_DESC_UPPER_H_NIBBLE[0x4]: Relative Offset to the EDID
	 *   detailed timing descriptors - Upper 4 bit for each H active/blank
	 *   field */
	/* EDID_TIMING_DESC_H_ACTIVE[0x2]: Relative Offset to the EDID detailed
	 *   timing descriptors - H active */
	active_h = ((((uint32)data_buf[0x4] >> 0x4) & 0xF) << 8)
		| data_buf[0x2];

	/* EDID_TIMING_DESC_H_BLANK[0x3]: Relative Offset to the EDID detailed
	 *   timing descriptors - H blank */
	blank_h = (((uint32)data_buf[0x4] & 0xF) << 8)
		| data_buf[0x3];

	/* EDID_TIMING_DESC_UPPER_V_NIBBLE[0x7]: Relative Offset to the EDID
	 *   detailed timing descriptors - Upper 4 bit for each V active/blank
	 *   field */
	/* EDID_TIMING_DESC_V_ACTIVE[0x5]: Relative Offset to the EDID detailed
	 *   timing descriptors - V active */
	active_v = ((((uint32)data_buf[0x7] >> 0x4) & 0xF) << 8)
		| data_buf[0x5];

	/* EDID_TIMING_DESC_V_BLANK[0x6]: Relative Offset to the EDID detailed
	 *   timing descriptors - V blank */
	blank_v = (((uint32)data_buf[0x7] & 0xF) << 8)
		| data_buf[0x6];

	/* EDID_TIMING_DESC_IMAGE_SIZE_UPPER_NIBBLE[0xE]: Relative Offset to the
	 *   EDID detailed timing descriptors - Image Size upper nibble
	 *   V and H */
	/* EDID_TIMING_DESC_H_IMAGE_SIZE[0xC]: Relative Offset to the EDID
	 *   detailed timing descriptors - H image size */
	/* EDID_TIMING_DESC_V_IMAGE_SIZE[0xD]: Relative Offset to the EDID
	 *   detailed timing descriptors - V image size */
	img_size_h = ((((uint32)data_buf[0xE] >> 0x4) & 0xF) << 8)
		| data_buf[0xC];
	img_size_v = (((uint32)data_buf[0xE] & 0xF) << 8)
		| data_buf[0xD];

	aspect_ratio_4_3 = (img_size_h * 3 == img_size_v * 4);

	max_num_of_elements  = sizeof(hdmi_edid_disp_mode_lut)
		/ sizeof(*hdmi_edid_disp_mode_lut);

	/* Break table in half and search using H Active */
	ndx = active_h < hdmi_edid_disp_mode_lut[max_num_of_elements / 2]
		.active_h ? 0 : max_num_of_elements / 2;

	/* EDID_TIMING_DESC_INTERLACE[0xD:8]: Relative Offset to the EDID
	 *   detailed timing descriptors - Interlace flag */
	interlaced = (data_buf[0x11] & 0x80) >> 7;

	printk("%s: A[%ux%u] B[%ux%u] V[%ux%u] %s\n", __func__,
		active_h, active_v, blank_h, blank_v, img_size_h, img_size_v,
		interlaced ? "i" : "p");

	*disp_mode = HDMI_VFRMT_FORCE_32BIT;	
    if(active_h > 1280 || active_v > 720){  ///it has not 720P resolution in 1080P's EDID
      horizontal_resolution = WIDTH_720P ;
      vertical_resolution   = HEIGHT_720P;		  
      video_format = HDMI_VFRMT_1280x720p60_16_9;
      hdmi_dvi_selection(TRUE);	
      dsp_mode_found = TRUE; 	    
	}else{
	 while (ndx < max_num_of_elements) {
		const struct hdmi_edid_video_mode_property_type *edid =
			hdmi_edid_disp_mode_lut+ndx;

		if ((interlaced    == edid->interlaced)    &&
			(active_h  == edid->active_h)      &&
			(blank_h   == edid->total_blank_h) &&
			(blank_v   == edid->total_blank_v) &&
			((active_v == edid->active_v) ||
			 (active_v == (edid->active_v + 1)))
		) {
			if (edid->aspect_ratio_4_3 && !aspect_ratio_4_3)
				/* Aspect ratio 16:9 */
				*disp_mode = edid->video_code + 1;
			else
				/* Aspect ratio 4:3 */
				*disp_mode = edid->video_code;
				
            horizontal_resolution = active_h;
            vertical_resolution   = active_v;
            hdmi_dvi_selection(TRUE);
	        check_support_format();
			printk("%s: mode found:%d\n", __func__, *disp_mode);
			break;
		}
		++ndx;
	}
	if (ndx == max_num_of_elements){
 	  horizontal_resolution = WIDTH_VGA ;
      vertical_resolution   = HEIGHT_VGA;		  
      video_format = HDMI_VFRMT_640x480p60_4_3;	
      hdmi_dvi_selection(FALSE);	
	  printk("%s: *no mode* found\n", __func__);  
	}
   }
}
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
#endif

#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
static void adv7525_close_hdcp_link(void)
{
    if (!hdcp_active){
        printk("HDCP ERROR Close HDMI link\n");
		return;    
	}
	
	printk("HDCP Close HDMI link\n");
	
	reg[0xD5] = adv7525_read_reg(hclient, 0xD5);
	reg[0xD5] &= 0xFE;
	adv7525_write_reg(hclient, 0xD5, (u8)reg[0xD5]);

	reg[0x16] = adv7525_read_reg(hclient, 0x16);
	reg[0x16] &= 0xFE;
	adv7525_write_reg(hclient, 0x16, (u8)reg[0x16]);

	/* UnMute Audio */
	adv7525_write_reg(hclient, 0x0C, (u8)0x84);
	hdcp_active = FALSE;
	hdcp_started = 0;

}

static void adv7525_hdcp_enable(struct work_struct *work)
{
	printk("%s: Start reg[0xaf]=%02x (mute audio)\n",
		__func__, reg[0xaf]);

	/* Mute Audio */
	adv7525_write_reg(hclient, 0x0C, (u8)0xC3);

	msleep(200);
	/* Wait for BKSV ready interrupt */
	/* Read BKSV's keys from HDTV */
	reg[0xBF] = adv7525_read_reg(hclient, 0xBF);
	reg[0xC0] = adv7525_read_reg(hclient, 0xC0);
	reg[0xC1] = adv7525_read_reg(hclient, 0xC1);
	reg[0xC2] = adv7525_read_reg(hclient, 0xC2);
	reg[0xc3] = adv7525_read_reg(hclient, 0xC3);

	printk("BKSV={%02x,%02x,%02x,%02x,%02x}\n", reg[0xbf], reg[0xc0],
		reg[0xc1], reg[0xc2], reg[0xc3]);

	/* Is SINK repeater */
	reg[0xBE] = adv7525_read_reg(hclient, 0xBE);
	printk("Sink Repeater reg is %x\n", reg[0xbe]);

	if (~(reg[0xBE] & 0x40)) {
		; /* compare with revocation list */
		/* Check 20 1's and 20 zero's */
	} else {
		/* Don't implement HDCP if sink as a repeater */
		adv7525_write_reg(hclient, 0x0C, (u8)0x84);
		printk("%s Sink Repeater, (UnMute Audio)\n", __func__);
		return;
	}

	msleep(200);
	reg[0xB8] = adv7525_read_reg(hclient, 0xB8);
	printk("HDCP Status  reg is reg[0xB8] is %x\n", reg[0xb8]);
	if (reg[0xb8] & 0x40) {
		/* UnMute Audio */
		adv7525_write_reg(hclient, 0x0C, (u8)0x84);
		printk("%s A/V content Encrypted (UnMute Audio)\n", __func__);
		hdcp_active = TRUE;		
	}
}

static void adv7525_hdcp_work_queue(void)
{
	schedule_work(&hdcp_handle_work);
}
#endif
#ifndef CONFIG_FIH_FTM 
static void adv7525_read_edid(void)
{
	const struct hdmi_edid_video_mode_property_type *edid;
	int i, read_edid_times = MAX_EDID_TRIES;
    bool edid_ready = TRUE;
///	uint32 num_of_disp_mode	= 0;
    uint32 support_ndx	= 0 , ndx = 0 ;
	uint32 num_og_cea_blocks  = 0;		
	unsigned long reg0xc9;
    uint32 first_dtb, last_dtb, desc_offset = 0;
    uint32	max_num_of_elements = 0;
    	
   	memset(ereg, 0, sizeof(ereg));   	
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */     
    current_vendor_id[0] = 0;     		
    video_format = -1;
    while(edid_ready){               	 
	 read_edid_times--;	
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
	/* Read EDID memory */

	 for (i = 0; i <= 0xff; i++)
	   ereg[i] = adv7525_read_reg(eclient, i);		
#if DEBUG	   
	   printk(KERN_DEBUG "ereg[%x] =%x \n " , i, ereg[i]);			
#endif	

		
	if(check_edid_header(ereg)) {     
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */		
	  hdmi_edid_extract_vendor_id(ereg);   
	  edid_ready = FALSE;
#if 0	 
	  if(strcmp(b_vendor_id,current_vendor_id) != 0){
	    strlcpy(b_vendor_id, current_vendor_id, sizeof(current_vendor_id));
	    extension_screen_resolution = 0x0000;
	  }else{	   	    
	    if(read_edid_times != 0)
	      edid_ready = TRUE;
	    else{
	      strlcpy(b_vendor_id, current_vendor_id, sizeof(current_vendor_id));
	      edid_ready = FALSE;
	    }
	  }
#endif	  
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
	     /* EDID_CEA_EXTENSION_FLAG[0x7E] - CEC extension byte */
	     num_og_cea_blocks = ereg[0x7E];		 
	  if(num_og_cea_blocks <= 1 ){
	     dsp_mode_found = FALSE;
         first_dtb = 0x36;
         last_dtb  = 0x6C;	
         for( desc_offset = first_dtb; desc_offset < last_dtb; desc_offset += 0x12 ){
		     hdmi_edid_detail_desc(ereg+desc_offset,&video_format);      
           if(dsp_mode_found)	 	 
              desc_offset = last_dtb + 1;
	 	  }		                               		    
	   if( ereg[ 0x80 ] == 0x2 ){                                    ///CEA 861 Extension block Tag Code
	       if(!dsp_mode_found){		   	    
	         last_dtb = ereg[0x84] & 0x1F;
	         for (desc_offset = 0; desc_offset < last_dtb; desc_offset++) {
	             support_ndx = 0;
	             video_format = (ereg[0x85 + desc_offset] & 0x7F) - 1;   	             
                  while (support_ndx < MAX_SUPPROT_MODES){	        		
                    if(video_format == hdmi_support_mode_lut[support_ndx]){
                      dsp_mode_found = TRUE;
	                  max_num_of_elements  = sizeof(hdmi_edid_disp_mode_lut)
		              / sizeof(*hdmi_edid_disp_mode_lut);
		              ndx = 0;
	                  while (ndx < max_num_of_elements) {		               	                   	 
		               edid = hdmi_edid_disp_mode_lut + ndx; 		              
		               if(edid->video_code == video_format){                  
                        horizontal_resolution = edid->active_h;
                        vertical_resolution   = edid->active_v;
                        break;
                       }
                       ++ndx;
                      }
                      support_ndx = MAX_SUPPROT_MODES;
 	                  desc_offset = last_dtb;               
 	                  break;
 	                }
 	               ++support_ndx;
 	               }	                 
	            }
	            if(!dsp_mode_found){
                    first_dtb = ereg[0x82];
                    last_dtb  = 0x80;
                   for( i = first_dtb; i < last_dtb; i += 0x12 ){
                      hdmi_edid_detail_desc(ereg+0x80+i,&video_format); 	
                     if(dsp_mode_found)
                        i = last_dtb;         
	                }
	                if(!dsp_mode_found) {
 		                horizontal_resolution = WIDTH_VGA ;
 		                vertical_resolution   = HEIGHT_VGA;		  
                        video_format = HDMI_VFRMT_640x480p60_4_3;	    
                        hdmi_dvi_selection(FALSE); 
#if DEBUG						               
                        enable_vibrator();                                          	                    
#endif						
	                    }
	             }	             
	        }		   	   
	          edid_ready = FALSE;				
	      }		 
		}else if(num_og_cea_blocks > 1){                                   ///For read 4 blocks	  
	    if(adv7525_read_reg(hclient, 0xc4)){
            reg[0xC4] = 0x00;
            adv7525_write_reg(hclient, 0xC4, reg[0xC4]);
          }else{
             reg[0xC4] = 0x01;
             adv7525_write_reg(hclient, 0xC4, reg[0xC4]);
          }			    	 	 
          msleep(50);		  
  	      for (i = 0; i <= 0xff; i++) 
	       ereg[i] = adv7525_read_reg(eclient, i);			  	        		        
	      edid_ready = FALSE;
		 }  	             
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */                        
	 }else{
	  if(!read_edid_times){                                                 ///NULL EDID
	    edid_ready = FALSE;
		printk(KERN_INFO " NULL EDID Setting 640X480 resolution \n");
 		horizontal_resolution = WIDTH_VGA ;
 		vertical_resolution   = HEIGHT_VGA;		  
        video_format = HDMI_VFRMT_640x480p60_4_3;		            
	   }
      msleep(500);	    	         
      set_bit(4, &reg0xc9);
	  adv7525_write_reg(hclient, 0xc9, (u8)reg0xc9);	 
	  clear_bit(4, &reg0xc9);	 
	  adv7525_write_reg(hclient, 0xc9, (u8)reg0xc9);	
	 }    
    }   
   if(horizontal_resolution == WIDTH_VGA && vertical_resolution == HEIGHT_VGA) {  
     video_format = HDMI_VFRMT_640x480p60_4_3;	
     hdmi_dvi_selection(FALSE);	     	
   }
   else
     hdmi_dvi_selection(TRUE);	

   printk(KERN_INFO "\n Monitor resolution is %d ** %d ID=%s\n ",
			horizontal_resolution, vertical_resolution, current_vendor_id) ;               
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */			
}                       
#endif                        
/*  Power ON/OFF  ADV7525 chip */
static int adv7525_power_on(struct platform_device *pdev)
{                       
	unsigned long reg0x41 = 0xff, reg0x42 = 0xff;
	bool hpd_set = TRUE;
#ifndef CONFIG_FIH_FTM                      
	if (pdev)           
		mfd = platform_get_drvdata(pdev);                        	                                             
#endif
	/* Attempting to enable power */
	/* Get the register holding the HPD bit, this must
	   be set before power up. */
	reg0x42 = adv7525_read_reg(hclient, 0x42);
	if (!test_bit(6, &reg0x42))
		hpd_set = FALSE;

	if (hpd_set) {
		/* Get the current register holding the power bit. */
		reg0x41 = adv7525_read_reg(hclient, 0x41);         
		/* Enable power */
		/* Clear the power down bit to enable power. */
		clear_bit(6, &reg0x41);
		adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
		power_on = TRUE ;
	}
///	read_edid = FALSE;
#ifndef CONFIG_FIH_FTM  	
	printk(KERN_INFO " mfd->var_xres = %d \n ", mfd->var_xres);
	printk(KERN_INFO " mfd->var_yres = %d \n ", mfd->var_yres);		
	
  if(hdmi_dvi_mode){
       hdmi_dvi_selection(FALSE);      
       reg[0x44] = adv7525_read_reg(hclient, 0x44); 				
   	   reg[0x44] &= 0xE7;   ///Disable AVI & Audio InfoFrame	   	
	   adv7525_write_reg(hclient, 0x44, reg[0x44]);     
  }
  else if(mfd->var_xres == WIDTH_VGA && mfd->var_yres == HEIGHT_VGA){
       hdmi_dvi_selection(FALSE);	
       reg[0x44] = adv7525_read_reg(hclient, 0x44); 				
   	   reg[0x44] &= 0xE7;   ///Disable AVI & Audio InfoFrame	   	
	   adv7525_write_reg(hclient, 0x44, reg[0x44]);       
   }
   else{
       hdmi_dvi_selection(TRUE);	   	
       set_di_params();
       set_audio_if();   
       set_avi_if();
   }
#endif   	
	return 0;
}

static int adv7525_power_off(struct platform_device *pdev)
{
	unsigned long reg0x41 = 0xff;

		if (power_on) {
		    printk("%s: turn off chip power\n", __func__);		    
			/* Power down the whole chip,except I2C,HPD interrupt */
			reg0x41 = adv7525_read_reg(hclient, 0x41);
			set_bit(6, &reg0x41);
			adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
			power_on = FALSE ;
		}
	return 0;
}
static void adv7525_chip_on(void)
{
		/* Get the current register holding the power bit. */
		unsigned long reg0x41 = adv7525_read_reg(hclient, 0x41);
		unsigned long reg0xaf = adv7525_read_reg(hclient, 0xaf);		

		printk("%s: turn on chip power\n", __func__);
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
        clk_enable_counter++;
	    clk_enable(tv_enc_clk);
	    clk_enable(tv_dac_clk);
 	    clk_enable(hdmi_clk);
	
	    if (mdp_tv_clk)
		    clk_enable(mdp_tv_clk);
        if(plugin_counter<1){ 
		    plugin_counter++;
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */  
		/* Enable power */
		/* Clear the power down bit to enable power. */
		clear_bit(6, &reg0x41);
		
		/* Set the HDMI select bit. */
		set_bit(1, &reg0xaf);
		
		adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
		adv7525_write_reg(hclient, 0xaf, (u8)reg0xaf);	
		}			
		chip_power_on = TRUE;
}

static void adv7525_chip_off(void)
{
		unsigned long reg0x41;

        if(chip_power_on){
		printk("%s: turn off chip power\n", __func__);
		clk_enable_counter--;
	    clk_disable(tv_enc_clk);
	    clk_disable(tv_dac_clk);
	    clk_disable(hdmi_clk);
	    if (mdp_tv_clk)
		    clk_disable(mdp_tv_clk);

		/* Power down the whole chip,except I2C,HPD interrupt */
		reg0x41 = adv7525_read_reg(hclient, 0x41);
		set_bit(6, &reg0x41);
		adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
        }
}
#if DEBUG
/* Read status registers for debugging */
static void adv7525_read_status(void)
{
	adv7525_read_reg(hclient, 0x94);
	adv7525_read_reg(hclient, 0x95);
	adv7525_read_reg(hclient, 0x97);
	adv7525_read_reg(hclient, 0xb8);
	adv7525_read_reg(hclient, 0xc8);
	adv7525_read_reg(hclient, 0x41);
	adv7525_read_reg(hclient, 0x42);
	adv7525_read_reg(hclient, 0xa1);
	adv7525_read_reg(hclient, 0xb4);
	adv7525_read_reg(hclient, 0xc5);
	adv7525_read_reg(hclient, 0x3e);
	adv7525_read_reg(hclient, 0x3d);
	adv7525_read_reg(hclient, 0xaf);
	adv7525_read_reg(hclient, 0xc6);

}
#else
#define adv7525_read_status() do {} while (0)
#endif


/* AV7525 chip specific initialization */
static void adv7525_enable(void)
{
	/* Initialize the variables used to read/write the ADV7525 chip. */
	memset(&reg, 0xff, sizeof(reg));

	/* Get the values from the "Fixed Registers That Must Be Set". */
	reg[0x98] = adv7525_read_reg(hclient, 0x98);
	reg[0x9c] = adv7525_read_reg(hclient, 0x9c);
	reg[0x9d] = adv7525_read_reg(hclient, 0x9d);
	reg[0xa2] = adv7525_read_reg(hclient, 0xa2);
	reg[0xa3] = adv7525_read_reg(hclient, 0xa3);
	reg[0xde] = adv7525_read_reg(hclient, 0xde);

	/* EDID Registers */
	reg[0x43] = adv7525_read_reg(hclient, 0x43);
	reg[0xc8] = adv7525_read_reg(hclient, 0xc8);

	/* Get the "HDMI/DVI Selection" register. */
	reg[0xaf] = adv7525_read_reg(hclient, 0xaf);
	
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
    /* Hard coded values provided by ADV7525 data sheet. */	
	reg[0x98] = 0x03;
	reg[0x9c] = 0x38;
	reg[0x9d] = 0x01;
	reg[0xa2] = 0xa0; ///0x94;
	reg[0xa3] = 0xa0; ///0x94;
    reg[0xce] = 0x00; ///SCLK-H	
	reg[0xcf] = 0x4a; ///SCLK-L	
	reg[0xd6] = 0x40; 	
	reg[0xde] = 0x82; ///0x88;
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
    if(enable_hdcp)	
	   reg[0xe4] = 0xee; /// DVDD1P2 is 1.2V 0x44 & Power on HDCP Controller	
	else
	   reg[0xe4] = 0xec; /// DVDD1P2 is 1.2V 0x44;    
#else	   
	reg[0xe4] = 0xec; /// DVDD1P2 is 1.2V 0x44;
#endif	
	reg[0xe5] = 0x88; ///0x80;
	reg[0xe6] = 0x1C;
	reg[0xeb] = 0x02;		

/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */  
	/* Set the HDMI select bit. */
	reg[0xaf] = 0x16;  ///0x16;HDMI
	

 	/* Set the 7525 audio related registers. */
	reg[0x01] = 0x00;
	reg[0x02] = 0x18;
	reg[0x03] = 0x00;
	reg[0x0a] = 0x41 ;
	reg[0x0b] = 0x8e ;
	reg[0x0c] =  0xbc ;
	reg[0x0d] =  0x18 ;
	reg[0x12] =  0x00 ;
	reg[0x14] =  0x00 ;
	reg[0x15] =  0x20 ;
	reg[0x44] =  0x79 ;
	reg[0x73] =  0x01 ;
	reg[0x76] =  0x00 ;



	/* Set 720p display related registers */
	reg[0x16] = 0x00;
	reg[0x18] = 0x46;
	reg[0x55] = 0x00;
	reg[0xba] = 0x60;
	reg[0x3c] = 0x04;

	/* Set Interrupt Mask register for HPD */
	reg[0x94] = 0xC4;
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
    if(enable_hdcp)
	   reg[0x95] = 0xC0;
	else
	   reg[0x95] = 0x00;	    
#else
	reg[0x95] = 0x00;
#endif
	adv7525_write_reg(hclient, 0x94, reg[0x94]);
	adv7525_write_reg(hclient, 0x95, reg[0x95]);


	/* Set the values from the "Fixed Registers That Must Be Set". */
	adv7525_write_reg(hclient, 0x98, reg[0x98]);
	adv7525_write_reg(hclient, 0x9c, reg[0x9c]);
	adv7525_write_reg(hclient, 0x9d, reg[0x9d]);
	adv7525_write_reg(hclient, 0xa2, reg[0xa2]);
	adv7525_write_reg(hclient, 0xa3, reg[0xa3]);
	adv7525_write_reg(hclient, 0xce, reg[0xce]);	
	adv7525_write_reg(hclient, 0xcf, reg[0xcf]);	
	adv7525_write_reg(hclient, 0xd6, reg[0xd6]);	
	adv7525_write_reg(hclient, 0xde, reg[0xde]);
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
	/* 7525 Set the values from the "Fixed Registers That Must Be Set". */	
	adv7525_write_reg(hclient, 0xe4, reg[0xe4]);
	adv7525_write_reg(hclient, 0xe5, reg[0xe5]);
	adv7525_write_reg(hclient, 0xe6, reg[0xe6]);
	adv7525_write_reg(hclient, 0xeb, reg[0xeb]);	
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */  

	/* Set the "HDMI/DVI Selection" register. */
	adv7525_write_reg(hclient, 0xaf, reg[0xaf]);

	/* Set EDID Monitor address */
	reg[0x43] = ADV7525_EDIDI2CSLAVEADDRESS ;
	adv7525_write_reg(hclient, 0x43, reg[0x43]);

	/*  Enable the i2s audio input.  */
	adv7525_write_reg(hclient, 0x01, reg[0x01]);
	adv7525_write_reg(hclient, 0x02, reg[0x02]);
	adv7525_write_reg(hclient, 0x03, reg[0x03]);
	adv7525_write_reg(hclient, 0x0a, reg[0x0a]);
	adv7525_write_reg(hclient, 0x0b, reg[0x0b]);
	adv7525_write_reg(hclient, 0x0c, reg[0x0c]);
	adv7525_write_reg(hclient, 0x0d, reg[0x0d]);
	adv7525_write_reg(hclient, 0x12, reg[0x12]);
	adv7525_write_reg(hclient, 0x14, reg[0x14]);
	adv7525_write_reg(hclient, 0x15, reg[0x15]);
	adv7525_write_reg(hclient, 0x44, reg[0x44]);
	adv7525_write_reg(hclient, 0x73, reg[0x73]);
	adv7525_write_reg(hclient, 0x76, reg[0x76]);

       /* Enable  720p display */
	adv7525_write_reg(hclient, 0x16, reg[0x16]);
	adv7525_write_reg(hclient, 0x18, reg[0x18]);
	adv7525_write_reg(hclient, 0x55, reg[0x55]);
	adv7525_write_reg(hclient, 0xba, reg[0xba]);
	adv7525_write_reg(hclient, 0x3c, reg[0x3c]);

}
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifndef CONFIG_FIH_FTM  
static void hdmi_edid_retry(void)
{
        unsigned long reg0xc9;
        int    timeout;  
        int read_edid_times = MAX_EDID_TRIES;
        bool edid_retry = TRUE;
        while(edid_retry){
           set_bit(4, &reg0xc9);
           adv7525_write_reg(hclient, 0xc9, (u8)reg0xc9);	 
           clear_bit(4, &reg0xc9);	 
           adv7525_write_reg(hclient, 0xc9, (u8)reg0xc9);	 	 
           msleep(500);
           timeout = (adv7525_read_reg(hclient, 0x96) & (1 << 2));         
         	if (timeout){
   		     adv7525_read_edid();
           	 msleep(50);
             if(hdmi_is_dvi_mode()){
 		      horizontal_resolution = WIDTH_VGA ;
 		      vertical_resolution   = HEIGHT_VGA;		  
              video_format = HDMI_VFRMT_640x480p60_4_3;
              hdmi_dvi_selection(FALSE);            
              hdmi_dvi_mode = TRUE;	
              reg[0x44] = adv7525_read_reg(hclient, 0x44); 				
   	          reg[0x44] &= 0xE7;   ///Disable AVI & Audio InfoFrame	   	
	          adv7525_write_reg(hclient, 0x44, reg[0x44]);  
              }
           	 edid_retry = FALSE;
	         read_edid = FALSE;
   	         }
   	        else if(read_edid_times!=0)
   	            read_edid_times--; 
   	        else {
#if 0
                horizontal_resolution = WIDTH_720P ;
                vertical_resolution   = HEIGHT_720P;		  
                video_format = HDMI_VFRMT_1280x720p60_16_9;
                hdmi_dvi_selection(TRUE);	
                printk(KERN_INFO " Read EDID fail,so Setting default resolution \n");						   
#endif				
		        printk(KERN_INFO " Read EDID fail,so Setting 640X480 resolution \n");		                        
#if DEBUG				
                enable_vibrator();	
#endif				
 		        horizontal_resolution = WIDTH_VGA ;
 		        vertical_resolution   = HEIGHT_VGA;		  
                video_format = HDMI_VFRMT_640x480p60_4_3;				
                hdmi_dvi_selection(FALSE);                                   			
   	            edid_retry = FALSE; 	    
   	            read_edid = FALSE;    
			}
	    }        
}
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
static void adv7525_handle_cable_work(struct work_struct *work)
{
	   if (monitor_plugin) {  
	    if(change_hdmi_counter<1){
        change_hdmi_state(1);
        change_hdmi_counter++;
		fqc_hpd_state = 1;
		///wake_lock(&hdmi_state_obj->wlock);
         adv7525_enable();		
		 adv7525_chip_on();		
#ifndef CONFIG_FIH_FTM  		 
		 if(read_edid){	   	
		    printk(KERN_INFO "calling read edid.. \n");			    
            hdmi_edid_retry();
		  if(video_format != HDMI_VFRMT_1280x720p60_16_9){
		      change_hdmi_state(0);
		      msleep(100);
		      change_hdmi_state(1);			
		      change_hdmi_counter++;
		   }
		  }	
#endif 		  	 
        }
		printk(KERN_DEBUG " \n %s Power ON from Interrupt handler \n ",
				__func__);
	} else {
        if(chip_power_on){
           if(change_hdmi_counter>0){              
		      change_hdmi_state(0);	
		      change_hdmi_counter = 0;
		    }   
		}
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */		
		adv7525_chip_off();	
        read_edid = TRUE;
		chip_power_on = FALSE;		
        hdmi_dvi_mode = FALSE;
        if(video_format != HDMI_VFRMT_1280x720p60_16_9)		
             hdmi_default_video_format();	
        for(;clk_enable_counter >0 ;clk_enable_counter--){ ///It's a workaround method that is the Qualcomm issue 
		msleep(500);
	    clk_disable(tv_enc_clk);
	    clk_disable(tv_dac_clk);
	    clk_disable(hdmi_clk);
	    if (mdp_tv_clk)
		    clk_disable(mdp_tv_clk);		
	    printk(KERN_DEBUG "\n %s clk_disable \n",
				__func__);					 			 
		}
		fqc_hpd_state = 0;		 
	    ///wake_unlock(&hdmi_state_obj->wlock);	
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */		
		plugin_counter = 0; 
	    while(hdmi_online){      
         msleep(500); 
        }		         	
		printk(KERN_DEBUG "\n %s Power OFF from Interrupt handler \n",
				__func__);
	}
}

static void adv7525_handle_cable(unsigned long data)
{
    schedule_work(&handle_work);
}

static void adv7525_work_f(struct work_struct *work)
{
	u8 reg0x96 = adv7525_read_reg(hclient, 0x96);
	unsigned long reg0x42 = 0x00;
	u8 retry = 0;
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
    u8 reg0xc8;
	u8 reg0x97 = adv7525_read_reg(hclient, 0x97);

    if(enable_hdcp)  {
       if (reg0x97 & 0x80) {
         reg[0xaf] = adv7525_read_reg(hclient, 0xaf);
         reg[0xaf] &= 0x16;
       	 adv7525_write_reg(hclient, 0xaf, reg[0xaf]);
         adv7525_close_hdcp_link();
         } 
    }
    printk("adv7525_irq: reg[0x96]=%x reg[0x97]=%x\n", reg0x96, reg0x97);	
#else		

	printk("adv7525_irq: reg[0x96]=%x\n", reg0x96);	
#endif	

	if (1) {
		unsigned int hpd_state = adv7525_read_reg(hclient, 0x42);

	         reg0x42 = hpd_state;
	    if (!test_bit(6, &reg0x42))
		    monitor_plugin = 0;		
		else
		    monitor_plugin = 1;		
		printk("from timer work queue:: reg[0x42] is %x && "
			" monitor plugin is = %x \n ", hpd_state, monitor_plugin);

		/* Timer for catching interrupt debouning */
		if (!timer_pending(&hpd_timer)) {
			init_timer(&hpd_timer);
			printk(KERN_DEBUG "%s Add Timer \n", __func__);
			hpd_timer.function = adv7525_handle_cable;
			hpd_timer.data = (unsigned long)NULL;
			hpd_timer.expires = jiffies + 1*HZ;
			add_timer(&hpd_timer);
		} else {
			printk(KERN_DEBUG "%s MOD Timer pending \n", __func__);
			mod_timer(&hpd_timer, jiffies + 1*HZ);
		}
	}		
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT	
    if(enable_hdcp)  {
       if(reg0x96 & 0x24){                  ///EDID Ready Interrupt
        msleep(500);
	    reg0x97 = adv7525_read_reg(hclient, 0x97);
       	/* request for HDCP after EDID is read */
       	reg[0xaf] = adv7525_read_reg(hclient, 0xaf);
       	reg[0xaf] |= 0x90;
       	adv7525_write_reg(hclient, 0xaf, reg[0xaf]);
       	reg[0xaf] = adv7525_read_reg(hclient, 0xaf);
        printk("%s Start HDCP reg[0xaf] is %x\n",
    				__func__, reg[0xaf]);
    
    	reg[0xba] = adv7525_read_reg(hclient, 0xba);
        reg[0xba] |= 0x10;
    	adv7525_write_reg(hclient, 0xba, reg[0xba]);
        reg[0xba] = adv7525_read_reg(hclient, 0xba);
    	printk("%s reg[0xba] is %x\n",
    			__func__, reg[0xba]);
    	hdcp_started = 1;
       	msleep(500);			 		       
        }
        printk("adv7525_irq: Enable HDCP reg[0x96]=%x reg[0x97]=%x\n", reg0x96, reg0x97);        
	   if (hdcp_started) {
           reg0x97 = adv7525_read_reg(hclient, 0x97);
		   /* BKSV Ready interrupts */
           printk("adv7525_irq: Start HDCP reg[0x96]=%x reg[0x97]=%x\n", reg0x96, reg0x97);		
		    if (reg0x97 & 0x40) {
			   printk("%s BKSV keys ready\n", __func__);
			   printk("Begin HDCP encryption\n");
			   adv7525_hdcp_work_queue();
	           reg0x97 = adv7525_read_reg(hclient, 0x97);
                /* Clearing the Interrupts */
	           adv7525_write_reg(hclient, 0x97, reg0x97);
	           reg0x97 = adv7525_read_reg(hclient, 0x97);
	           printk("BKSV: Clearing the Interrupts reg[0x96]=%x reg[0x97]=%x\n", reg0x96, reg0x97);
		     }
		     /* HDCP controller error Interrupt */
		    if (reg0x97 & 0x80) {
		      printk("adv7525_irq: HDCP_ERROR\n");
			  adv7525_close_hdcp_link();
	          reg0x97 = adv7525_read_reg(hclient, 0x97);
               /* Clearing the Interrupts */
	          adv7525_write_reg(hclient, 0x97, reg0x97);
	          reg0x97 = adv7525_read_reg(hclient, 0x97);
	          printk("HDCP_ERROR:Clearing the Interrupts reg[0x96]=%x reg[0x97]=%x\n", reg0x96, reg0x97);
		    }
		    reg0xc8 = adv7525_read_reg(hclient, 0xc8);
		    printk("DDC controller reg[0xC8] = %x\n", reg0xc8);
  	        printk("adv7525_irq final reg[0x96]=%x reg[0x97]=%x\n",
							reg0x96, reg0x97);
            if (reg0x97 & 0xC0) {
	          reg0x97 = adv7525_read_reg(hclient, 0x97);
              /* Clearing the Interrupts */
	          adv7525_write_reg(hclient, 0x97, reg0x97);
	          reg0x97 = adv7525_read_reg(hclient, 0x97);
	          printk("Final:Clearing the Interrupts reg[0x96]=%x reg[0x97]=%x\n", reg0x96, reg0x97);
		      reg0xc8 = adv7525_read_reg(hclient, 0xc8);
		      printk("DDC controller reg0x97==0xC0 reg[0xC8] = %x\n", reg0xc8);
  	          printk("adv7525_irq reg0x97==0xC0 reg[0x96]=%x reg[0x97]=%x\n",
							reg0x96, reg0x97);
			  }
	    }
	}		
#endif	
	while (retry < 5) {
		if (dd->pd->intr_detect())
			break;
		else {
			/* Clearing the Interrupts */
			reg0x96 = adv7525_read_reg(hclient, 0x96);
			adv7525_write_reg(hclient, 0x96, reg0x96);
			msleep(100);
			retry++;
			pr_info("%s: clear interrupt pin, retry %d times",
				__func__, retry);
		}
	}
///	enable_irq(dd->pd->irq);
}

static irqreturn_t adv7525_interrupt(int irq, void *dev_id)
{
///    disable_irq_nosync(dd->pd->irq);
    schedule_work(&dd->work);
    return IRQ_HANDLED ;
}

static const struct i2c_device_id adv7525_id[] = {
	{ ADV7525_DRV_NAME , 0},
	{}
};

static struct msm_fb_panel_data hdmi_panel_data = {
	.on  = adv7525_power_on,
	.off = adv7525_power_off,
};

static struct platform_device hdmi_device = {
	.name = ADV7525_DRV_NAME ,
	.id   = 2,
	.dev  = {
		.platform_data = &hdmi_panel_data,
		}
};

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
static void hdmi_default_video_format(void)
{
    horizontal_resolution = WIDTH_720P ;
    vertical_resolution   = HEIGHT_720P;	    	   
	video_format = HDMI_VFRMT_1280x720p60_16_9;	
	pinfo = &hdmi_panel_data.panel_info;
	pinfo->xres = WIDTH_720P ;
	pinfo->yres = HEIGHT_720P ;
	pinfo->type = DTV_PANEL;
	pinfo->pdest = DISPLAY_2;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 1;
	pinfo->clk_rate = 74250000;
	pinfo->lcdc.h_back_porch = 220;
	pinfo->lcdc.h_front_porch = 110;
	pinfo->lcdc.h_pulse_width = 40;
	pinfo->lcdc.v_back_porch = 20;
	pinfo->lcdc.v_front_porch = 5;
	pinfo->lcdc.v_pulse_width = 5;
	pinfo->lcdc.border_clr = 0;	/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;    
}
#ifdef CONFIG_FIH_FTM
static ssize_t hdmi_adv7525_ftm_test_switch_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{

        	unsigned long reg0x41 = 0xff;             ///For FTM HDMI Off

			/* Power down the whole chip,except I2C,HPD interrupt */
			reg0x41 = adv7525_read_reg(hclient, 0x41);
			set_bit(6, &reg0x41);
			adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
			power_on = FALSE ;
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */			
	        clk_disable(tv_enc_clk);
	        clk_disable(tv_dac_clk);
	        clk_disable(hdmi_clk);
	        if (mdp_tv_clk)
		       clk_disable(mdp_tv_clk);	
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */	
		    printk("Requested FTM HDMI Off\n");   
	return size;
		
}
#endif

#ifdef CONFIG_FIH_FTM
static ssize_t hdmi_adv7525_ftm_test_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{	
	unsigned long reg0x41 = adv7525_read_reg(hclient, 0x41);
	unsigned long reg0xaf = adv7525_read_reg(hclient, 0xaf);
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
    clk_enable(tv_enc_clk);    
    clk_enable(tv_dac_clk);    
    clk_enable(hdmi_clk);      
	                               
	if (mdp_tv_clk)            
		clk_enable(mdp_tv_clk);
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */

	/* Enable power */
	/* Clear the power down bit to enable power. */
	clear_bit(6, &reg0x41);		
    /* Set the HDMI select bit. */
	set_bit(1, &reg0xaf);
	adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
	adv7525_write_reg(hclient, 0xaf, (u8)reg0xaf);		
    mdelay(2700);
	  return 1;
}

static ssize_t hdmi_adv7525_ftm_ping_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int err;
	unsigned long reg0x41;
	reg[0xf5] = 0x00;
	reg[0xf6] = 0x00;
		
	/* Power up chip */
	reg0x41 = adv7525_read_reg(hclient, 0x41);


	/* Enable power */
	/* Clear the power down bit to enable power. */
	clear_bit(6, &reg0x41);		
	adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
	
    err = adv7525_read_reg(hclient, 0x41);
    if (err < 0 || err == -ENODEV){
  	    err = 0;
	 	printk(KERN_ERR "<%s> adv7525_dev I2C R/W failed.\n",
			__func__);
	}else {
        reg[0xf5] = adv7525_read_reg(hclient, 0xf5);
        reg[0xf6] = adv7525_read_reg(hclient, 0xf6);
        if(reg[0xf5] == 0x75 && reg[0xf6] == 0x25){    	
  	     err = 1;
  	    printk(KERN_INFO "<%s> adv7525_dev ID=%x%x\n",__func__,reg[0xf5],reg[0xf6]);
  	    } else {  	
  		err = 0;
	    printk(KERN_INFO "<%s> adv7525_dev ID=%x%x failed\n",__func__,reg[0xf5],reg[0xf6]);
	    }
	 }
	 set_bit(6, &reg0x41);
	 adv7525_write_reg(hclient, 0x41, (u8)reg0x41);	 
	 err = scnprintf(buf, PAGE_SIZE, "%d\n", err);
	return 1;
}
#endif
static ssize_t hdmi_hpd_status_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
    int i;   
    
#ifdef CONFIG_FIH_FTM  
    unsigned long reg0x42 = 0x00 , reg0x41 = 0x00;
	/* Power up chip */
	reg0x41 = adv7525_read_reg(hclient, 0x41);
	/* Enable power */
	/* Clear the power down bit to enable power. */
	clear_bit(6, &reg0x41);		
	adv7525_write_reg(hclient, 0x41, (u8)reg0x41);
    
	reg0x42 = adv7525_read_reg(hclient, 0x42);	
	if (test_bit(6, &reg0x42))
		 fqc_hpd_state = 1;
	else
	     fqc_hpd_state = 0;	 	      
#endif
	     
	i = scnprintf(buf, PAGE_SIZE, "%d\n", fqc_hpd_state);
	return i;
}

#ifdef CONFIG_FIH_FTM
static DEVICE_ATTR(hdmi_adv7525_ping, 0666, hdmi_adv7525_ftm_ping_show, NULL);	
static DEVICE_ATTR(hdmi_adv7525_test, 0666, hdmi_adv7525_ftm_test_show, hdmi_adv7525_ftm_test_switch_store);
#endif
static DEVICE_ATTR(hdmi_hpd_status, 0644, hdmi_hpd_status_show, NULL);		

static int adv7525_suspend(struct device *dev)
{  	     
    return 0;
}

static int adv7525_resume(struct device *dev)
{     
///	if (dd->pd->irq && hdmi_wakeup_enabled){		    
        read_edid = TRUE;	
#ifdef ENABLE_WAKEUP      
        if (hdmi_wakeup_enabled) {
		disable_irq_wake(dd->pd->irq);
		hdmi_wakeup_enabled = 0;
	    }
#endif	   	    
///	}	
    return 0;
}

static void adv7525_early_suspend(struct early_suspend *h)
{      		    
#if DEBUG    
    reg[0xE3] = 0x00;   ///override disable I2C address return to default 0x70
    adv7525_write_reg(hclient, 0xE3, reg[0xE3]);
    adv7525_write_reg(hclient, 0x96, 0x0);	
	reg[0x94] = 0xC4;
	adv7525_write_reg(hclient, 0x94, reg[0x94]);	
#endif
#ifdef ENABLE_WAKEUP
	 if (!hdmi_wakeup_enabled){ 
	 enable_irq_wake(dd->pd->irq);	
	 hdmi_wakeup_enabled = 1;
	 }
#endif	
     if(power_on){
     dd->pd->setup_int_power(0);
     monitor_plugin = 0;
     schedule_work(&handle_work);
     }
     
}

static void adv7525_late_resume(struct early_suspend *h)
{
#if DEBUG     	   
    reg[0xE3] = 0x00;   ///override disable
    adv7525_write_reg(hclient, 0xE3, reg[0xE3]);	
    reg[0xE3] = 0x08;   ///override enable
    adv7525_write_reg(hclient, 0xE3, reg[0xE3]);     	
    reg[0x45] = 0x80;   ///Modify I2C address 0x70 -> 0x80; 
    adv7525_write_reg(hclient, 0x45, reg[0x45]);  
#endif
    unsigned long reg0x42;
    dd->pd->setup_int_power(1); 
    reg0x42 = adv7525_read_reg(hclient, 0x42);
	if (!test_bit(6, &reg0x42))
	   monitor_plugin = 0;		
	else
	   monitor_plugin = 1;		
    schedule_work(&handle_work);	
    reg[0x96] = 0xff;
    adv7525_write_reg(hclient, 0x96, reg[0x96]); 
}

struct early_suspend g_adv7525_earlysuspend = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = adv7525_early_suspend,
	.resume = adv7525_late_resume,
};
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
static int __devinit
adv7525_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *edid_adap = i2c_get_adapter(0) ;

	int rc , mi2c = 0;
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
    bool mready = TRUE;
#ifdef CONFIG_FIH_FTM
	rc = device_create_file(&client->dev, &dev_attr_hdmi_adv7525_ping);
	if (rc < 0) {
	   dev_err(&client->dev, "%s: Create attribute \"hdmi_adv7525\" failed!! <%d>\n", __func__, rc);
	   return rc; 
	}
	
	rc = device_create_file(&client->dev, &dev_attr_hdmi_adv7525_test);
	if (rc < 0) {
	   dev_err(&client->dev, "%s: Create attribute \"hdmi_adv7525\" failed!! <%d>\n", __func__, rc);
	   return rc; 
	}
#endif	
	rc = device_create_file(&client->dev, &dev_attr_hdmi_hpd_status);
	if (rc < 0) {
	   dev_err(&client->dev, "%s: Create attribute \"hdmi_adv7525\" failed!! <%d>\n", __func__, rc);
	   return rc; 
	}
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */	
	dd = kzalloc(sizeof *dd, GFP_KERNEL);
	if (!dd) {
			rc = -ENOMEM ;
			goto probe_exit;
		}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	/* Init real i2c_client */
	hclient = client;
	eclient = i2c_new_dummy(edid_adap, ADV7525_EDIDI2CSLAVEADDRESS >> 1);

	if (!eclient)
		printk(KERN_ERR "Address %02x unavailable \n",
				ADV7525_EDIDI2CSLAVEADDRESS >> 1);
	
    while(mready){   	
    if(modify_pack_memory_address())
       mready = FALSE;  
    else if(mi2c != MAX_SUPPROT_MODES){
        mi2c++;  
       printk(KERN_INFO "<%s> unavailable modify_i2c_slave_address %d\n",__func__,mi2c);
    }
    else
       mready = FALSE;     
    }
	
	adv7525_enable();
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
    reg[0xf5] = adv7525_read_reg(hclient, 0xf5);
    reg[0xf6] = adv7525_read_reg(hclient, 0xf6);
    if((reg[0xf5] == 0x75) && (reg[0xf6] == 0x25))
      printk(KERN_INFO "<%s> adv7525_dev ID =%x%x Chip Revision=%x\n",__func__,reg[0xf5],reg[0xf6],
             adv7525_read_reg(hclient, 0x00));  
     else{
      printk(KERN_INFO "<%s> adv7525_dev the error ID =%x%x\n",__func__,reg[0xf5],reg[0xf6]);  
      rc = -ENOMEM ;
	  goto probe_exit;  
     }
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
	
	i2c_set_clientdata(client, dd);
	dd->pd = client->dev.platform_data;
	if (!dd->pd) {
			rc = -ENODEV ;
			goto probe_free;
		}
	if (dd->pd->irq) {
		INIT_WORK(&dd->work, adv7525_work_f);
		INIT_WORK(&handle_work, adv7525_handle_cable_work);
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL_HDCP_SUPPORT
	    INIT_WORK(&hdcp_handle_work, adv7525_hdcp_enable);
#endif		
		rc = request_irq(dd->pd->irq,
			&adv7525_interrupt,
			IRQF_TRIGGER_FALLING,
			"adv7525_cable", dd);

		if (rc)
			goto probe_free ;
	/*	disable_irq(dd->pd->irq); */
		}
	create_hdmi_state_kobj();
	msm_fb_add_device(&hdmi_device);
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
#ifdef ENABLE_WAKEUP     
   if (device_may_wakeup(&client->dev)){
     if (enable_irq_wake(dd->pd->irq) == 0)	
          device_init_wakeup(&client->dev, 1);
          hdmi_wakeup_enabled = 0;
      }
#endif      
	register_early_suspend(&g_adv7525_earlysuspend);
///wake_lock_init(&hdmi_state_obj->wlock, WAKE_LOCK_SUSPEND,
		///		"hdmi_active");
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
	return 0;

probe_free:
	i2c_set_clientdata(client, NULL);
	kfree(dd);
probe_exit:
	return rc;

}

static int __devexit adv7525_remove(struct i2c_client *client)
{
	int err = 0 ;
	if (!client->adapter) {
		printk(KERN_ERR "<%s> No HDMI Device \n",
			__func__);
		return -ENODEV;
	}
	i2c_unregister_device(eclient);
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifdef CONFIG_FIH_FTM
	device_remove_file(&client->dev, &dev_attr_hdmi_adv7525_ping);
	device_remove_file(&client->dev, &dev_attr_hdmi_adv7525_test);
#endif	
	device_remove_file(&client->dev, &dev_attr_hdmi_hpd_status);
    disable_irq_wake(dd->pd->irq);
    free_irq(dd->pd->irq, ADV7525_DRV_NAME);
    gpio_free(dd->pd->irq);
    unregister_early_suspend(&g_adv7525_earlysuspend);
    ///wake_lock_destroy(&hdmi_state_obj->wlock);
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
	return err ;
}
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
struct dev_pm_ops adv7525_driver_pm = {
     .suspend      = adv7525_suspend,
     .resume       = adv7525_resume,
};
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
static struct i2c_driver hdmi_i2c_driver = {
	.driver		= {
		.name   = ADV7525_DRV_NAME,
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */		
		.pm     = &adv7525_driver_pm,
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */		
	},
	.probe 		= adv7525_probe,
	.id_table 	= adv7525_id,
	.remove		= __devexit_p(adv7525_remove),
};

static int __init adv7525_init(void)
{
	int rc;
	
    hdmi_default_video_format();

	rc = i2c_add_driver(&hdmi_i2c_driver);
	if (rc) {
		printk(KERN_ERR "hdmi_init FAILED: i2c_add_driver rc=%d\n", rc);
		goto init_exit;
	}
	
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */	
	tv_enc_clk = clk_get(NULL, "tv_enc_clk");
	if (IS_ERR(tv_enc_clk)) {
		printk(KERN_ERR "error: can't get tv_enc_clk!\n");
		return IS_ERR(tv_enc_clk);
	}

	tv_dac_clk = clk_get(NULL, "tv_dac_clk");
	if (IS_ERR(tv_dac_clk)) {
		printk(KERN_ERR "error: can't get tv_dac_clk!\n");
		return IS_ERR(tv_dac_clk);
	}

	hdmi_clk = clk_get(NULL, "hdmi_clk");
	if (IS_ERR(hdmi_clk)) {
		printk(KERN_ERR "error: can't get hdmi_clk!\n");
		return IS_ERR(hdmi_clk);
	}
	
	mdp_tv_clk = clk_get(NULL, "mdp_tv_clk");
	if (IS_ERR(mdp_tv_clk))
		mdp_tv_clk = NULL;
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
			        	
	return 0;

init_exit:
	return rc;
}

static void __exit adv7525_exit(void)
{
	i2c_del_driver(&hdmi_i2c_driver);
}

module_init(adv7525_init);
module_exit(adv7525_exit);
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1");
MODULE_AUTHOR("Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("ADV7525 HDMI driver");
