/*
 *     mt9p111.c - Camera Sensor Config
 *
 *     Copyright (C) 2010 Kent Kwan <kentkwan@fihspec.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "fih_camera_parameter.h"
#include "fih_camera_power.h"
#include "mt9p111.h"
#include <linux/mfd/pmic8058.h> 
#include <mach/pmic.h>
#include <linux/completion.h>
#include <linux/hrtimer.h>
#include "../../../../arch/arm/mach-msm/smd_private.h"

#ifdef CONFIG_FIH_FTM
#include <linux/debugfs.h>
#endif
#ifdef CONFIG_MT9P111_SUSPEND
#include <linux/earlysuspend.h>
#ifdef CONFIG_MT9P111_RESUME
static pid_t mt9p111_resumethread_id;
struct hrtimer mt9p111_resume_timer;
DECLARE_COMPLETION(mt9p111_resume_comp);
#endif
#endif
static struct wake_lock mt9p111_wake_lock_suspend;
static int mt9p111_m_ledmod=0;
static int mt9p111_exposure_mode=-1;
static int mt9p111_touchAEC_mode=-1;
static int mt9p111_antibanding_mode=-1;
static int mt9p111_brightness_mode=-1;
static int mt9p111_contrast_mode=-1;
static int mt9p111_effect_mode=-1;
static int mt9p111_saturation_mode=-1;
static int mt9p111_sharpness_mode=-1;
static int mt9p111_whitebalance_mode=-1;
static int mt9p111_iso_mode = -1;
static int mt9p111_parameters_changed_count=0;
static int mt9p111_rectangle_x=0;
static int mt9p111_rectangle_y=0;
static int mt9p111_rectangle_w=0;
static int mt9p111_rectangle_h=0;
static int mt9p111_CAF_mode=0;

static pid_t mt9p111_thread_id;
struct hrtimer mt9p111_flashled_timer;

DECLARE_COMPLETION(mt9p111_flashled_comp);

#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*
static int wb_tmp = 0;
static int brightness_tmp = 0;
static uint16_t flash_target_addr_val = 0;
#endif

//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00+{
#ifdef CONFIG_FIH_AAT1272
#define FLASHLED_ID "aat1272"
#endif
#ifdef CONFIG_FIH_FITI_FP6773
#define FLASHLED_ID "fp6773"
#endif
//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00+}

/* Micron mt9p111 Registers and their values */
/* Sensor Core Registers */
#define  REG_MT9P111_MODEL_ID 0x3000
#define  MT9P111_MODEL_ID     0x2880
/*  SOC Registers Page 1  */
#define  REG_mt9p111_SENSOR_RESET     0x001A
#define  REG_mt9p111_STANDBY_CONTROL  0x0018
#define  REG_mt9p111_MCU_BOOT         0x001C
#define mt9p111_FRAME_WIDTH  640
#define mt9p111_FRAME_HEIGHT  480
static int mt9p111_init_done=0;
static int mt9p111_snapshoted=0;
static int mt9p111_vreg_acore=0;
static int mt9p111_vreg_vddio=0;

#ifdef CONFIG_MT9P111_STANDBY
static int mt9p111_first_init=1;
#endif

struct mt9p111_work {
	struct work_struct work;
};

static struct  mt9p111_work *mt9p111_sensorw;
static struct  i2c_client *mt9p111_client;
static const struct msm_camera_sensor_info *mt9p111info;

struct mt9p111_ctrl {
    const struct msm_camera_sensor_info *sensordata;
};

static struct mt9p111_ctrl *mt9p111_ctrl;

#ifdef CONFIG_FIH_AAT1272
static struct  i2c_client *aat1272_client;
#endif

#if defined(CONFIG_MT9P111_DYNPGA) || defined(CONFIG_MT9P111_AUTO_DETECT_OTP)//Div2-SW6-MM-MC-ImplementAutoDetectOTP-00*
// if use dynamic lens shading, uncomment below
//#define DYN_LSC_USE 
struct pga_struct {
    int32_t g1_p0q2;     /* 0x3644 */
    int32_t r_p0q2;    /* 0x364E */
    int32_t b_p0q1;    /* 0x3656 */
    int32_t b_p0q2;    /* 0x3658 */
    int32_t g2_p0q2;    /* 0x3662 */
    int32_t r_p1q0;    /* 0x368A */
    int32_t b_p1q0;    /* 0x3694 */
    int32_t g1_p2q0;    /* 0x36C0 */
    int32_t r_p2q0;    /* 0x36CA */
    int32_t b_p2q0;    /* 0x36D4 */
    int32_t g2_p2q0;    /* 0x36DE */
};
#endif

static DECLARE_WAIT_QUEUE_HEAD(mt9p111_wait_queue);
DECLARE_MUTEX(mt9p111_sem);
DEFINE_MUTEX(mt9p111_mut);
#ifdef CONFIG_MT9P111_RESUME
DEFINE_MUTEX(front_mut);
#endif
#ifdef CONFIG_FIH_FTM
#define MT9P111_FTM_SET_FLASH_SYNC "ftm_set_flash_sync"
#define MT9P111_FTM_SET_FLASH_DRV_EN "ftm_set_flash_drv_en"
#define MT9P111_FTM_SET_AF "ftm_set_af"
static int mt9p111_ftm_af_Enable=0;//Div2-SW6-MM-MC-ReduceTestTime-01+
#endif

//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+{
#ifdef CONFIG_MT9P111_AUTO_DETECT_OTP
typedef enum 
{
   cam_lc_indoor,
   cam_lc_outdoor,
   cam_lc_yellow_light,
   cam_otp_indoor,
   cam_otp_outdoor,
   FIH_MODE_MAX
}fih_cam_door_mode;

static int mt9p111_enable_auto_detect_otp = 0;
static fih_cam_door_mode door_mode = cam_lc_indoor;
static fih_cam_door_mode now_door_mode = FIH_MODE_MAX; 
#endif
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+}

#define MT9P111_USE_VFS
#ifdef MT9P111_USE_VFS

#define MT9P111_INIT_REG "init_reg"
#define MT9P111_LENS_REG "lens_reg"
#define MT9P111_IQ1_REG "iq1_reg"
#define MT9P111_IQ2_REG "iq2_reg"
#define MT9P111_CHAR_REG "char_reg"
#define MT9P111_AFTRIGGER_REG "aftrigger_reg"
#define MT9P111_PREV_SNAP_REG "prev_snap_reg"
#define MT9P111_PATCH_REG "patch_reg"
#define MT9P111_PLL_REG "pll_reg"
#define MT9P111_AF_REG "af_reg" 
#define MT9P111_OEMREG "oemreg" 
#define MT9P111_WRITEREG "writereg"
#define MT9P111_GETREG "getreg" 
#define MT9P111_FQC_SET_FLASH "fqc_set_flash"
#define MT9P111_VFS_PARAM_NUM 4 

/* MAX buf is ???? */
#define MT9P111_MAX_VFS_PREV_SNAP_INDEX 330
int mt9p111_use_vfs_prev_snap_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_prev_snap_settings_tbl[MT9P111_MAX_VFS_PREV_SNAP_INDEX];
uint16_t mt9p111_vfs_prev_snap_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_prev_snap_settings_tbl);

#define MT9P111_MAX_VFS_INIT_INDEX 330
int mt9p111_use_vfs_init_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_init_settings_tbl[MT9P111_MAX_VFS_INIT_INDEX];
uint16_t mt9p111_vfs_init_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_init_settings_tbl);

#define MT9P111_MAX_VFS_LENS_INDEX 330
int mt9p111_use_vfs_lens_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_lens_settings_tbl[MT9P111_MAX_VFS_LENS_INDEX];
uint16_t mt9p111_vfs_lens_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_lens_settings_tbl);

#define MT9P111_MAX_VFS_IQ1_INDEX 330
int mt9p111_use_vfs_iq1_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_iq1_settings_tbl[MT9P111_MAX_VFS_IQ1_INDEX];
uint16_t mt9p111_vfs_iq1_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_iq1_settings_tbl);

#define MT9P111_MAX_VFS_IQ2_INDEX 330
int mt9p111_use_vfs_iq2_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_iq2_settings_tbl[MT9P111_MAX_VFS_IQ2_INDEX];
uint16_t mt9p111_vfs_iq2_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_iq2_settings_tbl);

#define MT9P111_MAX_VFS_CHAR_INDEX 330
int mt9p111_use_vfs_char_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_char_settings_tbl[MT9P111_MAX_VFS_CHAR_INDEX];
uint16_t mt9p111_vfs_char_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_char_settings_tbl);

#define MT9P111_MAX_VFS_AFTRIGGER_INDEX 330
int mt9p111_use_vfs_aftrigger_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_aftrigger_settings_tbl[MT9P111_MAX_VFS_AFTRIGGER_INDEX];
uint16_t mt9p111_vfs_aftrigger_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_aftrigger_settings_tbl);

#define MT9P111_MAX_VFS_PATCH_INDEX 330
int mt9p111_use_vfs_patch_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_patch_settings_tbl[MT9P111_MAX_VFS_PATCH_INDEX];
uint16_t mt9p111_vfs_patch_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_patch_settings_tbl);

#define MT9P111_MAX_VFS_PLL_INDEX 330
int mt9p111_use_vfs_pll_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_pll_settings_tbl[MT9P111_MAX_VFS_PLL_INDEX];
uint16_t mt9p111_vfs_pll_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_pll_settings_tbl);

#define MT9P111_MAX_VFS_AF_INDEX 330
int mt9p111_use_vfs_af_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_af_settings_tbl[MT9P111_MAX_VFS_AF_INDEX];
uint16_t mt9p111_vfs_af_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_af_settings_tbl);
 
#define MT9P111_MAX_VFS_OEM_INDEX 330
int mt9p111_use_vfs_oem_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_oem_settings_tbl[MT9P111_MAX_VFS_OEM_INDEX];
uint16_t mt9p111_vfs_oem_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_oem_settings_tbl);
 
#define MT9P111_MAX_VFS_WRITEREG_INDEX 330
int mt9p111_use_vfs_writereg_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_writereg_settings_tbl[MT9P111_MAX_VFS_WRITEREG_INDEX];
uint16_t mt9p111_vfs_writereg_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_writereg_settings_tbl);

#define MT9P111_MAX_VFS_GETREG_INDEX 330
int mt9p111_use_vfs_getreg_setting=0;
struct mt9p111_i2c_reg_conf mt9p111_vfs_getreg_settings_tbl[MT9P111_MAX_VFS_GETREG_INDEX];
uint16_t mt9p111_vfs_getreg_settings_tbl_size= ARRAY_SIZE(mt9p111_vfs_getreg_settings_tbl);
#endif

/*=============================================================
    EXTERNAL DECLARATIONS
==============================================================*/
extern struct mt9p111_reg mt9p111_regs;

struct pm8058_gpio af_en = {
      .direction      = PM_GPIO_DIR_OUT,
      .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
      .output_value   = 1,
      .pull           = PM_GPIO_PULL_NO,
      .out_strength   = PM_GPIO_STRENGTH_HIGH,
      .function       = PM_GPIO_FUNC_NORMAL,
};
struct pm8058_gpio af_dis = {
      .direction      = PM_GPIO_DIR_OUT,
      .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
      .output_value   = 0,
      .pull           = PM_GPIO_PULL_NO,
      .out_strength   = PM_GPIO_STRENGTH_LOW,
      .function       = PM_GPIO_FUNC_NORMAL,
};

static unsigned int mt9p111_supported_effects[] = {
    CAMERA_EFFECT_OFF,
    CAMERA_EFFECT_MONO,
    CAMERA_EFFECT_SEPIA,
    CAMERA_EFFECT_NEGATIVE,
    CAMERA_EFFECT_SOLARIZE
};

static unsigned int mt9p111_supported_autoexposure[] = {
    CAMERA_AEC_FRAME_AVERAGE,
    CAMERA_AEC_CENTER_WEIGHTED,
    CAMERA_AEC_SPOT_METERING
};

static unsigned int mt9p111_supported_antibanding[] = {
    CAMERA_ANTIBANDING_OFF,
    CAMERA_ANTIBANDING_60HZ,
    CAMERA_ANTIBANDING_50HZ,
    CAMERA_ANTIBANDING_AUTO,
};

static unsigned int mt9p111_supported_wb[] = {
    CAMERA_WB_AUTO ,
    CAMERA_WB_INCANDESCENT,
    CAMERA_WB_FLUORESCENT,
    CAMERA_WB_DAYLIGHT,
    CAMERA_WB_CLOUDY_DAYLIGHT,
};

static unsigned int mt9p111_supported_led[] = {
    LED_MODE_OFF,
    LED_MODE_AUTO,
    LED_MODE_ON,
    LED_MODE_TORCH,//SW2-6--CL-Camera-flashmode-00+
};
static unsigned int mt9p111_supported_ISO[] = {
    CAMERA_ISO_ISO_AUTO,
    CAMERA_ISO_ISO_100,
    CAMERA_ISO_ISO_200,
    CAMERA_ISO_ISO_400,
    CAMERA_ISO_ISO_800,
};

static unsigned int mt9p111_supported_focus[] = {
    AF_MODE_AUTO,
    AF_MODE_NORMAL
};
static unsigned int mt9p111_supported_lensshade[] = {
    FALSE
};

static unsigned int mt9p111_supported_scenemode[] = {
    CAMERA_BESTSHOT_OFF
};

static unsigned int mt9p111_supported_continuous_af[] = {
    //TRUE,
    FALSE
};

static unsigned int mt9p111_supported_touchafaec[] = {
    TRUE,
    FALSE
};

static unsigned int mt9p111_supported_frame_rate_modes[] = {
    FPS_MODE_AUTO
};

struct sensor_parameters mt9p111_parameters = {
    .autoexposuretbl = mt9p111_supported_autoexposure,
    .autoexposuretbl_size = sizeof(mt9p111_supported_autoexposure)/sizeof(unsigned int),
    .effectstbl = mt9p111_supported_effects,
    .effectstbl_size = sizeof(mt9p111_supported_effects)/sizeof(unsigned int),
    .wbtbl = mt9p111_supported_wb,
    .wbtbl_size = sizeof(mt9p111_supported_wb)/sizeof(unsigned int),
    .antibandingtbl = mt9p111_supported_antibanding,
    .antibandingtbl_size = sizeof(mt9p111_supported_antibanding)/sizeof(unsigned int),
    .flashtbl = mt9p111_supported_led,
    .flashtbl_size = sizeof(mt9p111_supported_led)/sizeof(unsigned int),
    .focustbl = mt9p111_supported_focus,
    .focustbl_size = sizeof(mt9p111_supported_focus)/sizeof(unsigned int),
    .ISOtbl = mt9p111_supported_ISO,
    .ISOtbl_size = sizeof( mt9p111_supported_ISO)/sizeof(unsigned int),
    .lensshadetbl = mt9p111_supported_lensshade,
    .lensshadetbl_size = sizeof(mt9p111_supported_lensshade)/sizeof(unsigned int),
    .scenemodetbl = mt9p111_supported_scenemode,
    .scenemodetbl_size = sizeof( mt9p111_supported_scenemode)/sizeof(unsigned int),
    .continuous_aftbl = mt9p111_supported_continuous_af,
    .continuous_aftbl_size = sizeof(mt9p111_supported_continuous_af)/sizeof(unsigned int),
    .touchafaectbl = mt9p111_supported_touchafaec,
    .touchafaectbl_size = sizeof(mt9p111_supported_touchafaec)/sizeof(unsigned int),
    .frame_rate_modestbl = mt9p111_supported_frame_rate_modes,
    .frame_rate_modestbl_size = sizeof( mt9p111_supported_frame_rate_modes)/sizeof(unsigned int),
    .max_brightness = 6,
    .max_contrast = 4,
    .max_saturation = 4,
    .max_sharpness = 4,
    .min_brightness = 0,
    .min_contrast = 0,
    .min_saturation = 0,
    .min_sharpness = 0,
};

static int mt9p111_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
    struct i2c_msg msgs[] = {
        {
            .addr   = saddr,
            .flags = 0,
            .len   = 2,
            .buf   = rxdata,
        },
        {
            .addr   = saddr,
            .flags = I2C_M_RD,
            .len   = length,
            .buf   = rxdata,
        },
    };

    if (i2c_transfer(mt9p111_client->adapter, msgs, 2) < 0) {
        printk(KERN_ERR "mt9p111_msg: mt9p111_i2c_rxdata failed!\n");
        return -EIO;
    }

    return 0;
}

static int32_t mt9p111_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum mt9p111_width width)
{
    int32_t rc = 0;
    unsigned char buf[4];

    if (!rdata)
        return -EIO;

    memset(buf, 0, sizeof(buf));

    switch (width) {
        case WORD_LEN: {
            buf[0] = (raddr & 0xFF00)>>8;
            buf[1] = (raddr & 0x00FF);

            rc = mt9p111_i2c_rxdata(saddr, buf, 2);
            if (rc < 0)
                return rc;

            *rdata = buf[0] << 8 | buf[1];
        }
            break;

        case BYTE_LEN: {
            buf[0] = (raddr & 0xFF00)>>8;
            buf[1] = (raddr & 0x00FF);

            rc = mt9p111_i2c_rxdata(saddr, buf, 2);

            if (rc < 0)
                return rc;

            *rdata = buf[0];
        }
            break;

        default:
            break;
    }

    if (rc < 0)
        printk("mt9p111_i2c_read failed!\n");

    return rc;
}

//Div2-SW6-MM-MC-ImplementAutoDetectOTP-00+{
#ifdef CONFIG_MT9P111_AUTO_DETECT_OTP
static int32_t mt9p111_i2c_read_poll(unsigned short   saddr, unsigned short raddr, 
                                                                unsigned short *rdata, enum mt9p111_width width,
                                                                int poll_time_ms, int poll_count)
{
    int rc;
    int count = 0;
    uint16_t bytePoll = 0;
    
    do {
        count++;
        msleep(poll_time_ms);
        rc = mt9p111_i2c_read(saddr, raddr, &bytePoll, width);
        if (rc < 0)
            return rc;
        printk("mt9p111_i2c_read_poll: Read Back!!, raddr = 0x%x, rdata = 0x%x.\n", raddr, bytePoll);
    } while( (bytePoll != (*rdata)) && (count <poll_count) );

    *rdata = bytePoll;

    return rc;
}
#endif
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-00+}

static int32_t mt9p111_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
    struct i2c_msg msg[] = {
        {
            .addr = saddr,
            .flags = 0,
            .len = length,
            .buf = txdata,
        },
    };

    if (i2c_transfer(mt9p111_client->adapter, msg, 1) < 0) {
        printk(KERN_ERR "mt9p111_msg: mt9p111_i2c_txdata failed!\n");
        return -EIO;
    }

    return 0;
}

static int32_t mt9p111_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9p111_width width)
{
    int32_t rc = -EIO;
    int count = 0;
    unsigned char buf[4];
    unsigned short bytePoll;

    memset(buf, 0, sizeof(buf));
    switch (width) {
        case WORD_LEN: {
            buf[0] = (waddr & 0xFF00)>>8;
            buf[1] = (waddr & 0x00FF);
            buf[2] = (wdata & 0xFF00)>>8;
            buf[3] = (wdata & 0x00FF);

            rc = mt9p111_i2c_txdata(saddr, buf, 4);
        }
            break;

        case BYTE_LEN: {
            buf[0] = (waddr & 0xFF00)>>8;
            buf[1] = (waddr & 0x00FF);
            buf[2] = (wdata & 0xFF);

            rc = mt9p111_i2c_txdata(saddr, buf, 3);
        }
            break;

        case WORD_POLL: {
            do {
                count++;
                msleep(40);
                rc = mt9p111_i2c_read(saddr, waddr, &bytePoll, WORD_LEN);
                printk("[Debug Info] Read Back!!, raddr = 0x%x, rdata = 0x%x.\n", waddr, bytePoll);
            } while( (bytePoll != wdata) && (count <20) );
        }
            break;

        case BYTE_POLL: {
            do {
                count++;
                msleep(40);
                rc = mt9p111_i2c_read(saddr, waddr, &bytePoll, BYTE_LEN);
                printk("[Debug Info] Read Back!!, raddr = 0x%x, rdata = 0x%x.\n", waddr, bytePoll);
            } while( (bytePoll != wdata) && (count <20) );
        }
            break;

        default:
            break;
    }

    if (rc < 0)
        printk("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);

    return rc;
}

static int32_t mt9p111_i2c_write_table(
	struct mt9p111_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
    int i;
    int32_t rc = 0;

    for (i = 0; i < num_of_items_in_table; i++) {
        rc = mt9p111_i2c_write(mt9p111_client->addr,
            reg_conf_tbl->waddr, reg_conf_tbl->wdata,
                reg_conf_tbl->width);
        if (rc < 0)
            return rc;
        if (reg_conf_tbl->mdelay_time != 0)
            msleep(reg_conf_tbl->mdelay_time);

        reg_conf_tbl++;
    }

    return rc;
}

#ifdef CONFIG_FIH_AAT1272
static int32_t aat1272_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
    struct i2c_msg msg[] = {
        {
            .addr = saddr,
            .flags = 0,
            .len = length,
            .buf = txdata,
        },
    };

    if (i2c_transfer(aat1272_client->adapter, msg, 1) < 0) {
        printk(KERN_ERR "aat1272_msg: aat1272_i2c_txdata failed, try again!\n");
        msleep(1000);
        printk(KERN_ERR "aat1272_msg: delay 1s to retry i2c.\n");
        if (i2c_transfer(aat1272_client->adapter, msg, 1) < 0) {
            printk(KERN_ERR "aat1272_msg: aat1272_i2c_txdata failed twice, try again.\n");
            msleep(2000);
            printk(KERN_ERR "aat1272_msg: delay 2s to retry i2c.\n");
            if (i2c_transfer(aat1272_client->adapter, msg, 1) < 0) {
                printk(KERN_ERR "aat1272_msg: aat1272_i2c_txdata failed.\n");
                return -EIO;
            }
        }
    }

    return 0;
}

static int32_t aat1272_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9p111_width width)
{
    int32_t rc = -EIO;
    unsigned char buf[4];

    memset(buf, 0, sizeof(buf));

    switch (width) 
    {
        case AAT1272_LEN: 
        {
            buf[0] = (uint8_t) waddr;
            buf[1] = (uint8_t) wdata;
            rc = aat1272_i2c_txdata(saddr, buf, 2);
        }
            break;

        default:
            break;
    }

    if (rc < 0)
        printk("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);

    return rc;
}
#endif

#if defined(CONFIG_MT9P111_DYNPGA) || defined(CONFIG_MT9P111_AUTO_DETECT_OTP)//Div2-SW6-MM-MC-ImplementAutoDetectOTP-00*
unsigned short Float16toRegister(int32_t f16val)
{
    unsigned short bit4to0, bit14to5, bit15;
    unsigned short Reg16;
    // for debug
    CDBG("Float16toRegister: f16val=%d\n", f16val);

    // 1. bit15
    if (f16val < 0)
    { 
        f16val*= -1;
        bit15 = 1;
    }
    else
        bit15 = 0;

    // 2. bit 4 ~ 0
    bit4to0 = 10;
    while(f16val >= 2048)
    {
        f16val=f16val >> 1;
        bit4to0++;
    }

    // 3. bit 14 ~ 5
    bit14to5 = (unsigned short)(f16val -1024);

    // for debug
    CDBG("Float16toRegister: bit15=0x%x, bit14to5=0x%x, bit4to0=0x%x\n", bit15, bit14to5, bit4to0);

    // Merge to Register
    Reg16 = ((bit15<<15)&0x8000) | ((bit14to5<<5) & 0x7FE0) | (bit4to0 & 0x001F);
    return Reg16;
}
    
int32_t RegistertoFloat16(unsigned short reg16)
{
    int32_t f16val;
    int32_t fbit15, fbit14to5, fbit4to0;
    if(reg16>>15)
        fbit15 = -1;
    else fbit15 = 1;

    fbit14to5 = ((reg16>>5)&0x3FF) + 1024;
    fbit4to0 = (reg16&0x1F) - 10;
    // for debug
    CDBG("RegistertoFloat16: fbit15=%d, fbit14to5=%d, fbit4to0=%d\n", fbit15, fbit14to5, fbit4to0);

    if (fbit4to0 < 0)
        return 0;
    else
    {
        f16val = (fbit14to5 << fbit4to0) * fbit15;
        CDBG("RegistertoFloat16 return: f16val=%d\n", f16val);
        return f16val;
    }
}

    /*
 *  This fuction must be called at once after preview start.
 *  Initialize the LSC parameters from Sensor register for dynamic LSC.
 */
int32_t mt9p111_dynPGA_init(void)
{
    struct pga_struct pgaIndoor, pgaOutdoor;
    int32_t rc;
    unsigned short reg_data;
    printk("start mt9p111_dynPGA_init Setting.\n");

    // Read from Sensor for indoor-LSC
    rc = mt9p111_i2c_read(mt9p111_client->addr,0x3644, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.g1_p0q2 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:g1_p0q2=0x%x\n", pgaIndoor.g1_p0q2);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x364E, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.r_p0q2 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:r_p0q2=0x%x\n", pgaIndoor.r_p0q2);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x3656, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.b_p0q1 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:b_p0q1=0x%x\n", pgaIndoor.b_p0q1);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x3658, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.b_p0q2 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:b_p0q2=0x%x\n", pgaIndoor.b_p0q2);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x3662, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.g2_p0q2 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:g2_p0q2=0x%x\n", pgaIndoor.g2_p0q2);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x368A, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.r_p1q0 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:r_p1q0=0x%x\n", pgaIndoor.r_p1q0);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x3694, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.b_p1q0 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:b_p1q0=0x%x\n", pgaIndoor.b_p1q0);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x36C0, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.g1_p2q0 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:g1_p2q0=0x%x\n", pgaIndoor.g1_p2q0);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x36CA, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.r_p2q0 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:r_p2q0=0x%x\n", pgaIndoor.r_p2q0);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x36D4, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.b_p2q0 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:b_p2q0=0x%x\n", pgaIndoor.b_p2q0);

    rc = mt9p111_i2c_read(mt9p111_client->addr,0x36DE, &reg_data, WORD_LEN);
    if (rc < 0) return rc;
    else pgaIndoor.g2_p2q0 = RegistertoFloat16(reg_data);
    printk("dynPGA_init read:g2_p2q0=0x%x\n", pgaIndoor.g2_p2q0);

    // Calculation outdoor LSC from Indoor LSC
    pgaOutdoor.g1_p0q2 = pgaIndoor.g1_p0q2 * 9 / 10;
    pgaOutdoor.r_p0q2 = pgaIndoor.r_p0q2 * 11 / 10;

    pgaOutdoor.b_p0q1 = pgaIndoor.b_p0q1 + 3808;

    pgaOutdoor.b_p0q2 = pgaIndoor.b_p0q2 * 111 / 100;
    pgaOutdoor.g2_p0q2 = pgaIndoor.g2_p0q2 * 9 / 10;

    pgaOutdoor.r_p1q0 = pgaIndoor.r_p1q0 + 616;
    pgaOutdoor.b_p1q0 = pgaIndoor.b_p1q0 + 5243;

    pgaOutdoor.g1_p2q0 = pgaIndoor.g1_p2q0 * 23 / 25;
    pgaOutdoor.r_p2q0 = pgaIndoor.r_p2q0 * 231 / 200;
    pgaOutdoor.b_p2q0 = pgaIndoor.b_p2q0 * 127 / 125;
    pgaOutdoor.g2_p2q0 = pgaIndoor.g2_p2q0 * 23 / 25;

    // just for changing lens-shading parameters from indoor to outdoor
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x3644, Float16toRegister(pgaOutdoor.g1_p0q2), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x364E, Float16toRegister(pgaOutdoor.r_p0q2), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x3656, Float16toRegister(pgaOutdoor.b_p0q1), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x3658, Float16toRegister(pgaOutdoor.b_p0q2), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x3662, Float16toRegister(pgaOutdoor.g2_p0q2), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x368A, Float16toRegister(pgaOutdoor.r_p1q0), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x3694, Float16toRegister(pgaOutdoor.b_p1q0), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x36C0, Float16toRegister(pgaOutdoor.g1_p2q0), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x36CA, Float16toRegister(pgaOutdoor.r_p2q0), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x36D4, Float16toRegister(pgaOutdoor.b_p2q0), WORD_LEN);
    if (rc < 0) return rc;
    rc = mt9p111_i2c_write(mt9p111_client->addr,0x36DE, Float16toRegister(pgaOutdoor.g2_p2q0), WORD_LEN);
    if (rc < 0) return rc;	
    printk("finish mt9p111_dynPGA_init Setting.\n");

    return 1;
}
#endif

//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+{
#ifdef CONFIG_MT9P111_AUTO_DETECT_OTP
int32_t mt9p111_set_door_mode(int mode)
{
    long rc = 0;
    
    if (now_door_mode == mode)
    {
        printk("[LC & OTP] Ignore: Because now_door_mode = new_mode = %d \n", mode);
        return rc;
    }
    else
    {
        printk("[LC & OTP] Start setting new door_mode = %d.......\n", mode);
    }

    if (mode == cam_lc_indoor)
    {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.lctbl[0], mt9p111_regs.lctbl_size);
            if (rc < 0)
                return rc; 

            printk("[LC & OTP] Loading Lens Correction Indoor Setting...Done~\n");
    }
    else if (mode == cam_lc_outdoor)
    {
            // Here to loading LC table for outdoor mode.
            rc = mt9p111_i2c_write_table(&mt9p111_regs.lcdltbl[0], mt9p111_regs.lcdltbl_size);
            if (rc < 0)
                return rc; 
            printk("[LC & OTP] Loading Lens Correction Outdoor Setting...Done~\n");
    }
    else if (mode == cam_lc_yellow_light)
    {
            // Here to loading LC table for yellow light.
            rc = mt9p111_i2c_write_table(&mt9p111_regs.lcyltbl[0], mt9p111_regs.lcyltbl_size);
            if (rc < 0)
                return rc; 
            printk("[LC & OTP] Loading Lens Correction Yellow Light Setting...Done~\n");
    }
    else if (mode == cam_otp_outdoor)
    {
            rc = mt9p111_dynPGA_init();
            if (rc < 0)
                return rc;

            //SW5-Multimedia-TH-MT9P111LensCorrection-00+{
            rc = mt9p111_i2c_write_table(&mt9p111_regs.otpdltbl[0], mt9p111_regs.otpdltbl_size);
            if (rc < 0)
                return rc; 
            //SW5-Multimedia-TH-MT9P111LensCorrection-00+}
            
            printk("[LC & OTP]Loading OTP outdoor mode...Done~\n");
    }
    else if (mode == cam_otp_indoor)
    {
            rc = mt9p111_i2c_write(mt9p111_client->addr,0xD004, 0x04, BYTE_LEN); // PGA_SOLUTION
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0xD006, 0x0008, WORD_LEN);// PGA_ZONE_ADDR_0
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0xD005, 0x00, BYTE_LEN);// PGA_CURRENT_ZONE
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0xD002, 0x8002, WORD_LEN); // PGA_ALGO
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x3210, 0x49B8, WORD_LEN);// COLOR_PIPELINE_CONTROL
            if (rc < 0)
                return rc;

            //SW5-Multimedia-TH-MT9P111LensCorrection-00+{
            rc = mt9p111_i2c_write_table(&mt9p111_regs.otptbl[0], mt9p111_regs.otptbl_size);
            if (rc < 0)
                return rc; 
            //SW5-Multimedia-TH-MT9P111LensCorrection-00+}
            
            printk("[LC & OTP] Loading OTP indoor mode...Done~ \n");
    }
    else
    {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.lctbl[0], mt9p111_regs.lctbl_size);
            if (rc < 0)
                return rc; 

            printk("[LC & OTP] Loading Lens Correction Indoor Setting...Done~ (For Default Setting)\n");
    }

    now_door_mode = mode;
    printk("[LC & OTP] mt9p111_set_door_mode()...Done. \n");
    return rc;
}
#endif
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+}

static long mt9p111_reg_init(void)
{
    long rc;
    // ======================================================
    /* Initial Setting */
    // ======================================================

#ifdef MT9P111_USE_VFS
    if (mt9p111_use_vfs_init_setting)  
        rc = mt9p111_i2c_write_table(&mt9p111_vfs_init_settings_tbl[0],	mt9p111_vfs_init_settings_tbl_size); 
    else
#endif	
        rc = mt9p111_i2c_write_table(&mt9p111_regs.inittbl[0], mt9p111_regs.inittbl_size);

    if (rc < 0)
        return rc;
    else
    {
        if (mt9p111info->sensor_Orientation == MSM_CAMERA_SENSOR_ORIENTATION_270) 
        {
            //Here to setting sensor orientation for HW design.
            //Preview orientation.
            rc =	mt9p111_i2c_write(mt9p111_client->addr, 0xC850, 0x03, BYTE_LEN);
            if (rc < 0)
                return rc;
            rc =	mt9p111_i2c_write(mt9p111_client->addr, 0xC851, 0x03, BYTE_LEN);
            if (rc < 0)
                return rc;
            //Snapshot orientation.
            rc =	mt9p111_i2c_write(mt9p111_client->addr, 0xC888, 0x03, BYTE_LEN);
            if (rc < 0)
                return rc;
            rc =	mt9p111_i2c_write(mt9p111_client->addr, 0xC889, 0x03, BYTE_LEN);
            if (rc < 0)
                return rc;

            rc =	mt9p111_i2c_write(mt9p111_client->addr, 0x8404, 0x06, BYTE_LEN);
            if (rc < 0)
                return rc;
            printk("Finish Orientation Setting.\n");
        }
        printk("Finish Initial Setting.\n");
    }
    return 0;
}

static long mt9p111_reg_init2(void)
{
    long rc;

#ifdef CONFIG_MT9P111_DYNPGA
    uint16_t lc_data = 0;
    uint16_t useDNPforOTP=0;
    int product_phase = 0;
    int pid = 0;
    pid = fih_get_product_id();
    product_phase = fih_get_product_phase();

    // ======================================================
    /* Lens Correction Setting */
    // ======================================================
#ifdef MT9P111_USE_VFS
    if (mt9p111_use_vfs_lens_setting)  
        rc = mt9p111_i2c_write_table(&mt9p111_vfs_lens_settings_tbl[0], mt9p111_vfs_lens_settings_tbl_size); 
    else
#endif
    {
        if ((pid==Product_FB0)&&(product_phase == Product_PR1))
        {
                rc = mt9p111_i2c_write_table(&mt9p111_regs.lctbl[0], mt9p111_regs.lctbl_size);

                if (rc < 0)
                    return rc;  
                else
                    printk("Finish Lens Correction Setting.\n");
        }
        else
        {
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x381C, 0x0000, WORD_LEN);
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0xE02A, 0x0001, WORD_LEN);
            if (rc < 0)
                return rc;
            do{
                msleep(40);

                rc = mt9p111_i2c_read(mt9p111_client->addr, 0xE023, &lc_data, BYTE_LEN);
                if(rc < 0)
                    return rc;
                printk("mt9p111  lc_data = 0x%x\n", lc_data);
                }while((lc_data&0x40) == 0);

            if(lc_data!=0xC1)  // changed by mcnex
            {
                rc = mt9p111_i2c_write_table(&mt9p111_regs.lctbl[0], mt9p111_regs.lctbl_size);

                if (rc < 0)
                    return rc;  
                else
                    printk("Finish Lens Correction Setting.\n");
            }
            else  // lc_data should be 0xC1 for OTP LSC loading
            {
                rc = mt9p111_i2c_write(mt9p111_client->addr,0xE024, 0x0000, WORD_LEN); // IO_NV_MEM_ADDR
                rc = mt9p111_i2c_write(mt9p111_client->addr, 0xE02A, 0xA010, WORD_LEN);// IO_NV_MEM_COMMAND ¡V READ Command
                msleep(100);
                rc = mt9p111_i2c_read(mt9p111_client->addr, 0xE026, &useDNPforOTP, WORD_LEN); // ID Read
                printk("mt9p111 useDNPforOTP = 0x%x\n", useDNPforOTP); // for debug

                rc = mt9p111_i2c_write(mt9p111_client->addr,0xD004, 0x04, BYTE_LEN); // PGA_SOLUTION
                if (rc < 0)
                    return rc;
                rc = mt9p111_i2c_write(mt9p111_client->addr,0xD006, 0x0008, WORD_LEN);// PGA_ZONE_ADDR_0
                if (rc < 0)
                    return rc;
                rc = mt9p111_i2c_write(mt9p111_client->addr,0xD005, 0x00, BYTE_LEN);// PGA_CURRENT_ZONE
                if (rc < 0)
                    return rc;
                rc = mt9p111_i2c_write(mt9p111_client->addr,0xD002, 0x8002, WORD_LEN); // PGA_ALGO
                if (rc < 0)
                    return rc;
                rc = mt9p111_i2c_write(mt9p111_client->addr,0x3210, 0x49B8, WORD_LEN);// COLOR_PIPELINE_CONTROL
                if (rc < 0)
                    return rc;
                // added by mcnex
                msleep(100);
                if (useDNPforOTP != 0x5100) // 0x5100 means OTP light source is DNP
                    mt9p111_dynPGA_init();
            }
        }
    }
#else//else for "#ifdef CONFIG_MT9P111_DYNPGA"

#ifdef MT9P111_USE_VFS
        if (mt9p111_use_vfs_lens_setting)
            rc = mt9p111_i2c_write_table(&mt9p111_vfs_lens_settings_tbl[0], mt9p111_vfs_lens_settings_tbl_size);
        else
#endif
        {
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+{
#ifdef CONFIG_MT9P111_AUTO_DETECT_OTP
            uint16_t Poll_R0xE023_val = 0;
    
            //LOAD = Lens Correction 90% 04/29/10 12:58:28
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x381C, 0x0000, WORD_LEN); //Aptina suggestion to force the OTPM completely initialized before applying the PGA settings.
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0xE02A, 0x0001, WORD_LEN); // IO_NV_MEM_COMMAND
            if (rc < 0)
                return rc;

            /* Poll for detect OTP */
            Poll_R0xE023_val = 0xC1;
            mt9p111_i2c_read_poll(mt9p111_client->addr, 0xE023, &Poll_R0xE023_val, BYTE_LEN, 40,20);
            printk("[Auto detect OTP] Poll_R0xE023_val = 0x%x. \n", Poll_R0xE023_val);

            /* Auto select mode base on poll value*/
            if (Poll_R0xE023_val != 0xC1)
            {
                mt9p111_enable_auto_detect_otp = 0;
                door_mode = cam_lc_indoor;
            }
            else
            {
                mt9p111_enable_auto_detect_otp = 1;
                printk("[Auto detect OTP] Detect OTP success...\n");
                door_mode = cam_otp_indoor;
            }

            now_door_mode = FIH_MODE_MAX; 
            rc =  mt9p111_set_door_mode(door_mode);
            if (rc < 0)
                return rc; 
            
#else//else for "#ifdef CONFIG_MT9P111_AUTO_DETECT_OTP"
            rc = mt9p111_i2c_write_table(&mt9p111_regs.lctbl[0], mt9p111_regs.lctbl_size);

            if (rc < 0)
                return rc;  
            else
                printk("Finish Lens Correction Setting.\n");
#endif//End for "#ifdef CONFIG_MT9P111_AUTO_DETECT_OTP"
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+}
        }
#endif//End for "#ifdef CONFIG_MT9P111_DYNPGA"

    // ======================================================
    /* Char Setting */
    // ======================================================
#ifdef MT9P111_USE_VFS
    if (mt9p111_use_vfs_char_setting)  
        rc = mt9p111_i2c_write_table(&mt9p111_vfs_char_settings_tbl[0], mt9p111_vfs_char_settings_tbl_size);
    else
#endif
        rc = mt9p111_i2c_write_table(&mt9p111_regs.chartbl[0], mt9p111_regs.chartbl_size);

    if (rc < 0)
        return rc;  
    else
        printk("Finish Char Setting.\n");
    // ======================================================
    /* Image Quality Setting */
    // ======================================================
#ifdef MT9P111_USE_VFS
    if (mt9p111_use_vfs_iq1_setting)
    {
        rc = mt9p111_i2c_write_table(&mt9p111_vfs_iq1_settings_tbl[0],  mt9p111_vfs_iq1_settings_tbl_size); 
        rc = mt9p111_i2c_write_table(&mt9p111_vfs_iq2_settings_tbl[0],  mt9p111_vfs_iq2_settings_tbl_size); 
    }
    else
#endif
        rc = mt9p111_i2c_write_table(&mt9p111_regs.iqtbl[0], mt9p111_regs.iqtbl_size);

    if (rc < 0)
        return rc;
    else
        printk("Finish Image Quality Setting.\n");
    // ======================================================
    /* AF Setting */
    // ======================================================   
#ifdef MT9P111_USE_VFS
    if (mt9p111_use_vfs_af_setting)
        rc = mt9p111_i2c_write_table(&mt9p111_vfs_af_settings_tbl[0],   mt9p111_vfs_af_settings_tbl_size);  
    else
#endif
        rc = mt9p111_i2c_write_table(&mt9p111_regs.aftbl[0], mt9p111_regs.aftbl_size);
    if (rc < 0)
        return rc;  
    else
        printk("Finish AF Setting.\n");

    mt9p111_init_done=1;

    return 0;
}

static long mt9p111_set_antibanding(int mode, int antibanding)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_antibanding_mode==antibanding)
    {
        printk("mt9p111_set_antibanding, old = %d, new = %d\n",mt9p111_antibanding_mode, antibanding);
        return 0;
    }

    mt9p111_antibanding_mode=antibanding;
    printk("mt9p111_set_antibanding, mode = %d, antibanding = %d\n",mode, antibanding);		//CDBG
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (antibanding) 
    {
        case CAMERA_ANTIBANDING_OFF:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.ab_off_tbl[0], mt9p111_regs.ab_off_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_ANTIBANDING_60HZ:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.ab_60mhz_tbl[0], mt9p111_regs.ab_60mhz_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_ANTIBANDING_50HZ:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.ab_50mhz_tbl[0], mt9p111_regs.ab_50mhz_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_ANTIBANDING_AUTO:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.ab_auto_tbl[0], mt9p111_regs.ab_auto_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        default: 
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_brightness(int mode, int brightness)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_brightness_mode==brightness)
    {
        printk("mt9p111_set_brightness, old = %d, new = %d\n",mt9p111_brightness_mode, brightness);
        return 0;
    }

    mt9p111_brightness_mode=brightness;

    printk("mt9p111_set_brightness, mode = %d, brightness = %d\n",mode, brightness);//CDBG
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (brightness) 
    {
        case CAMERA_BRIGHTNESS_0: //[EV 0]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.br0_tbl[0], mt9p111_regs.br0_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_BRIGHTNESS_1: //[EV 1]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.br1_tbl[0], mt9p111_regs.br1_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_BRIGHTNESS_2: //[EV 2]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.br2_tbl[0], mt9p111_regs.br2_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_BRIGHTNESS_3: //[EV 3 (default)]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.br3_tbl[0], mt9p111_regs.br3_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_BRIGHTNESS_4: //[EV 4]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.br4_tbl[0], mt9p111_regs.br4_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_BRIGHTNESS_5: //[EV 5]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.br5_tbl[0], mt9p111_regs.br5_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_BRIGHTNESS_6: //[EV +3]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.br6_tbl[0], mt9p111_regs.br6_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        default: 
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_contrast(int mode, int contrast)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_contrast_mode==contrast)
    {
        printk("mt9p111_set_contrast, old = %d, new = %d\n",mt9p111_contrast_mode, contrast);
        return 0;
    }

    mt9p111_contrast_mode=contrast;

    printk("mt9p111_set_contrast, mode = %d, contrast = %d\n",
        mode, contrast);	//CDBG
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (contrast)
    {
        case CAMERA_CONTRAST_MINUS2: //[Const -2]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.const_m2_tbl[0], mt9p111_regs.const_m2_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_CONTRAST_MINUS1: //[Const -1]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.const_m1_tbl[0], mt9p111_regs.const_m1_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_CONTRAST_ZERO: //[Const 0 (default)]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.const_zero_tbl[0], mt9p111_regs.const_zero_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_CONTRAST_POSITIVE1: //[Const +1]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.const_p1_tbl[0], mt9p111_regs.const_p1_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_CONTRAST_POSITIVE2: //[Const +2]
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.const_p2_tbl[0], mt9p111_regs.const_p2_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        default:
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_effect(int mode, int effect)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_effect_mode==effect)
    {
        printk("mt9p111_set_effect, old = %d, new = %d\n",
        mt9p111_effect_mode, effect);
        return 0;
    }

    mt9p111_effect_mode=effect;

    printk("mt9p111_set_effect, mode = %d, effect = %d\n",
        mode, effect);		//CDBG
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (effect) 
    {
        case CAMERA_EFFECT_OFF:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.effect_none_tbl[0], mt9p111_regs.effect_none_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_EFFECT_MONO:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.effect_mono_tbl[0], mt9p111_regs.effect_mono_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_EFFECT_SEPIA:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.effect_sepia_tbl[0], mt9p111_regs.effect_sepia_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_EFFECT_NEGATIVE:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.effect_negative_tbl[0], mt9p111_regs.effect_negative_tbl_size);
            if (rc < 0)
                return rc;
        }
        break;

        case CAMERA_EFFECT_SOLARIZE:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.effect_solarize_tbl[0], mt9p111_regs.effect_solarize_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_EFFECT_COLORBAR:
        {
            printk("%s: case = CAMERA_EFFECT_COLORBAR ~~ \n", __func__);

            /* Enable Test Mode */
            rc =	mt9p111_i2c_write(mt9p111_client->addr, 0x3070, 0x03, WORD_LEN);
            if (rc < 0)
            {
                printk("mt9p111_set_effect: ERR: Enable test mode fail ! \n");
                return rc;
            }
            printk("mt9p111_set_effect: Finish enable Test Mode.\n");

        }
            break;

        default: 
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_exposure_mode(int mode, int exp_mode)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_touchAEC_mode)
    {
        printk("mt9p111 touchAEC enabled skip mt9p111_set_exposure_mode\n");
        return 0;
    }

    if(mt9p111_exposure_mode==exp_mode)
    {
        printk("mt9p111_set_exposure_mode, old = %d, new = %d\n",
        mt9p111_exposure_mode, exp_mode);
        return 0;
    }

    mt9p111_exposure_mode=exp_mode;

    printk("mt9p111_set_exposure_mode, mode = %d, exp_mode = %d\n",
        mode, exp_mode);//CDBG
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (exp_mode) 
    {
        case CAMERA_AEC_FRAME_AVERAGE://EXPOSURE_AVERAGE
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.exp_average_tbl[0], mt9p111_regs.exp_average_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_AEC_CENTER_WEIGHTED://EXPOSURE_NORMAL
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.exp_normal_tbl[0], mt9p111_regs.exp_normal_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_AEC_SPOT_METERING://EXPOSURE_SPOT
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.exp_spot_tbl[0], mt9p111_regs.exp_spot_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        default: 
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_saturation(int mode, int satu)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_saturation_mode==satu)
    {
        printk("mt9p111_set_saturation, old = %d, new = %d\n",
        mt9p111_saturation_mode, satu);
        return 0;
    }

    mt9p111_saturation_mode=satu;

    printk("mt9p111_set_saturation, mode = %d, satu = %d\n",
        mode, satu);
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (satu) 
    {
        case CAMERA_SATURATION_MINUS2:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.satu_m2_tbl[0], mt9p111_regs.satu_m2_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SATURATION_MINUS1:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.satu_m1_tbl[0], mt9p111_regs.satu_m1_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SATURATION_ZERO:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.satu_zero_tbl[0], mt9p111_regs.satu_zero_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SATURATION_POSITIVE1:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.satu_p1_tbl[0], mt9p111_regs.satu_p1_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SATURATION_POSITIVE2:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.satu_p2_tbl[0], mt9p111_regs.satu_p2_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        default: 
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_sharpness(int mode, int sharpness)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_sharpness_mode==sharpness)
    {
        printk("mt9p111_set_sharpness, old = %d, new = %d\n",
        mt9p111_sharpness_mode, sharpness);
        return 0;
    }

    mt9p111_sharpness_mode=sharpness;

    printk("mt9p111_set_sharpness, mode = %d, sharpness = %d\n",
        mode, sharpness);
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (sharpness) 
    {
        case CAMERA_SHARPNESS_MINUS2:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.sharp_m2_tbl[0], mt9p111_regs.sharp_m2_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SHARPNESS_MINUS1:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.sharp_m1_tbl[0], mt9p111_regs.sharp_m1_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SHARPNESS_ZERO:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.sharp_zero_tbl[0], mt9p111_regs.sharp_zero_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SHARPNESS_POSITIVE1:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.sharp_p1_tbl[0], mt9p111_regs.sharp_p1_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_SHARPNESS_POSITIVE2:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.sharp_p2_tbl[0], mt9p111_regs.sharp_p2_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        default: 
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_whitebalance(int mode, int whitebalace)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_whitebalance_mode==whitebalace)
    {
        printk("mt9p111_set_whitebalance, old = %d, new = %d\n",
        mt9p111_whitebalance_mode, whitebalace);
        return 0;
    }

    mt9p111_whitebalance_mode=whitebalace;

    printk("mt9p111_set_whitebalace, mode = %d, whitebalace = %d\n",
        mode, whitebalace);
    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (whitebalace)
    {
        case CAMERA_WB_AUTO:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.wb_auto_tbl[0], mt9p111_regs.wb_auto_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_WB_DAYLIGHT:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.wb_daylight_tbl[0], mt9p111_regs.wb_daylight_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_WB_CLOUDY_DAYLIGHT:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.wb_cloudy_daylight_tbl[0], mt9p111_regs.wb_cloudy_daylight_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_WB_INCANDESCENT: 
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.wb_incandescent_tbl[0], mt9p111_regs.wb_incandescent_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_WB_FLUORESCENT: 
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.wb_fluorescent_tbl[0], mt9p111_regs.wb_fluorescent_tbl_size);
            if (rc < 0)
                return rc;
        } 
            break;

        default: 
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_ledmod(int mode, int ledmod)
{
    long rc = 0;
    printk("mt9p111_set_ledmod, mode = %d, ledmod = %d\n",mode, ledmod);
    //SW2-6--CL-Camera-flashmode-00*{
    switch (ledmod)
    {
        case LED_MODE_OFF:
        case LED_MODE_ON:
        case LED_MODE_AUTO:
        {
            mt9p111_m_ledmod = ledmod;
#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-ImplementFlashLedOfffunctionForSF4H8.B-168-00*
            rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 0);
#endif
        }
            break;

        case LED_MODE_TORCH:
        {
            mt9p111_m_ledmod = 3;
//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*{           
#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)
            rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 1);

#ifdef CONFIG_FIH_AAT1272
            //Write Reg 1 with 0x33 to let AAT1272 enter into Movie mode and flash with default brightness
            rc = aat1272_i2c_write(aat1272_client->addr, 0x01, 0x34, AAT1272_LEN);//SW2-6--CL-Camera-flashmode-01*
#endif
#endif
//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*}
        }
            break;

        default:
            return -EINVAL;	
    }
    //SW2-6--CL-Camera-flashmode-00*}
    return rc;
}

static enum hrtimer_restart mt9p111_flashled_timer_func(struct hrtimer *timer)
{
    printk(KERN_INFO "mt9p111_msg: mt9p111_flashled_timer_func mt9p111_m_ledmod = %d\n", mt9p111_m_ledmod);	
    complete(&mt9p111_flashled_comp);

    return HRTIMER_NORESTART;
}

static int mt9p111_flashled_off_thread(void *arg)
{
#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*
    int rc = 0;
#endif
    int pid = 0;
    pid = fih_get_product_id();

    printk(KERN_INFO "mt9p111_msg: mt9p111_flashled_off_thread running\n");
    daemonize("mt9p111_flashled_off_thread");
    while (1)
    {
        wait_for_completion(&mt9p111_flashled_comp);
        printk(KERN_INFO "%s: Got complete signal\n", __func__);

#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*

//Div2-SW6-MM-MC-ModifyFlashLedControlModeFromMovieToFlashForSF5-01*{
#if defined(CONFIG_AAT1272_ENABLE_FLASH_MODE) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*
        // Enter into flash mode and flash led.
        rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, FLASHLED_ID, 1);
        if (rc)
            return -EPERM;
        
        mdelay(mt9p111info->flash_main_waittime);
        printk(KERN_INFO "%s: Delay %d ms for flash led done ~ \n", __func__,mt9p111info->flash_main_waittime);


        // Exit  into flash mode.
        rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, FLASHLED_ID, 0);
        if (rc)
            return -EPERM;
            
#else//Use movie mode.

        if(pid==Product_FD1)//For FD1
        {
            rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 1);
            rc = aat1272_i2c_write(aat1272_client->addr, 0x00, 0x00, AAT1272_LEN);
            if (rc < 0)
                return rc;
            rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, FLASHLED_ID, 1);
            mdelay(mt9p111info->flash_main_waittime);
            rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, FLASHLED_ID, 0);
        }
        else
        {
            //Write Reg 1 with 0x33 to let AAT1272 enter into Movie mode and flash with default brightness
            rc = aat1272_i2c_write(aat1272_client->addr, 0x01, 0x30, AAT1272_LEN);
            if (rc < 0)
                return rc;

            mdelay(mt9p111info->flash_main_waittime);
        }
#endif
//Div2-SW6-MM-MC-ModifyFlashLedControlModeFromMovieToFlashForSF5-01*}

        rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 0);
        if (rc)
            return -EPERM;
#endif //End for "defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)"

    }

    return 0;
}
//Div2-SW6-MM-HL-Camera-CAF-00+{
static long mt9p111_set_CAF(int mode,int CAF)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_CAF_mode == CAF)
    {
        printk("mt9p111_set_CAF, old = %d, new = %d\n", mt9p111_CAF_mode, CAF);
        return 0;
    }

    printk("mt9p111_set_CAF set = %d\n",CAF);

    mt9p111_CAF_mode=CAF;

    mt9p111_parameters_changed_count++;

    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);

    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    if(mt9p111_CAF_mode==1)
    {
        if(mt9p111info->AF_pmic_en_pin!=0xffff)
            pm8058_gpio_config(mt9p111info->AF_pmic_en_pin, &af_en);
        mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x8419, WORD_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0x8419, 0x03, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0xB004, 0x10, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0x8404, 0x06, BYTE_LEN);
    }
    else if (mt9p111_CAF_mode==0)
    {
        if(mt9p111info->AF_pmic_en_pin!=0xffff)
                pm8058_gpio_config(mt9p111info->AF_pmic_en_pin, &af_dis);
        mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x8419, WORD_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0x8419, 0x05, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0xB004, 0x02, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0x8404, 0x06, BYTE_LEN);
    } 

    return rc;
}
//Div2-SW6-MM-HL-Camera-CAF-00+}
static long mt9p111_set_autofocus(int mode,int focus_set)
{
    long rc = 0;
    uint16_t f_state = 0;
    uint16_t retry_count=0;
//SW5-Multimedia-TH-MT9P111ReAFTest-00+{
#ifdef CONFIG_MT9P111_FAST_AF_SPECIAL_CONDITION
    uint16_t AF_Mode = 0;
    uint16_t Poll_R0xB000_val = 0;
    uint16_t AF_Brightness = 0;
#endif
//SW5-Multimedia-TH-MT9P111ReAFTest-00+}

    printk("mt9p111_set_autofocus set=%d\n",focus_set);
    if(focus_set==1)
    {
    
//SW5-Multimedia-TH-MT9P111ReAFTest-00+{       
#ifdef CONFIG_MT9P111_FAST_AF_SPECIAL_CONDITION
        rc = mt9p111_i2c_write(mt9p111_client->addr,0x098E, 0x8419, BYTE_LEN);
        if (rc < 0)
            return rc;
        rc = mt9p111_i2c_read(mt9p111_client->addr,0x8419, &AF_Mode, BYTE_LEN);
        if (rc < 0)
            return rc;
        printk("mt9p111 Fast 1.AF_Mode = 0x%x\n", AF_Mode);
        if (AF_Mode != 0x04)
        {
            //Set Fast AF
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x8419, 0x04, BYTE_LEN);
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x8404, 0x06, BYTE_LEN);
            if (rc < 0)
                return rc;
            printk("mt9p111 Fast 2.Fast mode\n");
        }
#endif
//SW5-Multimedia-TH-MT9P111ReAFTest-00+}
    
        if(mt9p111info->AF_pmic_en_pin!=0xffff)
            pm8058_gpio_config(mt9p111info->AF_pmic_en_pin, &af_en);

#ifdef MT9P111_USE_VFS
        if (mt9p111_use_vfs_aftrigger_setting)  
            rc = mt9p111_i2c_write_table(&mt9p111_vfs_aftrigger_settings_tbl[0],	mt9p111_vfs_aftrigger_settings_tbl_size);
        else
#endif
            rc = mt9p111_i2c_write_table(&mt9p111_regs.aftrigger_tbl[0], mt9p111_regs.aftrigger_tbl_size);

        if (rc < 0)
            return rc;
        else
            printk("Finish AF-trigger.\n");

        do 
        {
            msleep(100);

            rc = mt9p111_i2c_read(mt9p111_client->addr, 0xB006, &f_state, BYTE_LEN);
            if (rc < 0)
                return rc;

            printk("mt9p111_set_autofocus_state = 0x%x\n", f_state);
            retry_count++;
        } while((f_state != 0x00)&&(retry_count<50));
        printk("mt9p111_set_autofocus_retry_count = %d\n", retry_count);
        
//SW5-Multimedia-TH-MT9P111ReAFTest-00+{
#ifdef CONFIG_MT9P111_FAST_AF_SPECIAL_CONDITION
        //Read AF PASS or Fail
        rc = mt9p111_i2c_write(mt9p111_client->addr,0x098E, 0x3000, WORD_LEN);
        if (rc < 0)
            return rc;
        Poll_R0xB000_val = 0x0010;
        rc = mt9p111_i2c_read_poll(mt9p111_client->addr, 0xB000, &Poll_R0xB000_val, WORD_LEN, 40,5);
        if (rc < 0)
            return rc; 
        printk("mt9p111 Fast 3.Poll_R0xB000_val = 0x%x\n", Poll_R0xB000_val);
        
        if(Poll_R0xB000_val != 0x0010)
        {
            //Read Brightness
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x098E, 0x380C, WORD_LEN);
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0xB80C, &AF_Brightness, WORD_LEN);
            if(rc < 0)
                return rc;
            printk("mt9p111 Fast 4.AF_0xB80C = 0x%x\n", AF_Brightness);

            //Set Manual AF
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x098E, 0x8419, BYTE_LEN);
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x8419, 0x01, BYTE_LEN);
            if (rc < 0)
                return rc;
            rc = mt9p111_i2c_write(mt9p111_client->addr,0x8404, 0x06, BYTE_LEN);
            if (rc < 0)
                return rc;
            printk("mt9p111 Fast 5.Manual mode\n");

            rc = mt9p111_i2c_write(mt9p111_client->addr,0x098E, 0xB007, WORD_LEN);
            if (rc < 0)
                return rc;

            if (AF_Brightness >= mt9p111info->fast_af_retest_target)
            {
                //Low Brightness
                rc = mt9p111_i2c_write(mt9p111_client->addr, 0xB007, 0x20, BYTE_LEN);
                if(rc < 0)
                    return rc;  
                printk("mt9p111 Fast 6.AF on Low Brightness\n");
            }
            else
            {
                //High Brightness
                rc = mt9p111_i2c_write(mt9p111_client->addr, 0xB007, 0x00, BYTE_LEN);
                if(rc < 0)
                    return rc;  
                printk("mt9p111 Fast 7.AF on High Brightness\n");
            }
        }
        mdelay(50);
#endif
//SW5-Multimedia-TH-MT9P111ReAFTest-00+}

    }
    else
    {
        rc = mt9p111_i2c_write(mt9p111_client->addr,	0xB006, 0x00, BYTE_LEN);
        if (rc < 0)
            return rc;
    }
    return rc;
}

static long mt9p111_set_focusrec(int x,int y,int w,int h)
{
    long rc = 1;
    int8_t r_x,r_y,r_w,r_h;

    printk("mt9p111_set_focusrec x=%d,y=%d,w=%d,h=%d\n",x,y,w,h);

    if((x<0)||(y<0)||((x+w)>mt9p111_FRAME_WIDTH)||((y+h)>mt9p111_FRAME_HEIGHT))
        return 0;

    if((mt9p111_rectangle_x==x)&&(mt9p111_rectangle_y==y)&&(mt9p111_rectangle_w==w)&&(mt9p111_rectangle_h==h))
        return rc;
    mt9p111_rectangle_x=x;
    mt9p111_rectangle_y=y;
    mt9p111_rectangle_w=w;
    mt9p111_rectangle_h=h;
    mt9p111_parameters_changed_count++;

    r_x=(int8_t)(256 * x /mt9p111_FRAME_WIDTH );
    r_y=(int8_t)(256 * y /mt9p111_FRAME_HEIGHT );
    r_w=(int8_t)(256 * w /mt9p111_FRAME_WIDTH -1);
    r_h=(int8_t)(256 * h /mt9p111_FRAME_HEIGHT -1);

    CDBG("mt9p111_set_focusrec r_x=%x,r_y=%x,r_w=%x,r_h=%x\n",r_x,r_y,r_w,r_h);

    mt9p111_i2c_write(mt9p111_client->addr, 0xB854, r_x, BYTE_LEN);
    mt9p111_i2c_write(mt9p111_client->addr, 0xB855, r_y, BYTE_LEN);
    mt9p111_i2c_write(mt9p111_client->addr, 0xB856, r_w, BYTE_LEN);
    mt9p111_i2c_write(mt9p111_client->addr, 0xB857, r_h, BYTE_LEN);
    mt9p111_i2c_write(mt9p111_client->addr, 0x8404, 0x06, BYTE_LEN);
    return rc;
}

static long mt9p111_set_touchAEC(int enable,uint32_t x,uint32_t y)
{
    long rc = 1;
    int8_t r_x,r_y;

    printk("mt9p111_set_touchAEC enable=%d, x=%d,y=%d\n",enable,x,y);

    if(enable)
    {
        x=x-60;
        y=y-60;
        if(x<=0)
            x=0;
        if(y<=0)
            y=0;
        r_x=(int8_t)(256 * x /mt9p111_FRAME_WIDTH );
        r_y=(int8_t)(256 * y /mt9p111_FRAME_HEIGHT );
        printk("mt9p111_set_touchAEC r_x=%x,r_y=%x\n",r_x,r_y);

        mt9p111_parameters_changed_count++;
        mt9p111_touchAEC_mode=enable;

        mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0xB820, WORD_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0xB820, r_x, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0xB821, r_y, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0xB822, 0x33, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0xB823, 0x33, BYTE_LEN);
        mt9p111_i2c_write(mt9p111_client->addr, 0x8404, 0x06, BYTE_LEN);
    }
    else
    {
        if(mt9p111_touchAEC_mode==enable)
            return rc;
        mt9p111_touchAEC_mode=enable;
        mt9p111_set_exposure_mode(0,-1);
        mt9p111_set_exposure_mode(0,mt9p111_exposure_mode);
    }

    return rc;
}

static int mt9p111_set_strobe_flash(void)
{
    long rc = 0;
    uint16_t flash_target_addr_val = 0;

    if (mt9p111info->flash_target_addr == 0xB80C)
    {
        rc = mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x380C, WORD_LEN);
        if (rc < 0)
            return rc;
    }
    
    rc = mt9p111_i2c_read(mt9p111_client->addr, mt9p111info->flash_target_addr, &flash_target_addr_val, WORD_LEN);
    if (rc < 0)
        return rc;
    printk("mt9p111_set_strobe_flash: mt9p111_m_ledmod = %d, flash_target_addr(0x%x) = %x.\n", mt9p111_m_ledmod, mt9p111info->flash_target_addr, flash_target_addr_val);

    if ((mt9p111_m_ledmod == 2) || ((mt9p111_m_ledmod == 1) && (flash_target_addr_val >= mt9p111info->flash_target)))
    {
        hrtimer_cancel(&mt9p111_flashled_timer);
        hrtimer_start(&mt9p111_flashled_timer,
        ktime_set(mt9p111info->flash_main_starttime/ 1000, (mt9p111info->flash_main_starttime % 1000) * 1000000), HRTIMER_MODE_REL);
        if (hrtimer_active(&mt9p111_flashled_timer))
            printk(KERN_INFO "%s: TIMER running\n", __func__);
    }
    return rc;
}

static int mt9p111_set_iso(camera_iso_mode iso)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;

    if(mt9p111_iso_mode == iso)
    {
        printk("mt9p111_set_iso, old = %d, new = %d\n", mt9p111_iso_mode, iso);
        return 0;
    }
    mt9p111_iso_mode = iso;
    printk("mt9p111_set_iso, iso = %d\n", iso);
    mt9p111_parameters_changed_count++;

    
    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);
    if((seq_state != 0x03)&&(seq_state != 0x07))
    {
        do 
        {
            msleep(30);
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
            printk("mt9p111  seq_state = 0x%x\n", seq_state);
            retry_count++;
        } 
        while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
    }

    switch (iso) 
    {
        case CAMERA_ISO_ISO_AUTO:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.iso_auto_tbl[0], mt9p111_regs.iso_auto_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_ISO_ISO_100:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.iso_100_tbl[0], mt9p111_regs.iso_100_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_ISO_ISO_200:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.iso_200_tbl[0], mt9p111_regs.iso_200_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_ISO_ISO_400:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.iso_400_tbl[0], mt9p111_regs.iso_400_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        case CAMERA_ISO_ISO_800:
        {
            rc = mt9p111_i2c_write_table(&mt9p111_regs.iso_800_tbl[0], mt9p111_regs.iso_800_tbl_size);
            if (rc < 0)
                return rc;
        }
            break;

        default:
            return -EINVAL;
    }

    return rc;
}

static long mt9p111_set_sensor_mode(int mode)
{
    long rc = 0;
    uint16_t seq_state = 0;
    uint16_t retry_count=0;
    int pid = 0;
    
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+{
#ifdef CONFIG_MT9P111_AUTO_SWITCH_INDOOR_AND_OUTDOOR
    uint16_t Poll_R0x3012_val = 0;
    uint16_t Poll_R0xAC08_val = 0;
#endif     
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+}

    pid = fih_get_product_id();

    printk("mt9p111_set_sensor_mode, mode = %d\n",mode);
    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
    printk("mt9p111  seq_state = 0x%x\n", seq_state);

    switch (mode) 
    {
        case SENSOR_PREVIEW_MODE:
        {

//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+{
#ifdef CONFIG_MT9P111_AUTO_SWITCH_INDOOR_AND_OUTDOOR

            /* Poll for indoor/outdoor mode */
            rc = mt9p111_i2c_read_poll(mt9p111_client->addr, 0x3012, &Poll_R0x3012_val, WORD_LEN, 40,3);
            if (rc < 0)
                return rc; 
            printk("mt9p111_set_sensor_mode: [LC & OTP] Poll_R0x3012_val = 0x%x. \n", Poll_R0x3012_val);
            rc = mt9p111_i2c_read_poll(mt9p111_client->addr, 0xAC08, &Poll_R0xAC08_val, BYTE_LEN, 40,3);
            if (rc < 0)
                return rc; 
            printk("mt9p111_set_sensor_mode: [LC & OTP] Poll_R0xAC08_val = 0x%x. \n", Poll_R0xAC08_val);
            

            /* Auto select mode base on poll value*/
            if (mt9p111_enable_auto_detect_otp != 1)
            {
                //SW5-Multimedia-TH-LCSetting-01+{
                //if ((Poll_R0x3012_val < 0x0100 && Poll_R0xAC08_val > 0x5a) || Poll_R0x3012_val <= 0x65)
                if (Poll_R0x3012_val <= 0x0200)
                //SW5-Multimedia-TH-LCSetting-01+}
                {
                    door_mode = cam_lc_outdoor;
                }
                else
                {
                    door_mode = cam_lc_indoor;
                }
            }
            else
            {
                //SW5-Multimedia-TH-LCSetting-00+{
                //if ((Poll_R0x3012_val < 0x0100 && Poll_R0xAC08_val > 0x5a) || Poll_R0x3012_val <= 0x65)
                if (Poll_R0x3012_val <= 0x200)
                {
                    door_mode = cam_otp_outdoor;
                }
                //SW5-Multimedia-TH-LCSetting-00+}
                else
                {
                    door_mode = cam_otp_indoor;
                }
            }
            
            //SW5-Multimedia-TH-LCSetting-01+{
            //if (Poll_R0xAC08_val == 0)
            //{
            //        door_mode = cam_lc_yellow_light;
            //}
            //SW5-Multimedia-TH-LCSetting-01+}
            
            rc =  mt9p111_set_door_mode(door_mode);
            if (rc < 0)
                return rc;
            
#endif
//Div2-SW6-MM-MC-ImplementAutoDetectOTP-01+}

            if(mt9p111_snapshoted==0)
                return 0;
            mt9p111_snapshoted=0;

            printk(KERN_ERR "mt9p111_msg: case SENSOR_PREVIEW_MODE.\n");

            if(mt9p111_parameters_changed_count>0)
            {
                msleep(250);
                do 
                {
                    msleep(30);

                    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);

                    printk("mt9p111  seq_state = 0x%x\n", seq_state);
                    retry_count++;
                } 
                while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
                printk("mt9p111_set_sensor_mode waiting  retry_count = %d\n", retry_count);
                mt9p111_parameters_changed_count=0;
            }

            else if((seq_state != 0x03)&&(seq_state != 0x07))
            {
                retry_count=0;
                do 
                {
                    msleep(30);

                    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
                    if (rc < 0)
                        printk("mt9p111_set_sensor_mode i2c error \n");

                    printk("mt9p111  seq_state = 0x%x\n", seq_state);
                    retry_count++;
                } 
                while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
                printk("mt9p111_set_preconfig  retry_count = %d\n", retry_count);
            }

#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*
            if ((mt9p111_m_ledmod==2) || ((mt9p111_m_ledmod == 1) && (flash_target_addr_val >= mt9p111info->flash_target)))
            {
                if (flash_target_addr_val >= mt9p111info->flash_target) 
                {
                    printk("mt9p111_msg: PREVIEW_MODE: ledmod = %d, flash_target_addr(0x%x) = %x.\n", mt9p111_m_ledmod, mt9p111info->flash_target_addr, flash_target_addr_val);
                    //Change AWB & Effect setting back if LED is lit
                    mt9p111_set_whitebalance(0, wb_tmp);
                    mt9p111_set_brightness(0, brightness_tmp);// For brightness setting
                }
            }
#endif

        // ======================================================
        /* Context B to A */
        // ======================================================			
            rc = mt9p111_i2c_write_table(&mt9p111_regs.context_b_to_a_tbl[0], mt9p111_regs.context_b_to_a_tbl_size);
            if (rc < 0)
                return rc;
            else
                printk("Finish Context B to A.\n");

            msleep(100);
            retry_count=0;
            do 
            {
                msleep(50);

                rc = mt9p111_i2c_read(mt9p111_client->addr,	0x8405, &seq_state, BYTE_LEN);
                if (rc < 0)
                    printk("mt9p111 SENSOR_PREVIEW_MODE i2c error \n");

                printk("mt9p111 Preview seq_state = 0x%x\n", seq_state);
                retry_count++;
            } 
            while((seq_state != 0x03)&&(retry_count<50));
            printk("mt9p111 SENSOR_PREVIEW_MODE retry_count = %d\n", retry_count);

            printk(KERN_ERR "mt9p111_msg: case SENSOR_PREVIEW_MODE end~~~~~~.\n");
        }
            break;

        case SENSOR_SNAPSHOT_MODE:
        {
            printk(KERN_ERR "mt9p111_msg: case SENSOR_SNAPSHOT_MODE.\n");
            mt9p111_snapshoted=1;
            if(mt9p111_parameters_changed_count>0)
            {
                msleep(250);
                do 
                {
                    msleep(30);

                    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);

                    printk("mt9p111  seq_state = 0x%x\n", seq_state);
                    retry_count++;
                } 
                while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
                printk("mt9p111_set_sensor_mode waiting retry_count = %d\n", retry_count);

                mt9p111_parameters_changed_count=0;
            }

            else if((seq_state != 0x03)&&(seq_state != 0x07))
            {
                retry_count=0;
                do 
                {
                    msleep(30);

                    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
                    if (rc < 0)
                        printk("mt9p111_set_sensor_mode i2c error \n");

                    printk("mt9p111  seq_state = 0x%x\n", seq_state);
                    retry_count++;
                } 
                while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
                printk("mt9p111_set_preconfig  retry_count = %d\n", retry_count);
            }

#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*

            if (mt9p111info->flash_target_addr == 0xB80C)
            {
                rc = mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x380C, WORD_LEN);
                if (rc < 0)
                    return rc;
            }
    
            rc = mt9p111_i2c_read(mt9p111_client->addr, mt9p111info->flash_target_addr, &flash_target_addr_val, WORD_LEN);
            if (rc < 0)
                return rc;
            printk("mt9p111_msg: SNAPSHOT_MODE: ledmod = %d, flash_target_addr(0x%x) = %x.\n", mt9p111_m_ledmod, mt9p111info->flash_target_addr, flash_target_addr_val);

            if ((mt9p111_m_ledmod==2) || ((mt9p111_m_ledmod == 1) && (flash_target_addr_val >= mt9p111info->flash_target)))
            {
                if(pid==Product_FD1)
                {
                    if (((mt9p111_m_ledmod==2) && (flash_target_addr_val >= 0x76C)) || ((mt9p111_m_ledmod == 1) && (flash_target_addr_val >= mt9p111info->flash_target)))
                    {
                        //Change AWB & Effect setting first when turn on LED
                        wb_tmp = mt9p111_whitebalance_mode;
                        mt9p111_whitebalance_mode = -1;
                        mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0xACB0, WORD_LEN);
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB0, 0x38, BYTE_LEN);	// AWB_RG_MIN
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB1, 0x4C, BYTE_LEN);	// AWB_RG_MAX
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB4, 0x38, BYTE_LEN);	// AWB_BG_MIN
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB5, 0x4C, BYTE_LEN);	// AWB_BG_MAX
                        mt9p111_i2c_write(mt9p111_client->addr, 0xAC04, 0x3E, BYTE_LEN); // AWB_PRE_AWB_R2G_RATIO
                        mt9p111_i2c_write(mt9p111_client->addr, 0xAC05, 0x48, BYTE_LEN); // AWB_PRE_AWB_B2G_RATIO
                        brightness_tmp =mt9p111_brightness_mode;
                        mt9p111_brightness_mode = -1;

                        mt9p111_i2c_write(mt9p111_client->addr, 0xA409, mt9p111info->flash_bright, BYTE_LEN); // For brightness setting
                    }
                }
                else
                {
                    if (flash_target_addr_val >= mt9p111info->flash_target)
                    {
                        //Change AWB & Effect setting first when turn on LED
                        wb_tmp = mt9p111_whitebalance_mode;
                        mt9p111_whitebalance_mode = -1;
                        mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0xACB0, WORD_LEN);
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB0, 0x38, BYTE_LEN);	// AWB_RG_MIN
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB1, 0x4C, BYTE_LEN);	// AWB_RG_MAX
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB4, 0x38, BYTE_LEN);	// AWB_BG_MIN
                        mt9p111_i2c_write(mt9p111_client->addr, 0xACB5, 0x4C, BYTE_LEN);	// AWB_BG_MAX
                        mt9p111_i2c_write(mt9p111_client->addr, 0xAC04, 0x3E, BYTE_LEN); // AWB_PRE_AWB_R2G_RATIO
                        mt9p111_i2c_write(mt9p111_client->addr, 0xAC05, 0x48, BYTE_LEN); // AWB_PRE_AWB_B2G_RATIO
                        brightness_tmp =mt9p111_brightness_mode;
                        mt9p111_brightness_mode = -1;

                        mt9p111_i2c_write(mt9p111_client->addr, 0xA409, mt9p111info->flash_bright, BYTE_LEN); // For brightness setting
                    }
                }

                gpio_tlmm_config(GPIO_CFG(mt9p111info->GPIO_FLASHLED_DRV_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

                rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 1);
                if (rc)
                    return -EPERM;
                
#ifdef CONFIG_FIH_AAT1272
                //Write Reg 1 with 0x33 to let AAT1272 enter into Movie mode and flash with default brightness
                rc = aat1272_i2c_write(aat1272_client->addr, 0x01, 0x31, AAT1272_LEN);	 //0x33
                if (rc < 0)
                    return rc;
#endif
                mdelay(mt9p111info->flash_second_waittime);

            }
#endif

            // ======================================================
            /* Context A to B */
            // ======================================================
            rc = mt9p111_i2c_write_table(&mt9p111_regs.context_a_to_b_tbl[0], mt9p111_regs.context_a_to_b_tbl_size);
            if (rc < 0)
                return rc;
            else
                printk("Finish Context A to B.\n");

            mdelay(100);
            retry_count=0;
            do 
            {
                mdelay(50);

                rc = mt9p111_i2c_read(mt9p111_client->addr,	0x8405, &seq_state, BYTE_LEN);
                if (rc < 0)
                    printk("mt9p111 Snapshot i2c error \n");

                printk("mt9p111 Snapshot seq_state = 0x%x\n", seq_state);
                retry_count++;
            } while((seq_state != 0x07)&&(retry_count<50));
            printk("mt9p111 SENSOR_SNAPSHOT_MODE retry_count = %d\n", retry_count);

#if defined(CONFIG_FIH_AAT1272) || defined(CONFIG_FIH_FITI_FP6773)//Div2-SW6-MM-MC-AddFitiFp6773FlashLed-00*
            if ((mt9p111_m_ledmod==2) || ((mt9p111_m_ledmod == 1) && (flash_target_addr_val >= mt9p111info->flash_target)))
            {
                hrtimer_cancel(&mt9p111_flashled_timer);
                hrtimer_start(&mt9p111_flashled_timer,
                ktime_set(mt9p111info->flash_main_starttime / 1000, (mt9p111info->flash_main_starttime % 1000) * 1000000),
                    HRTIMER_MODE_REL);

                if (hrtimer_active(&mt9p111_flashled_timer))
                    printk(KERN_INFO "%s: TIMER running\n", __func__);
                if(pid==Product_FD1)//For FD1
                {
                    rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 0);
                    mdelay(120);
                }
            }
#endif

            printk(KERN_ERR "mt9p111_msg: case SENSOR_SNAPSHOT_MODE end~~~~~~~~~~.\n");
        }
            break;

        case SENSOR_RAW_SNAPSHOT_MODE:
        {
            printk(KERN_ERR "mt9p111_msg: case SENSOR_RAW_SNAPSHOT_MODE.\n");
            mt9p111_snapshoted=1;
            if(mt9p111_parameters_changed_count>0)
            {
                msleep(250);
                do 
                {
                    msleep(30);

                    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);

                    printk("mt9p111  seq_state = 0x%x\n", seq_state);
                    retry_count++;
                } 
                while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
                printk("mt9p111_set_sensor_mode  waiting retry_count = %d\n", retry_count);

                mt9p111_parameters_changed_count=0;
            }

            else if((seq_state != 0x03)&&(seq_state != 0x07))
            {
                retry_count=0;
                do 
                {
                    msleep(30);

                    rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
                    if (rc < 0)
                        printk("mt9p111_set_sensor_mode i2c error \n");

                    printk("mt9p111  seq_state = 0x%x\n", seq_state);
                    retry_count++;
                } 
                while(((seq_state != 0x03)&&(seq_state != 0x07))&&(retry_count<50));
                printk("mt9p111_set_preconfig  retry_count = %d\n", retry_count);
            }

#ifdef CONFIG_FIH_FTM
            //Div2-SW6-MM-MC-ImplementFTMCameraTestMode-00*{
            if ((mt9p111_effect_mode != CAMERA_EFFECT_COLORBAR)&&(mt9p111_ftm_af_Enable == 1))//Div2-SW6-MM-MC-ReduceTestTime-01*
            {
                rc = mt9p111_set_autofocus(1, 1);
                if (rc < 0)
                    return rc;
                printk(KERN_INFO"mt9p111_set_sensor_mode: Enable AutoFocus function for FTM~.\n");
            }
            else
            {
                printk(KERN_INFO"mt9p111_set_sensor_mode: Ignore AutoFocus because enable test mode or mt9p111_ftm_af_Enable = 0 ~.\n");
            }
            //Div2-SW6-MM-MC-ImplementFTMCameraTestMode-00*}
#endif

            // ======================================================
            /* Context A to B */
            // ======================================================
            rc = mt9p111_i2c_write_table(&mt9p111_regs.context_a_to_b_tbl[0], mt9p111_regs.context_a_to_b_tbl_size);
            if (rc < 0)
                return rc;
            else
                printk("Finish Context A to B.\n");
            msleep(50);
            retry_count=0;
            do 
            {
                msleep(50);

                rc = mt9p111_i2c_read(mt9p111_client->addr, 0x8405, &seq_state, BYTE_LEN);
                if (rc < 0)
                    printk("mt9p111 Snapshot i2c error \n");

                printk("mt9p111 Raw Snapshot seq_state = 0x%x\n", seq_state);
                retry_count++;
            } while((seq_state != 0x07)&&(retry_count<50));
            printk("mt9p111 SENSOR_RAW_SNAPSHOT_MODE retry_count = %d\n", retry_count);

#ifdef CONFIG_FIH_FTM
            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x332E, 0x02, WORD_LEN);
            if (rc < 0)
                return rc;
            else
            printk("HL: Change sensor format from CBYCRY to YCBYCR\n");
#endif
            printk(KERN_ERR "mt9p111_msg: case SENSOR_RAW_SNAPSHOT_MODE end~~~~~~~~~~.\n");
        }
            break;
        //Div2-SW6-MM-CL-mt9p111noImage-00+{
        case SENSOR_RESET_MODE:
        {
            printk("mt9p111 SENSOR_RESET_MODE E.\n");
            /* 5M Reset Pin Pull Low*/
            rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 0);
            msleep(1); 
        
            gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
            msleep(5); 
            gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
            msleep(1);
        
            rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 1);
            msleep(10);
        
            mt9p111_reg_init();
            mt9p111_reg_init2();
            printk("mt9p111 SENSOR_RESET_MODE X.\n");
        }
            break;
        //Div2-SW6-MM-CL-mt9p111noImage-00+}

        default:
            return -EINVAL;
    }

    return 0;
}


int mt9p111_power_on(void)
{
    int rc=0;

    /* Setting MCLK */
    msm_camio_clk_rate_set(24000000);
    msm_camio_camif_pad_reg_reset();
    if(mt9p111info->mclk_sw_pin!=0xffff)
    {
    /* Disable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
        /* Enable camera power*/
        if(mt9p111_vreg_vddio==0)
        {
            rc = fih_cam_vreg_control(mt9p111info->cam_vreg_vddio_id, 1800, 1);
            if (rc)
                return rc;
        }
        mt9p111_vreg_vddio=1;

        if(mt9p111_vreg_acore==0)
        {
            if (mt9p111info->cam_v2p8_en_pin==0xffff)
                rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 1);
            else
                rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 1);

            if (rc)
                return rc;
        }
        mt9p111_vreg_acore=1;

        mdelay(2);

        /* Enable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        printk(KERN_INFO "%s: Enable MCLK = 24MHz, GPIO_2MA. \n", __func__);

        /* 5M Reset Pin Pull High*/
        rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 1);
        if (rc)
            return rc;

        mdelay(10);

        return rc;
    }
    else
    {
		//SW5-Multimedia-TH-MT9P111PowerOn-00+{
		/* VGA Pwdn Pin Pull High*/
	    rc = fih_cam_output_gpio_control(mt9p111info->vga_pwdn_pin, "hm035x", 1);
        printk(KERN_INFO "mt9p111_power_on: VGA Pwdn Pin Pull High\n");
		mdelay(1);
		
		/* Enable camera power*/
        if(mt9p111_vreg_vddio==0)
        {
            rc = fih_cam_vreg_control(mt9p111info->cam_vreg_vddio_id, 1800, 1);
            if (rc)
                return rc;
        }
        mt9p111_vreg_vddio=1;
        printk(KERN_INFO "mt9p111_power_on: Enable camera power 1.8V\n");

        if(mt9p111_vreg_acore==0)
        {
            if (mt9p111info->cam_v2p8_en_pin==0xffff)
                rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 1);
            else
                rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 1);

            if (rc)
                return rc;
        }
        mt9p111_vreg_acore=1;
        printk(KERN_INFO "mt9p111_power_on: Enable camera power 2.8V\n");
        mdelay(100);//t2 = 1~500ms

        /* Enable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        printk(KERN_INFO "%s: Enable MCLK = 24MHz, GPIO_2MA. \n", __func__);
        printk(KERN_INFO "mt9p111_power_on: Enable MCLK\n");
        mdelay(2);//t3 = 70~500 MCLK

        /* 5M Reset Pin Pull High*/
        rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 1);
        if (rc)
            return rc;
        printk(KERN_INFO "mt9p111_power_on: 5M Reset Pin Pull High\n");
        mdelay(1);
        return rc;
        //SW5-Multimedia-TH-MT9P111PowerOn-00+}
    }
}

int mt9p111_power_off(void)
{
    int rc=0;
    if(mt9p111info->mclk_sw_pin!=0xffff)
    {
        /* Disable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
        printk("%s: mt9p111 disable mclk\n", __func__);

        mdelay(1);
        rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 0);
        if (rc)
            return rc;

        mdelay(5);

        /* Disable camera 1.8V power*/
        if(mt9p111_vreg_vddio==1)
        {
            rc = fih_cam_vreg_control(mt9p111info->cam_vreg_vddio_id, 1800, 0);
            if (rc)
                return rc;
        }
        mt9p111_vreg_vddio=0;

        /* Disable camera 2.8V power*/
        if(mt9p111_vreg_acore==1)
        {
            if (mt9p111info->cam_v2p8_en_pin==0xffff)
                rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 0);
            else
                rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 0);

            if (rc)
                return rc;
        }
        mt9p111_vreg_acore=0;

        return rc;

    }
    else
    {
        //SW5-Multimedia-TH-MT9P111PowerOff-00+{
        /* VGA Pwdn Pin Pull Low*/
	    rc = fih_cam_output_gpio_control(mt9p111info->vga_pwdn_pin, "hm035x", 0);
        if (rc)
            return rc;
        printk(KERN_INFO "mt9p111_power_off: VGA Pwdn Pin Pull High\n");

        /* 5M Reset Pin Pull Low*/
        rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 0);
        if (rc)
            return rc;
        printk(KERN_INFO "mt9p111_power_off: 5M Reset Pin Pull Low\n");

        /* Disable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        rc = fih_cam_output_gpio_control(mt9p111info->MCLK_PIN, "mt9p111", 0);
        if (rc)
            return rc;
        printk("%s: Disable mclk\n", __func__);
        printk(KERN_INFO "mt9p111_power_off: Disable MCLK\n");

        /* Disable camera 2.8V power*/
        if(mt9p111_vreg_acore==1)
        {
            if (mt9p111info->cam_v2p8_en_pin==0xffff)
                rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 0);
            else
                rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 0);

            if (rc)
                return rc;
        }
        mt9p111_vreg_acore=0;
        printk(KERN_INFO "mt9p111_power_off: Disable camera 2.8V power\n");

        /* Disable camera 1.8V power*/
        if(mt9p111_vreg_vddio==1)
        {
            rc = fih_cam_vreg_control(mt9p111info->cam_vreg_vddio_id, 1800, 0);
            if (rc)
                return rc;
        }
        mt9p111_vreg_vddio=0;
        printk(KERN_INFO "mt9p111_power_off: Disable camera 1.8V power\n");

        /* 5M Pwdn Pin Pull Low*/
        rc = fih_cam_output_gpio_control(mt9p111info->pwdn_pin, "mt9p111", 0);
        if (rc)
            return rc;
        printk(KERN_INFO "mt9p111_power_off: 5M Pwdn Pin Pull Low\n");

        return rc;
        //SW5-Multimedia-TH-MT9P111PowerOff-00+}
    }
}

static int mt9p111_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id = 0;
    int rc = 0;

    mutex_lock(&mt9p111_mut);
    printk("MT9P111_sensor_init_probe entry.\n");
    wake_lock_init(&mt9p111_wake_lock_suspend, WAKE_LOCK_SUSPEND, "mt9p111");
    printk("mt9p111_sensor_init_probe wake_lock.\n");
    wake_lock(&mt9p111_wake_lock_suspend);

    sensor_init_parameters(data,&mt9p111_parameters);

    /* Pull Low CAMIF_SW Pin */
    if(mt9p111info->mclk_sw_pin!=0xffff)
    {
        rc = fih_cam_output_gpio_control(mt9p111info->mclk_sw_pin, "mt9p111", 0);
        if (rc)
            goto init_probe_fail;
        mdelay(2);
    }

#ifdef CONFIG_MT9P111_STANDBY
    if(mt9p111_first_init==1)
    {
        mt9p111_first_init=0;
#endif

        /* 5M Pwdn Pin Pull Low*/
        rc = fih_cam_output_gpio_control(mt9p111info->pwdn_pin, "mt9p111", 0);
        if (rc)
            return rc;
        mdelay(10);

        /* 5M Reset Pin Pull Low*/
        rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 0);
        if (rc)
            return rc;

        mdelay(10);

        mt9p111_power_on();

        rc = mt9p111_reg_init();
        if (rc < 0)
            goto init_probe_fail;

        rc = mt9p111_i2c_read(mt9p111_client->addr,REG_MT9P111_MODEL_ID, &model_id, WORD_LEN);
        if (rc < 0 || model_id != MT9P111_MODEL_ID)//Div2-SW6-MM-MC-ImplementCameraFTMforSF8Serials-00*
            goto init_probe_fail;

        printk("mt9p111 model_id = 0x%x . \n", model_id);

        rc = mt9p111_i2c_read(mt9p111_client->addr,0x0014, &model_id, WORD_LEN);
        if (rc < 0)
            goto init_probe_fail;

        printk("mt9p111 pll_control = 0x%x\n", model_id);

        rc = mt9p111_i2c_read(mt9p111_client->addr,0x0018, &model_id, WORD_LEN);
        if (rc < 0)
            goto init_probe_fail;

        printk("mt9p111 standby_reg = 0x%x\n", model_id);

        rc = mt9p111_i2c_read(mt9p111_client->addr,0x001A, &model_id, WORD_LEN);
        if (rc < 0)
            goto init_probe_fail;

        printk("mt9p111 reset_and_misc_reg = 0x%x\n", model_id);

        rc = mt9p111_i2c_write(mt9p111_client->addr,	0x098E, 0x8405, WORD_LEN);
        if (rc < 0)
            goto init_probe_fail;

        rc = mt9p111_i2c_read(mt9p111_client->addr,0x8405, &model_id, BYTE_LEN);
        if (rc < 0)
            goto init_probe_fail;

        printk("mt9p111 seq_state_reg = 0x%x in mt9p111_sensor_init_probe() \n", model_id);

        mt9p111_exposure_mode=CAMERA_AEC_FRAME_AVERAGE;
        mt9p111_antibanding_mode=CAMERA_ANTIBANDING_AUTO;
        mt9p111_brightness_mode=CAMERA_BRIGHTNESS_3;
        mt9p111_contrast_mode=CAMERA_CONTRAST_ZERO;
        mt9p111_effect_mode=CAMERA_EFFECT_OFF;
        mt9p111_saturation_mode=CAMERA_SATURATION_ZERO;
        mt9p111_sharpness_mode=CAMERA_SHARPNESS_ZERO;
        mt9p111_whitebalance_mode=CAMERA_WB_AUTO;
        mt9p111_iso_mode=CAMERA_ISO_ISO_AUTO;

        mt9p111_rectangle_x=270;
        mt9p111_rectangle_y=190;
        mt9p111_rectangle_w=100;
        mt9p111_rectangle_h=100;
#ifdef CONFIG_MT9P111_STANDBY
        mt9p111_first_init=0;

    }
else
{
    /* Enable camera power*/
    if(mt9p111_vreg_acore==0)
    {
        if (mt9p111info->cam_v2p8_en_pin==0xffff)
            rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 1);
        else
            rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 1);

        if (rc)
            goto init_probe_fail;
    }
    mt9p111_vreg_acore=1;

    /* Input MCLK = 24MHz */
    msm_camio_clk_rate_set(24000000);
    gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    msm_camio_camif_pad_reg_reset();
    printk(KERN_INFO "%s: Input MCLK = 24MHz.\n", __func__);
    msleep(20);
    rc = mtp9111_sensor_standby(1);
    if (rc)
        goto init_probe_fail;
}
#endif
    if(mt9p111info->standby_pin!=0xffff)
        gpio_tlmm_config(GPIO_CFG(mt9p111info->standby_pin, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
    printk("mt9p111_sensor_init_probe finish.\n");
    mutex_unlock(&mt9p111_mut);
    return rc;

init_probe_fail:
#ifdef CONFIG_MT9P111_STANDBY
    mt9p111_first_init=1;
#endif
    printk("mt9p111_sensor_init_probe FAIL.\n");
    mutex_unlock(&mt9p111_mut);
    return rc;
}

int mt9p111_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc = 0;

    mt9p111_ctrl = kzalloc(sizeof(struct mt9p111_ctrl), GFP_KERNEL);
    if (!mt9p111_ctrl) {
        CDBG("mt9p111_init failed!\n");
        rc = -ENOMEM;
        goto init_done;
    }

    if (data)
        mt9p111_ctrl->sensordata = data;

    rc = mt9p111_sensor_init_probe(data);
    if (rc < 0) {
        CDBG("mt9p111_sensor_init failed!\n");
        goto init_fail;
    }

init_done:
    return rc;

init_fail:
    kfree(mt9p111_ctrl);
    return rc;
}

static int mt9p111_init_client(struct i2c_client *client)
{
    /* Initialize the MSM_CAMI2C Chip */
    init_waitqueue_head(&mt9p111_wait_queue);
    return 0;
}

int mt9p111_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long   rc = 0;

    if (copy_from_user(&cfg_data,(void *)argp,sizeof(struct sensor_cfg_data)))
        return -EFAULT;

    CDBG("mt9p111_ioctl, cfgtype = %d, mode = %d\n",cfg_data.cfgtype, cfg_data.mode);

#ifndef CONFIG_FIH_FTM

    if(cfg_data.cfgtype!=CFG_SET_MODE)
    {
        if(mt9p111_init_done==0)
        {
            mt9p111_reg_init2();
        }
    }
#else
        if(mt9p111_init_done==0)
        {
            mt9p111_reg_init2();
            return 0;
        }
#endif

    switch (cfg_data.cfgtype) 
    {
        case CFG_SET_MODE:
        {
            if(mt9p111_init_done==0)
                return 0;
            rc = mt9p111_set_sensor_mode(cfg_data.mode);
        }
            break;

        case CFG_SET_ANTIBANDING:
        {
            rc = mt9p111_set_antibanding(cfg_data.mode,cfg_data.cfg.antibanding);
        }
            break;

        case CFG_SET_BRIGHTNESS:
        {
            rc = mt9p111_set_brightness(cfg_data.mode,cfg_data.cfg.brightness);
        }
        break;

        case CFG_SET_CONTRAST:
        {
            rc = mt9p111_set_contrast(cfg_data.mode,cfg_data.cfg.contrast);
        }
            break;

        case CFG_SET_EFFECT:
        {
            rc = mt9p111_set_effect(cfg_data.mode,cfg_data.cfg.effect);
        }
            break;

        case CFG_SET_EXPOSURE_MODE:
        {
            rc = mt9p111_set_exposure_mode(cfg_data.mode,cfg_data.cfg.exposuremod);
        }
            break;

        case CFG_SET_SATURATION:
        {
            rc = mt9p111_set_saturation(cfg_data.mode,cfg_data.cfg.saturation);
        }
            break;

        case CFG_SET_SHARPNESS:
        {
            rc = mt9p111_set_sharpness(cfg_data.mode,cfg_data.cfg.sharpness);
        }
            break;

        case CFG_GET_AF_MAX_STEPS:
            break;

        case CFG_SET_WB:
        {
            rc = mt9p111_set_whitebalance(cfg_data.mode,cfg_data.cfg.wb);
        }
            break;

        case CFG_SET_LEDMOD:
        {
            rc = mt9p111_set_ledmod(cfg_data.mode, cfg_data.cfg.ledmod);
        }
            break;

        case CFG_SET_CAF:
        {               
            rc = mt9p111_set_CAF(cfg_data.mode,cfg_data.cfg.CAF);//Div2-SW6-MM-HL-Camera-CAF-00+
        }
        break; 

        case CFG_SET_AUTOFOCUS:
        {               
            rc = mt9p111_set_autofocus(cfg_data.mode,cfg_data.cfg.autofocus);
        }           
        break; 

        case CFG_SET_FOCUSREC:
        {
            rc = mt9p111_set_focusrec(cfg_data.cfg.focusrec.x_upper_left,cfg_data.cfg.focusrec.y_upper_left,cfg_data.cfg.focusrec.width,cfg_data.cfg.focusrec.height);            
        }
            break;

        case CFG_STROBE_FLASH:
        {
            rc = mt9p111_set_strobe_flash();
        }
            break;

        case CFG_SET_ISO:
        {
            rc = mt9p111_set_iso(cfg_data.cfg.iso);
        }
            break;

        case CFG_SET_touchAEC:
        {
            rc = mt9p111_set_touchAEC(cfg_data.cfg.AECIndex.enable,cfg_data.cfg.AECIndex.AEC_X,cfg_data.cfg.AECIndex.AEC_Y);
        }
            break;

        default:
            break;
    }

    return rc;
}

#ifdef CONFIG_MT9P111_STANDBY
int mtp9111_sensor_standby(int onoff)
{
    int rc = 0;

    uint16_t standby_i2c = 0x00;
    uint16_t standby_i2c_state = 0;
    uint16_t en_vdd_dist_soft_state = 0;
    uint16_t retry_count=0;

    if(onoff==0)//entering stanby mode
    {
        printk("mt9p111 entering stanby mode = %d\n", onoff);

        rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0028, 0x00, WORD_LEN);
        if (rc < 0)
            return rc;
        else
        {
            rc = mt9p111_i2c_read(mt9p111_client->addr, 0x0028, &en_vdd_dist_soft_state, WORD_LEN);
            if (rc < 0)
                return rc;

            printk("mt9p111 en_vdd_dist_soft_state = 0x%x\n", en_vdd_dist_soft_state);
        }

        //================= 0x0018 =====================================
        rc = mt9p111_i2c_read(mt9p111_client->addr, 0x0018, &standby_i2c_state, WORD_LEN);
        if (rc < 0)
            return rc;

        printk("mt9p111 standby_i2c_state = 0x%x\n", standby_i2c_state);
        standby_i2c=standby_i2c_state|0x01;

        rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0018, standby_i2c, WORD_LEN);
        if (rc < 0)
            return rc;
        else
        {
            do 
            {
                msleep(50);
                rc = mt9p111_i2c_read(mt9p111_client->addr, 0x0018, &standby_i2c_state, WORD_LEN);
                if(rc < 0)
                    return rc;
                 retry_count++;
                printk("mt9p111 standby_i2c_state = 0x%x\n", standby_i2c_state);
            } 
            while(((standby_i2c_state&0x4000)==0)&&(retry_count<15));
        }
        if(retry_count>=15)
            return -1;
        msleep(150);
    }

    if(onoff==1)//exiting stanby mode
    {
        printk("mt9p111 exiting stanby mode = %d\n", onoff);

        rc = mt9p111_i2c_read(mt9p111_client->addr, 0x0018, &standby_i2c_state, WORD_LEN);
        if (rc < 0)
            return rc;

        printk("mt9p111 read standby_i2c_state = 0x%x\n", standby_i2c_state);
        standby_i2c=standby_i2c_state&0xFFFE;

        rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0018, standby_i2c, WORD_LEN);
        if (rc < 0)
            return rc;
        rc = mt9p111_i2c_read(mt9p111_client->addr, 0x0018, &standby_i2c_state, WORD_LEN);
        if (rc < 0)
            return rc;

        printk("mt9p111 read standby_i2c_state = 0x%x\n", standby_i2c_state);

        msleep(20);
    }

    return rc;
}
#endif
int mt9p111_sensor_release(void)
{
    int rc = 0;

    mutex_lock(&mt9p111_mut);
    printk("mt9p111_sensor_release()+++\n");
    if(mt9p111info->AF_pmic_en_pin!=0xffff)
        pm8058_gpio_config(mt9p111info->AF_pmic_en_pin, &af_dis);

#ifdef CONFIG_MT9P111_STANDBY
    mt9p111_set_antibanding(SENSOR_PREVIEW_MODE,CAMERA_ANTIBANDING_OFF);
    mt9p111_set_brightness(SENSOR_PREVIEW_MODE,CAMERA_BRIGHTNESS_3);
    mt9p111_set_contrast(SENSOR_PREVIEW_MODE,CAMERA_CONTRAST_ZERO);
    mt9p111_set_effect(SENSOR_PREVIEW_MODE,CAMERA_EFFECT_OFF);
    mt9p111_set_touchAEC(0,0,0);
    mt9p111_set_exposure_mode(SENSOR_PREVIEW_MODE,CAMERA_AEC_FRAME_AVERAGE);
    mt9p111_set_saturation(SENSOR_PREVIEW_MODE,CAMERA_SATURATION_ZERO);
    mt9p111_set_sharpness(SENSOR_PREVIEW_MODE,CAMERA_SHARPNESS_ZERO);
    mt9p111_set_whitebalance(SENSOR_PREVIEW_MODE,CAMERA_WB_AUTO);
    mt9p111_set_CAF(0,0);//Div2-SW6-MM-HL-Camera-CAF-00+
    rc =mtp9111_sensor_standby(0);
    if(rc < 0 )
    {
        mt9p111_first_init=1;
        mt9p111_init_done=0;
        mt9p111_snapshoted=0;
        mt9p111_power_off();
    }
    gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
    printk("%s: mt9p111 disable mclk\n", __func__);

    /* Disable camera 2.8V power*/
    if(mt9p111info->mclk_sw_pin!=0xffff)
    {
        if(mt9p111_vreg_acore==1)
        {
            if (mt9p111info->cam_v2p8_en_pin==0xffff)
                rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 0);
            else
                rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 0);

            if (rc) {
                printk(KERN_ERR "%s: vreg disable failed (%d)\n", __func__, rc);
                mutex_unlock(&mt9p111_mut);
                return -EIO;
            }
        }
    }
    mt9p111_vreg_acore=0;

#else
        rc = mt9p111_power_off();
#endif
    kfree(mt9p111_ctrl);
    mt9p111_ctrl = NULL;

#ifndef CONFIG_MT9P111_STANDBY
    mt9p111_init_done=0;
    mt9p111_snapshoted=0;
#endif
    
    mt9p111_parameters_changed_count=0;
    printk("%s: mt9p111_sensor_release wake_unlock.\n",__func__);
    wake_unlock(&mt9p111_wake_lock_suspend);
    wake_lock_destroy(&mt9p111_wake_lock_suspend);
    printk("mt9p111_sensor_release()---\n");
    mutex_unlock(&mt9p111_mut);
    return rc;
}

#ifdef MT9P111_USE_VFS
int mt9p111_set_flash_gpio(void)
{
    int rc=0;

    rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 1);
    if (rc)
        return -EPERM;

    rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, FLASHLED_ID, 1);
    if (rc)
        return -EPERM;

    mdelay(380);//wait until maximum flash timout time (375 ms) is reached

    rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, FLASHLED_ID, 0);
    if (rc)
        return -EPERM;

    rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, 0);
    if (rc)
        return -EPERM;

    return rc;
}


void mt9p111_get_param(const char *buf, size_t count, struct mt9p111_i2c_reg_conf *tbl, 
	unsigned short tbl_size, int *use_setting, int param_num)
{
    unsigned short waddr;
    unsigned short wdata;
    enum mt9p111_width width;
    unsigned short mdelay_time;

    char param1[10],param2[10],param3[10],param4[10];
    int read_count;
    const char *pstr;
    int vfs_index=0;
    pstr=buf;

    CDBG("count=%d\n",count);

    do
    {
        if (param_num ==3)
            read_count=sscanf(pstr,"%4s,%4s,%1s",param1,param2,param3);
        else
            read_count=sscanf(pstr,"%4s,%4s,%1s,%4s",param1,param2,param3,param4);

        //CDBG("pstr=%s\n",pstr);
        //CDBG("read_count=%d,count=%d\n",read_count,count);
        if (read_count ==3 && param_num ==3)
        {
            waddr=simple_strtoul(param1,NULL,16);
            wdata=simple_strtoul(param2,NULL,16);
            width=simple_strtoul(param3,NULL,16);
            mdelay_time=0;

            tbl[vfs_index].waddr= waddr;
            tbl[vfs_index].wdata= wdata;
            tbl[vfs_index].width= width;
            tbl[vfs_index].mdelay_time= mdelay_time;
            vfs_index++;

            if (vfs_index <= tbl_size)
            {
                CDBG("Just match MAX_VFS_INDEX\n");
                *use_setting=1;
            }else if (vfs_index > tbl_size)
            {
                CDBG("Out of range MAX_VFS_INDEX\n");
                *use_setting=0;
                mt9p111_set_flash_gpio();
            break;
            }

            //CDBG("param1=%s,param2=%s,param3=%s\n",param1,param2,param3);
            //CDBG("waddr=0x%04X,wdata=0x%04X,width=%d,mdelay_time=%d,mask=0x%04X\n",waddr,wdata,width,mdelay_time,mask);
        }else if (read_count==4 && param_num ==4)
        {
            if (!strcmp(param1, "undo"))
            {
                *use_setting=0;
                break;
            }	
            waddr=simple_strtoul(param1,NULL,16);
            wdata=simple_strtoul(param2,NULL,16);
            width=simple_strtoul(param3,NULL,16);
            mdelay_time=simple_strtoul(param4,NULL,10); //Div6-PT2-MM-CH-Camera_VFS-01*	

            tbl[vfs_index].waddr= waddr;
            tbl[vfs_index].wdata= wdata;
            tbl[vfs_index].width= width;
            tbl[vfs_index].mdelay_time= mdelay_time;
            vfs_index++;

            if (vfs_index <= tbl_size)
            {
                CDBG("Just match MAX_VFS_INDEX\n");
                *use_setting=1;
            }else if (vfs_index > tbl_size)
            {
                CDBG("Out of range MAX_VFS_INDEX\n");
                *use_setting=0;
                mt9p111_set_flash_gpio(); //Div6-PT2-MM-CH-Camera_VFS-02+
                break;
            }

            //CDBG("param1=%s,param2=%s,param3=%s\n",param1,param2,param3);
            //CDBG("waddr=0x%04X,wdata=0x%04X,width=%d,mdelay_time=%d,mask=0x%04X\n",waddr,wdata,width,mdelay_time,mask);
        }else{
            tbl[vfs_index].waddr= 0xFFFF;
            tbl[vfs_index].wdata= 0xFFFF;
            tbl[vfs_index].width= 1;
            tbl[vfs_index].mdelay_time= 0; 
            *use_setting=0;
            mt9p111_set_flash_gpio(); //Div6-PT2-MM-CH-Camera_VFS-02+
            break;
        }
        /* get next line */
        pstr=strchr(pstr, '\n');
        if (pstr==NULL) 
        {
            tbl[vfs_index].waddr= 0xFFFF;
            tbl[vfs_index].wdata= 0xFFFF;
            tbl[vfs_index].width= 1;
            tbl[vfs_index].mdelay_time= 0; 
            break; 
        }
        pstr++;
    }while(read_count!=0);
}

static ssize_t mt9p111_write_prev_snap_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_prev_snap_settings_tbl[0], mt9p111_vfs_prev_snap_settings_tbl_size, &mt9p111_use_vfs_prev_snap_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_write_init_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_init_settings_tbl[0], mt9p111_vfs_init_settings_tbl_size, &mt9p111_use_vfs_init_setting, MT9P111_VFS_PARAM_NUM); 
    return count;
}

static ssize_t mt9p111_write_lens_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_lens_settings_tbl[0], mt9p111_vfs_lens_settings_tbl_size, &mt9p111_use_vfs_lens_setting, MT9P111_VFS_PARAM_NUM); 
    return count;
}

static ssize_t mt9p111_write_iq1_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_iq1_settings_tbl[0], mt9p111_vfs_iq1_settings_tbl_size, &mt9p111_use_vfs_iq1_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_write_iq2_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_iq2_settings_tbl[0], mt9p111_vfs_iq2_settings_tbl_size, &mt9p111_use_vfs_iq2_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_write_char_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_char_settings_tbl[0], mt9p111_vfs_char_settings_tbl_size, &mt9p111_use_vfs_char_setting, MT9P111_VFS_PARAM_NUM); 
    return count;
}

static ssize_t mt9p111_write_aftrigger_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_aftrigger_settings_tbl[0], mt9p111_vfs_aftrigger_settings_tbl_size, &mt9p111_use_vfs_aftrigger_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_write_patch_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_patch_settings_tbl[0], mt9p111_vfs_patch_settings_tbl_size, &mt9p111_use_vfs_patch_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_write_pll_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_pll_settings_tbl[0], mt9p111_vfs_pll_settings_tbl_size, &mt9p111_use_vfs_pll_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_write_af_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_af_settings_tbl[0], mt9p111_vfs_af_settings_tbl_size, &mt9p111_use_vfs_af_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_write_oemtreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_oem_settings_tbl[0], mt9p111_vfs_oem_settings_tbl_size, &mt9p111_use_vfs_oem_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}
static ssize_t mt9p111_write_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    long rc = 0;

    mt9p111_get_param(buf, count, &mt9p111_vfs_writereg_settings_tbl[0], mt9p111_vfs_writereg_settings_tbl_size, &mt9p111_use_vfs_writereg_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    if (mt9p111_use_vfs_writereg_setting)
    {
        rc = mt9p111_i2c_write_table(&mt9p111_vfs_writereg_settings_tbl[0],
            mt9p111_vfs_writereg_settings_tbl_size);
        mt9p111_use_vfs_writereg_setting =0;
    }
    return count;
}

static ssize_t mt9p111_fqc_set_flash(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_set_flash_gpio();
    return count;
}

static ssize_t mt9p111_setrange(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mt9p111_get_param(buf, count, &mt9p111_vfs_getreg_settings_tbl[0], mt9p111_vfs_getreg_settings_tbl_size, &mt9p111_use_vfs_getreg_setting, MT9P111_VFS_PARAM_NUM); //Div6-PT2-MM-CH-Camera_VFS-01*
    return count;
}

static ssize_t mt9p111_getrange(struct device *dev, struct device_attribute *attr, char *buf)
{
    int i,rc;
    char *str = buf;

    if (mt9p111_use_vfs_getreg_setting)
    {
        for (i=0;i<=mt9p111_vfs_getreg_settings_tbl_size;i++)
        {
            if (mt9p111_vfs_getreg_settings_tbl[i].waddr==0xFFFF)
                break;

            rc = mt9p111_i2c_read(mt9p111_client->addr,
            mt9p111_vfs_getreg_settings_tbl[i].waddr, &(mt9p111_vfs_getreg_settings_tbl[i].wdata), mt9p111_vfs_getreg_settings_tbl[i].width);
            if (rc <0)
            {
                mt9p111_set_flash_gpio();
                break;
            }

            CDBG("mt9p111 reg 0x%4X = 0x%2X\n", mt9p111_vfs_getreg_settings_tbl[i].waddr, mt9p111_vfs_getreg_settings_tbl[i].wdata);

            str += sprintf(str, "%04X,%2X \n", mt9p111_vfs_getreg_settings_tbl[i].waddr, 
            mt9p111_vfs_getreg_settings_tbl[i].wdata);

        }
    }
    return (str - buf);
}
DEVICE_ATTR(prev_snap_reg_mt9p111, 0664, NULL, mt9p111_write_prev_snap_reg);
DEVICE_ATTR(init_reg_mt9p111, 0664, NULL, mt9p111_write_init_reg);
DEVICE_ATTR(lens_reg_mt9p111, 0664, NULL, mt9p111_write_lens_reg);
DEVICE_ATTR(iq1_reg_mt9p111, 0664, NULL, mt9p111_write_iq1_reg);
DEVICE_ATTR(iq2_reg_mt9p111, 0664, NULL, mt9p111_write_iq2_reg);
DEVICE_ATTR(char_reg_mt9p111, 0664, NULL, mt9p111_write_char_reg);
DEVICE_ATTR(aftrigger_reg_mt9p111, 0664, NULL, mt9p111_write_aftrigger_reg);
DEVICE_ATTR(patch_reg_mt9p111, 0664, NULL, mt9p111_write_patch_reg);
DEVICE_ATTR(pll_reg_mt9p111, 0664, NULL, mt9p111_write_pll_reg);
DEVICE_ATTR(af_reg_mt9p111, 0664, NULL, mt9p111_write_af_reg);
DEVICE_ATTR(oemreg_5642, 0664, NULL, mt9p111_write_oemtreg);
DEVICE_ATTR(writereg_5642, 0664, NULL, mt9p111_write_reg);
DEVICE_ATTR(fqc_set_flash_mt9p111, 0664, NULL, mt9p111_fqc_set_flash);
DEVICE_ATTR(getreg_5642, 0664, mt9p111_getrange, mt9p111_setrange);

static int create_attributes(struct i2c_client *client)
{
    int rc;

    dev_attr_prev_snap_reg_mt9p111.attr.name = MT9P111_PREV_SNAP_REG;
    dev_attr_init_reg_mt9p111.attr.name = MT9P111_INIT_REG;
    dev_attr_lens_reg_mt9p111.attr.name = MT9P111_LENS_REG;
    dev_attr_iq1_reg_mt9p111.attr.name = MT9P111_IQ1_REG;
    dev_attr_iq2_reg_mt9p111.attr.name = MT9P111_IQ2_REG;
    dev_attr_char_reg_mt9p111.attr.name = MT9P111_CHAR_REG;	
    dev_attr_aftrigger_reg_mt9p111.attr.name = MT9P111_AFTRIGGER_REG;
    dev_attr_patch_reg_mt9p111.attr.name = MT9P111_PATCH_REG;
    dev_attr_pll_reg_mt9p111.attr.name = MT9P111_PLL_REG;
    dev_attr_af_reg_mt9p111.attr.name = MT9P111_AF_REG;
    dev_attr_oemreg_5642.attr.name = MT9P111_OEMREG;
    dev_attr_writereg_5642.attr.name = MT9P111_WRITEREG;
    dev_attr_fqc_set_flash_mt9p111.attr.name = MT9P111_FQC_SET_FLASH;
    dev_attr_getreg_5642.attr.name = MT9P111_GETREG;

    rc = device_create_file(&client->dev, &dev_attr_prev_snap_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"prev_snap_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_init_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"init_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_lens_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"lens_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_iq1_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"iq1_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_iq2_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"iq2_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }		 

    rc = device_create_file(&client->dev, &dev_attr_char_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"char_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_aftrigger_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"aftrigger_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_patch_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"patch_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }	

    rc = device_create_file(&client->dev, &dev_attr_pll_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"pll_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_af_reg_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"af_reg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_oemreg_5642);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"oemreg\" failed!! <%d>", __func__, rc);
        return rc; 
    }
    
    rc = device_create_file(&client->dev, &dev_attr_writereg_5642);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"writereg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_fqc_set_flash_mt9p111);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"fqc_set_flash\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&client->dev, &dev_attr_getreg_5642);
    if (rc < 0) {
        dev_err(&client->dev, "%s: Create mt9p111 attribute \"getreg\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    return rc;
}

#endif /* MT9P111_USE_VFS */

#ifdef CONFIG_FIH_FTM
static ssize_t mt9p111_ftm_set_flash_sync(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    int val = 0;

    sscanf(buf, "%d\n", &val);
    printk("%s: Input = %d.\n", __func__, val);

    ret = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, FLASHLED_ID, (int)val);
    if (ret)
        return -EPERM;

    return ret;
}
static ssize_t mt9p111_ftm_set_flash_drv_en(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    int val = 0;

    sscanf(buf, "%d\n", &val);
    printk("%s: Input = %d.\n", __func__, val);

    ret = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, FLASHLED_ID, (int)val);
    if (ret)
        return -EPERM;

    return ret;	
}

static ssize_t mt9p111_ftm_set_af(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    int val = 0;

    sscanf(buf, "%d\n", &val);
    printk("%s: Input = %d.\n", __func__, val);
//Div2-SW6-MM-MC-ReduceTestTime-01+{
#ifdef CONFIG_FIH_FTM
    mt9p111_ftm_af_Enable = val;
#endif
//Div2-SW6-MM-MC-ReduceTestTime-01+}

    mt9p111_set_autofocus(1, val);
    return ret;	
}

DEVICE_ATTR(ftm_set_flash_sync_mt9p111, 0666, NULL, mt9p111_ftm_set_flash_sync);
DEVICE_ATTR(ftm_set_flash_drv_en_mt9p111, 0666, NULL, mt9p111_ftm_set_flash_drv_en);
DEVICE_ATTR(ftm_set_af_mt9p111, 0666, NULL, mt9p111_ftm_set_af);
#endif

static int mt9p111_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
    int rc = 0;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    mt9p111_sensorw =
        kzalloc(sizeof(struct mt9p111_work), GFP_KERNEL);

    if (!mt9p111_sensorw) {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, mt9p111_sensorw);
    mt9p111_init_client(client);
    mt9p111_client = client;

    CDBG("mt9p111_probe succeeded!\n");

    return 0;

probe_failure:
    kfree(mt9p111_sensorw);
    mt9p111_sensorw = NULL;
    CDBG("mt9p111_probe failed!\n");
    return rc;
}

#ifdef CONFIG_FIH_AAT1272
static int aat1272_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
    int rc = 0;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    aat1272_client = client;

    printk("aat1272_probe succeeded!\n");

    return 0;

probe_failure:
    CDBG("aat1272_probe failed!\n");
    return rc;
}
#endif

static const struct i2c_device_id mt9p111_i2c_id[] = {
    { "mt9p111", 0},
    { },
};

static struct i2c_driver mt9p111_i2c_driver = {
    .id_table = mt9p111_i2c_id,
    .probe  = mt9p111_i2c_probe,
    .remove = __exit_p(mt9p111_i2c_remove),
    .driver = {
        .name = "mt9p111",
    },
};

#ifdef CONFIG_FIH_AAT1272
static const struct i2c_device_id aat1272_i2c_id[] = {
    { "aat1272", 0},
    { },
};

static struct i2c_driver aat1272_i2c_driver = {
    .id_table = aat1272_i2c_id,
    .probe  = aat1272_i2c_probe,
    .remove = __exit_p(aat1272_i2c_remove),
    .driver = {
        .name = "aat1272",
    },
};
#endif

#ifdef CONFIG_MT9P111_SUSPEND
#ifdef CONFIG_MT9P111_RESUME
static void mt9p111_resume(void)
{
    int rc = 0;

    printk("MT9P111_resume entry.\n");
    wake_lock_init(&mt9p111_wake_lock_suspend, WAKE_LOCK_SUSPEND, "mt9p111");
    printk("MT9P111_resume wake_lock.\n");
    wake_lock(&mt9p111_wake_lock_suspend);

    msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);

    /* Pull Low CAMIF_SW Pin */
    if(mt9p111info->mclk_sw_pin!=0xffff)
    {
        rc = fih_cam_output_gpio_control(mt9p111info->mclk_sw_pin, "mt9p111", 0);
        if (rc)
            goto init_probe_fail;
        msleep(2);
    }
    
    /* 5M Pwdn Pin Pull Low*/
    rc = fih_cam_output_gpio_control(mt9p111info->pwdn_pin, "mt9p111", 0);
    if (rc)
        goto init_probe_fail;
    msleep(10);
    
    /* 5M Reset Pin Pull Low*/
    rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 0);
    if (rc)
        goto init_probe_fail;
    
    msleep(10);

    /* Input MCLK = 24MHz */
    msm_camio_clk_rate_set(24000000);

    /* Enable camera power*/
    if(mt9p111_vreg_vddio==0)
    {
        rc = fih_cam_vreg_control(mt9p111info->cam_vreg_vddio_id, 1800, 1);
        if (rc)
            goto init_probe_fail;
    }
    mt9p111_vreg_vddio=1;

    mdelay(1);//t1 = 0~500 ms

    if(mt9p111_vreg_acore==0)
    {
        if (mt9p111info->cam_v2p8_en_pin==0xffff)
            rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 1);
        else
            rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 1);

        if (rc)
            goto init_probe_fail;
    }
    mt9p111_vreg_acore=1;

    mdelay(2);//t2 = 1~500ms

    /* Enable MCLK = 24MHz */
    gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    printk(KERN_INFO "%s: Enable MCLK = 24MHz, GPIO_2MA. \n", __func__);

    mdelay(2);//t3 = 70~500 MCLK

    /* 5M Reset Pin Pull High*/
    rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 1);
    if (rc)
        goto init_probe_fail;

    mdelay(100);//tA = 100ms

    rc = mt9p111_i2c_write(mt9p111_client->addr,0x0020, 0x0000, WORD_LEN);
    if (rc < 0)
        goto init_probe_fail;

    mdelay(1);//tB = 1ms

    /* 5M Reset Pin Pull Low*/
    rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 0);
    if (rc)
        goto init_probe_fail;

    udelay(3);//tC = 3us

    /* 5M Reset Pin Pull High*/
    rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 1);
    if (rc)
        goto init_probe_fail;

    udelay(40);//tD = 40us

    rc = mt9p111_i2c_write(mt9p111_client->addr,0x0020, 0x0000, WORD_LEN);
    if (rc < 0)
        goto init_probe_fail;

    mdelay(10);//tE = 10ms


    rc = mt9p111_reg_init();
    if (rc < 0)
        goto init_probe_fail;

    rc=mt9p111_reg_init2();
    if (rc < 0)
        goto init_probe_fail;
//init finished, entering standby
    rc = mtp9111_sensor_standby(0);
    if (rc < 0)
        goto init_probe_fail;

    if(mt9p111_vreg_acore==1)
    {
         if (mt9p111info->cam_v2p8_en_pin==0xffff)
            rc = fih_cam_vreg_control(mt9p111info->cam_vreg_acore_id, 2800, 0);
        else
            rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 0);
        
        if (rc)
            printk(KERN_ERR "%s: vreg disable failed (%d)\n", __func__, rc);
    }
    mt9p111_vreg_acore=0;
    
    printk("mt9p111 disable mclk\n");
    msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
    //Pull Low CAMIF_SW Pin 50
    rc = fih_cam_output_gpio_control(mt9p111info->mclk_sw_pin, "mt9p111", 1);

    msleep(2);

    mt9p111_exposure_mode=CAMERA_AEC_FRAME_AVERAGE;
    mt9p111_antibanding_mode=CAMERA_ANTIBANDING_AUTO;
    mt9p111_brightness_mode=CAMERA_BRIGHTNESS_3;
    mt9p111_contrast_mode=CAMERA_CONTRAST_ZERO;
    mt9p111_effect_mode=CAMERA_EFFECT_OFF;
    mt9p111_saturation_mode=CAMERA_SATURATION_ZERO;
    mt9p111_sharpness_mode=CAMERA_SHARPNESS_ZERO;
    mt9p111_whitebalance_mode=CAMERA_WB_AUTO;

    mt9p111_rectangle_x=270;
    mt9p111_rectangle_y=190;
    mt9p111_rectangle_w=100;
    mt9p111_rectangle_h=100;
    mt9p111_first_init=0;

    printk("mt9p111_resume wake_unlock.\n");
    wake_unlock(&mt9p111_wake_lock_suspend);
    wake_lock_destroy(&mt9p111_wake_lock_suspend);

    return;

init_probe_fail:
    mt9p111_init_done=0;
    mt9p111_snapshoted=0;
    mt9p111_parameters_changed_count=0;
    mt9p111_first_init=1;
    gpio_tlmm_config(GPIO_CFG(mt9p111info->MCLK_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
    mt9p111_power_off();
    printk("mt9p111 disable mclk\n");
    msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
    printk("mt9p111_resume wake_unlock.\n");
    wake_unlock(&mt9p111_wake_lock_suspend);
    wake_lock_destroy(&mt9p111_wake_lock_suspend);
    printk("mt9p111_resume FAIL.\n");
    return;
}
#endif
#endif

#ifdef CONFIG_MT9P111_SUSPEND
static void mt9p111_early_suspend(struct early_suspend *h)
{

}

static void mt9p111_late_resume(struct early_suspend *h)
{
#ifdef CONFIG_MT9P111_RESUME
    hrtimer_cancel(&mt9p111_resume_timer);
    hrtimer_start(&mt9p111_resume_timer,ktime_set(3, 0),HRTIMER_MODE_REL);

    if (hrtimer_active(&mt9p111_resume_timer))
        printk(KERN_INFO "%s: TIMER running\n", __func__);
#endif  
}

#ifdef CONFIG_MT9P111_RESUME
static enum hrtimer_restart mt9p111_resume_timer_func(struct hrtimer *timer)
{
    printk(KERN_INFO "mt9p111_msg: mt9p111_resume_timer_func\n" );
    complete(&mt9p111_resume_comp);	

    return HRTIMER_NORESTART;
}

static int mt9p111_resume_thread(void *arg)
{
    printk(KERN_INFO "mt9p111_msg: mt9p111_resume_thread running\n");
    daemonize("mt9p111_resume_thread");
    while (1)
    {
        wait_for_completion(&mt9p111_resume_comp);
        printk(KERN_INFO "%s: Got complete signal\n", __func__);
        mutex_lock(&front_mut);
        msleep(1000);
        mutex_lock(&mt9p111_mut);
        if(mt9p111_first_init==1)
        {
            printk(KERN_INFO "%s: Need to init mt9p111\n", __func__);
            mt9p111_first_init=0;
            mt9p111_resume();
        }
        else
            printk(KERN_INFO "%s: Don't need to init mt9p111\n", __func__);
        mutex_unlock(&mt9p111_mut);
        mutex_unlock(&front_mut);
    }
    return 0;
}
#endif

static int mt9p111_suspend(struct platform_device *pdev, pm_message_t state)
{
    printk(KERN_INFO "mt9p111_suspend\n" );

    if(mt9p111_first_init==0)
    {
        mt9p111_power_off();

        mt9p111_init_done=0;
        mt9p111_snapshoted=0;
        mt9p111_parameters_changed_count=0;
        mt9p111_first_init=1;
    }
    return 0;
}

struct early_suspend pmt9p111_early_suspend = {
        .level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 2,
        .suspend = mt9p111_early_suspend,
        .resume = mt9p111_late_resume,
};
#endif

static int mt9p111_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
    int rc = i2c_add_driver(&mt9p111_i2c_driver);

    printk(KERN_INFO "mt9p111_msg: mt9p111_sensor_probe() is called.\n");

    if (rc < 0 || mt9p111_client == NULL) {
        rc = -ENOTSUPP;
        goto probe_done;
    }

#ifdef CONFIG_FIH_AAT1272
    rc = i2c_add_driver(&aat1272_i2c_driver);
    printk(KERN_INFO "%s: i2c_add_driver(aat1272_i2c_driver), rc = %d\n", __func__, rc);
    if (rc < 0 || aat1272_client == NULL) {
        rc = -ENOTSUPP;
        goto probe_done;
    }
#endif

#ifdef MT9P111_USE_VFS
    rc = create_attributes(mt9p111_client);
    if (rc < 0) {
        dev_err(&mt9p111_client->dev, "%s: create attributes failed!! <%d>", __func__, rc);
        goto probe_done;
    }
#endif 
    hrtimer_init(&mt9p111_flashled_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);	
    mt9p111_flashled_timer.function = mt9p111_flashled_timer_func;	
    mt9p111_thread_id = kernel_thread(mt9p111_flashled_off_thread, NULL, CLONE_FS | CLONE_FILES);

#ifdef CONFIG_FIH_FTM
    dev_attr_ftm_set_flash_sync_mt9p111.attr.name = MT9P111_FTM_SET_FLASH_SYNC;
    dev_attr_ftm_set_flash_drv_en_mt9p111.attr.name = MT9P111_FTM_SET_FLASH_DRV_EN;
    dev_attr_ftm_set_af_mt9p111.attr.name = MT9P111_FTM_SET_AF;

    rc = device_create_file(&mt9p111_client->dev, &dev_attr_ftm_set_flash_sync_mt9p111);
    if (rc < 0) {
        dev_err(&mt9p111_client->dev, "%s: Create mt9p111 attribute \"ftm_set_flash_sync\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&mt9p111_client->dev, &dev_attr_ftm_set_flash_drv_en_mt9p111);
    if (rc < 0) {
        dev_err(&mt9p111_client->dev, "%s: Create mt9p111 attribute \"ftm_set_flash_drv_en\" failed!! <%d>", __func__, rc);
        return rc; 
    }

    rc = device_create_file(&mt9p111_client->dev, &dev_attr_ftm_set_af_mt9p111);
    if (rc < 0) {
        dev_err(&mt9p111_client->dev, "%s: Create mt9p111 attribute \"ftm_set_af\" failed!! <%d>", __func__, rc);
        return rc; 
    }
#endif
    mt9p111info = info;

    /* Init 5M pins state */
    if(mt9p111info->standby_pin!=0xffff)
        gpio_tlmm_config(GPIO_CFG(mt9p111info->standby_pin, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_DISABLE);

    rc = fih_cam_output_gpio_control(mt9p111info->rst_pin, "mt9p111", 0);
    if (rc)
        return rc;

    rc = fih_cam_output_gpio_control(mt9p111info->pwdn_pin, "mt9p111", 0);
    if (rc)
        return rc;

    rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED, "mt9p111", 0);
    if (rc)
        return rc;

    rc = fih_cam_output_gpio_control(mt9p111info->GPIO_FLASHLED_DRV_EN, "mt9p111", 0);
    if (rc)
        return rc;

    /* About power setting */

    if (mt9p111info->cam_v2p8_en_pin!=0xffff)
    {
        /* Disable camera 2.8V */
        rc = fih_cam_output_gpio_control(mt9p111info->cam_v2p8_en_pin, "mt9p111", 0);
        if (rc)
            return rc;
    }
    else
    {
        /* Enable camera 2.8V pull_down_switch */
        rc = fih_cam_vreg_pull_down_switch(mt9p111info->cam_vreg_acore_id, 1);
        if (rc)
            return rc;
    }

    rc = fih_cam_vreg_pull_down_switch(mt9p111info->cam_vreg_vddio_id, 1);
    if (rc)
        return rc;

    s->s_init = mt9p111_sensor_init;
    s->s_release = mt9p111_sensor_release;
    s->s_config  = mt9p111_sensor_config;

#ifdef CONFIG_MT9P111_STANDBY
    //mt9p111_resume();
#endif
#ifdef CONFIG_MT9P111_SUSPEND
    register_early_suspend(&pmt9p111_early_suspend);
#ifdef CONFIG_MT9P111_RESUME
    hrtimer_init(&mt9p111_resume_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);	
    mt9p111_resume_timer.function = mt9p111_resume_timer_func;	
    mt9p111_resumethread_id = kernel_thread(mt9p111_resume_thread, NULL, CLONE_FS | CLONE_FILES);
#endif
#endif

probe_done:
    CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
    return rc;
}

static int __mt9p111_probe(struct platform_device *pdev)
{
    printk(KERN_WARNING "mt9p111_msg: in __mt9p111_probe() because name of msm_camera_mt9p111 match.\n");
    return msm_camera_drv_start(pdev, mt9p111_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
    .probe = __mt9p111_probe,
    .driver = {
        .name = "msm_camera_mt9p111",
        .owner = THIS_MODULE,
    },
#ifdef CONFIG_MT9P111_SUSPEND
    .suspend = mt9p111_suspend,
#endif
};

static int __init mt9p111_init(void)
{
    return platform_driver_register(&msm_camera_driver);
}

module_init(mt9p111_init);

