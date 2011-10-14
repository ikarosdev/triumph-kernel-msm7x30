/*
 *     hm0356.c - Camera Sensor Config
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
#include "hm0356.h"

#include <mach/pmic.h>
#include <linux/completion.h>
#include <linux/hrtimer.h>
#include "fih_camera_parameter.h"
#include "fih_camera_power.h"

#include "../../../arch/arm/mach-msm/smd_private.h"

//Div2-SW6-MM-MC-ImplementCameraFTMforSF8Serials-00*{
#define  HM0356_MODEL_ID_1     0x03
#define  HM0356_MODEL_ID_2     0x56
//Div2-SW6-MM-MC-ImplementCameraFTMforSF8Serials-00*}

/*  SOC Registers Page 1  */
#define  REG_hm0356_SENSOR_RESET     0x001A
#define  REG_hm0356_STANDBY_CONTROL  0x0018
#define  REG_hm0356_MCU_BOOT         0x001C

struct hm0356_work {
    struct work_struct work;
};

static struct  hm0356_work *hm0356_sensorw;
static struct  i2c_client *hm0356_client;
static const struct msm_camera_sensor_info *hm0356info;

struct hm0356_ctrl {
    const struct msm_camera_sensor_info *sensordata;
};

static struct hm0356_ctrl *hm0356_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(hm0356_wait_queue);
DECLARE_MUTEX(hm0356_sem);
DEFINE_MUTEX(hm0356_mut);
#ifdef CONFIG_MT9P111_RESUME
extern struct mutex front_mut;
#endif

static unsigned int hm0356_supported_effects[] = {
    CAMERA_EFFECT_OFF
};

static unsigned int hm0356_supported_autoexposure[] = {
    CAMERA_AEC_FRAME_AVERAGE
};

static unsigned int hm0356_supported_antibanding[] = {
    CAMERA_ANTIBANDING_OFF
};

static unsigned int hm0356_supported_wb[] = {
    CAMERA_WB_AUTO 
};

static unsigned int hm0356_supported_led[] = {
    LED_MODE_OFF,
};
static unsigned int hm0356_supported_ISO[] = {
    CAMERA_ISO_ISO_AUTO,
};

static unsigned int hm0356_supported_focus[] = {
    DONT_CARE
};
static unsigned int hm0356_supported_lensshade[] = {
    FALSE
};

static unsigned int hm0356_supported_scenemode[] = {
    CAMERA_BESTSHOT_OFF
};

static unsigned int hm0356_supported_continuous_af[] = {
    FALSE
};

static unsigned int hm0356_supported_touchafaec[] = {
    FALSE
};

static unsigned int hm0356_supported_frame_rate_modes[] = {
    FPS_MODE_AUTO
};
struct sensor_parameters hm0356_parameters = {
    .autoexposuretbl = hm0356_supported_autoexposure,
    .autoexposuretbl_size = sizeof(hm0356_supported_autoexposure)/sizeof(unsigned int),
    .effectstbl = hm0356_supported_effects,
    .effectstbl_size = sizeof(hm0356_supported_effects)/sizeof(unsigned int),
    .wbtbl = hm0356_supported_wb,
    .wbtbl_size = sizeof(hm0356_supported_wb)/sizeof(unsigned int),
    .antibandingtbl = hm0356_supported_antibanding,
    .antibandingtbl_size = sizeof(hm0356_supported_antibanding)/sizeof(unsigned int),
    .flashtbl = hm0356_supported_led,
    .flashtbl_size = sizeof(hm0356_supported_led)/sizeof(unsigned int),
    .focustbl = hm0356_supported_focus,
    .focustbl_size = sizeof(hm0356_supported_focus)/sizeof(unsigned int),
    .ISOtbl = hm0356_supported_ISO,
    .ISOtbl_size = sizeof( hm0356_supported_ISO)/sizeof(unsigned int),
    .lensshadetbl = hm0356_supported_lensshade,
    .lensshadetbl_size = sizeof(hm0356_supported_lensshade)/sizeof(unsigned int),
    .scenemodetbl = hm0356_supported_scenemode,
    .scenemodetbl_size = sizeof( hm0356_supported_scenemode)/sizeof(unsigned int),
    .continuous_aftbl = hm0356_supported_continuous_af,
    .continuous_aftbl_size = sizeof(hm0356_supported_continuous_af)/sizeof(unsigned int),
    .touchafaectbl = hm0356_supported_touchafaec,
    .touchafaectbl_size = sizeof(hm0356_supported_touchafaec)/sizeof(unsigned int),
    .frame_rate_modestbl = hm0356_supported_frame_rate_modes,
    .frame_rate_modestbl_size = sizeof( hm0356_supported_frame_rate_modes)/sizeof(unsigned int),
    .max_brightness = 3,
    .max_contrast = 2,
    .max_saturation = 2,
    .max_sharpness = 2,
    .min_brightness = 2,
    .min_contrast = 2,
    .min_saturation = 2,
    .min_sharpness = 2,
};

/*=============================================================
	EXTERNAL DECLARATIONS
==============================================================*/
extern struct hm0356_reg hm0356_regs;

/*=============================================================*/

static int hm0356_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
    struct i2c_msg msgs[] = {
        {
            .addr   = saddr,
            .flags = 0,
            .len   = 1,
            .buf   = rxdata,
        },
        {
            .addr   = saddr,
            .flags = I2C_M_RD,
            .len   = length,
            .buf   = rxdata,
        },
    };

    if (i2c_transfer(hm0356_client->adapter, msgs, 2) < 0) {
        printk(KERN_ERR "hm0356_msg: hm0356_i2c_rxdata failed\n");
        return -EIO;
    }

    return 0;
}

static int32_t hm0356_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum hm0356_width width)
{
    int32_t rc = 0;
    unsigned char buf[4];

    if (!rdata)
        return -EIO;

    memset(buf, 0, sizeof(buf));

    switch (width) {
        case FC_WORD_LEN: {
            buf[0] = (raddr & 0xFF00)>>8;
            buf[1] = (raddr & 0x00FF);

            rc = hm0356_i2c_rxdata(saddr, buf, 2);
            if (rc < 0)
                return rc;

            *rdata = buf[0] << 8 | buf[1];
        }
        break;

        case FC_BYTE_LEN: {
            buf[0] = (uint8_t) raddr;

            rc = hm0356_i2c_rxdata(saddr, buf, 1);

            if (rc < 0)
                return rc;

            *rdata = buf[0];
        }
        break;

        default:
            break;
    }

    if (rc < 0)
    CDBG("hm0356_i2c_read failed!\n");

    return rc;
}

static int32_t hm0356_i2c_txdata(unsigned short saddr,
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

    if (i2c_transfer(hm0356_client->adapter, msg, 1) < 0){
        printk(KERN_ERR "hm0356_msg: hm0356_i2c_txdata failed\n");
        return -EIO;
    }

    return 0;
}

static int32_t hm0356_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum hm0356_width width)
{
    int32_t rc = -EIO;
    unsigned char buf[4];

    memset(buf, 0, sizeof(buf));
    switch (width) {
        case FC_WORD_LEN: {
            buf[0] = (waddr & 0xFF00)>>8;
            buf[1] = (waddr & 0x00FF);
            buf[2] = (wdata & 0xFF00)>>8;
            buf[3] = (wdata & 0x00FF);

            rc = hm0356_i2c_txdata(saddr, buf, 4);
        }
            break;
        case FC_BYTE_LEN: {
            buf[0] = (uint8_t) waddr;
            buf[1] = (uint8_t) wdata;

            rc = hm0356_i2c_txdata(saddr, buf, 2);
        }
            break;

        default:
            break;
    }

    if (rc < 0)
        printk("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);

    return rc;
}

static int32_t hm0356_i2c_write_table(
	struct hm0356_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
    int i;
    int32_t rc = -EIO;

    for (i = 0; i < num_of_items_in_table; i++) {
        rc = hm0356_i2c_write(hm0356_client->addr,
        reg_conf_tbl->waddr, reg_conf_tbl->wdata,
        reg_conf_tbl->width);
        if (rc < 0)
            break;
        if (reg_conf_tbl->mdelay_time != 0)
            mdelay(reg_conf_tbl->mdelay_time);

        reg_conf_tbl++;
    }

    return rc;
}

static long hm0356_reg_init(void)
{
    long rc = 0;

    rc = hm0356_i2c_write_table(&hm0356_regs.inittbl[0], hm0356_regs.inittbl_size);
    if (rc < 0)
        return rc;
    else
    {
        if (hm0356info->sensor_Orientation == MSM_CAMERA_SENSOR_ORIENTATION_180) 
        {
            //Here to setting sensor orientation for HW design.
            //Preview and Snapshot orientation.
            hm0356_i2c_write(hm0356_client->addr, 0x0006, 0x80, FC_BYTE_LEN);
            printk("Finish Orientation Setting.\n");	
        }
        printk("Finish Initial Setting for HM0356.\n");
    }
    return rc;
}

static long hm0356_set_effect(int mode, int effect)
{
    long rc = 0;
    printk("hm0356_set_effect, mode = %d, effect = %d\n",mode, effect);

    switch (effect) 
    {
        case CAMERA_EFFECT_COLORBAR: 
        {
            printk("%s: case = CAMERA_EFFECT_COLORBAR ~~ \n", __func__);

            //Disable Flip and Mirror function effect test patterns.  
            rc = hm0356_i2c_write(hm0356_client->addr, 0x0006, 0x00, FC_BYTE_LEN);
            if (rc < 0)
                return rc;
            else
                printk("%s: Disable Flip and Mirror function end ~\n", __func__);
            msleep(500);

            //Enable test mode for color bar pattern.
            rc = hm0356_i2c_write(hm0356_client->addr, 0x0025, 0x83, FC_BYTE_LEN);
            if (rc < 0)
                return rc;
            else
                printk("%s: Enable test mode for color bar pattern end ~\n", __func__);
            msleep(500);
        }
            break;

        default: 
            printk("%s: Not support this effect ~\n", __func__);
            return rc;
    }

    return rc;
}

static long hm0356_set_sensor_mode(int mode)
{
    switch (mode) 
    {
        case SENSOR_PREVIEW_MODE:
        {
            printk(KERN_ERR "hm0356_msg: case SENSOR_PREVIEW_MODE.\n");
        }
            break;

        case SENSOR_SNAPSHOT_MODE:
        {
            printk(KERN_ERR "hm0356_msg: case SENSOR_SNAPSHOT_MODE.\n");
        }
            break;

        case SENSOR_RAW_SNAPSHOT_MODE:
        {
            printk(KERN_ERR "hm0356_msg: case SENSOR_RAW_SNAPSHOT_MODE.\n");
        }
            break;

        default:
        return -EINVAL;
    }

    return 0;
}

//SW5-Multimedia-TH-SWStandby-00+{
#ifdef CONFIG_HM0356_STANDBY
int hm0356_sensor_standby(int on)
{
    int rc = 0;

        /* VGA Pwdn Pin Pull High*/
        rc = fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 1);
        if (rc)
            return rc;
        printk(KERN_INFO "hm0356_sensor_standby:  VGA Pwdn Pin Pull High\n");

        /* 5M Pwdn Pin Pull Low*/
        rc = fih_cam_output_gpio_control(hm0356info->pwdn_pin, "mt9p111", 0);
        if (rc)
            return rc;
        mdelay(1);
        printk(KERN_INFO "hm0356_sensor_standby: 5M Pwdn Pin Pull Low\n");

        /* Disable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(hm0356info->MCLK_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        rc = fih_cam_output_gpio_control(hm0356info->MCLK_PIN, "hm3056", 0);
        if (rc)
            return rc;
        printk("%s: Disable mclk\n", __func__);
        printk(KERN_INFO "hm0356_sensor_standby: Disable MCLK\n");

    return rc;
}
#endif
//SW5-Multimedia-TH-SWStandby-00+}

int hm0356_power_on(void)
{
    int rc;

    //Div2-SW6-MM-MC-PortingCameraDriverForSF8-00*{
    if (hm0356info->mclk_sw_pin == 0xffff)
    {
        //SW5-Multimedia-TH-hm0356PowerOn-00+{

        //SW5-Multimedia-TH-SWStandby-00+{
        #ifndef CONFIG_HM0356_STANDBY
        /* 5M Pwdn Pin Pull High*/
        rc = fih_cam_output_gpio_control(hm0356info->pwdn_pin, "mt9p111", 1);
        if (rc)
            return rc;
        mdelay(1);
        printk(KERN_INFO "hm0356_power_on: 5M Pwdn Pin Pull High\n");
        #endif
        //SW5-Multimedia-TH-SWStandby-00+}

        /* Enable camera power*/
        rc = fih_cam_vreg_control(hm0356info->cam_vreg_vddio_id, 1800, 1);
        if (rc)
            return rc;

        if (hm0356info->cam_v2p8_en_pin == 0xffff )
            rc = fih_cam_vreg_control(hm0356info->cam_vreg_acore_id, 2800, 1);
        else
            rc = fih_cam_output_gpio_control(hm0356info->cam_v2p8_en_pin, "hm0356", 1);
        
        if (rc)
            return rc;
        mdelay(1);
        printk(KERN_INFO "hm0356_power_on: Enable camera power\n");

        /* Enable MCLK = 24MHz */
        msm_camio_clk_rate_set(24000000);
        msm_camio_camif_pad_reg_reset();
        gpio_tlmm_config(GPIO_CFG(hm0356info->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        printk(KERN_INFO "%s: Enable MCLK = 24MHz, GPIO_2MA. \n", __func__);
        printk(KERN_INFO "hm0356_power_on: Enable MCLK\n");

        /* 5M Reset Pin Pull High*/
        rc = fih_cam_output_gpio_control(hm0356info->rst_pin, "mt9p111", 1);
        if (rc)
            return rc;
        mdelay(1);
        printk(KERN_INFO "hm0356_power_on: 5M Reset Pin Pull High\n");

        /* VGA Pwdn Pin Pull Low*/
        rc = fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 0);
        if (rc)
            return rc;
        printk(KERN_INFO "hm0356_power_on:  VGA Pwdn Pin Pull Low\n");
        //SW5-Multimedia-TH-hm0356PowerOn-00+}
    }
    else
    {	
        rc = fih_cam_vreg_control(hm0356info->cam_vreg_vddio_id, 1800, 1);
        if (rc)
            return rc;

        mdelay(1); // t2	//5

        rc = fih_cam_vreg_control(hm0356info->cam_vreg_acore_id, 2800, 1);
        if (rc)
            return rc;
        mdelay(1);

        /* Power Down Pin */
        rc = fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 0);
        if (rc)
            return rc;

        mdelay(2);	//t1

        /* Input MCLK = 24MHz */
        msm_camio_clk_rate_set(24000000);
        gpio_tlmm_config(GPIO_CFG(hm0356info->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        msm_camio_camif_pad_reg_reset();

        mdelay(2);	//t2	
    }
     //Div2-SW6-MM-MC-PortingCameraDriverForSF8-00*}

	return rc;
}

int hm0356_power_off(void)
{
	int rc;

    //Div2-SW6-MM-MC-PortingCameraDriverForSF8-00*{
    if (hm0356info->mclk_sw_pin == 0xffff)
    {
        //SW5-Multimedia-TH-hm0356PowerOff-00+{
        /* 5M Pwdn Pin Pull Low*/
        rc = fih_cam_output_gpio_control(hm0356info->pwdn_pin, "mt9p111", 0);
        if (rc)
            return rc;
        mdelay(1);
        printk(KERN_INFO "hm0356_power_off: 5M Pwdn Pin Pull Low\n");

        /* 5M Reset Pin Pull Low*/
        rc = fih_cam_output_gpio_control(hm0356info->rst_pin, "mt9p111", 0);
        if (rc)
            return rc;
        mdelay(1);
        printk(KERN_INFO "hm0356_power_off: 5M Reset Pin Pull Low\n");

        /* Disable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(hm0356info->MCLK_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        rc = fih_cam_output_gpio_control(hm0356info->MCLK_PIN, "hm3056", 0);
        if (rc)
            return rc;
        printk("%s: Disable mclk\n", __func__);
        printk(KERN_INFO "hm0356_power_off: Disable MCLK\n");

        /* Disable camera 2.8V power*/
        if (hm0356info->cam_v2p8_en_pin == 0xffff )
            rc = fih_cam_vreg_control(hm0356info->cam_vreg_acore_id, 2800, 0);
        else
            rc = fih_cam_output_gpio_control(hm0356info->cam_v2p8_en_pin, "hm0356", 0);
        if (rc)
            return rc;

        /* Disable camera 1.8V power */
        rc = fih_cam_vreg_control(hm0356info->cam_vreg_vddio_id, 1800, 1);
        if (rc)
            return rc;
        printk(KERN_INFO "hm0356_power_off: Disable camera power\n");

        //SW5-Multimedia-TH-hm0356PowerOff-00+{
    }
    else
    {
        rc = fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 1);
        if (rc)
            return rc;
        mdelay(1);

        rc = fih_cam_vreg_control(hm0356info->cam_vreg_acore_id, 2800, 0);
        if (rc)
            return rc;

        mdelay(5); // t2


        rc = fih_cam_vreg_control(hm0356info->cam_vreg_vddio_id, 1800, 0);
        if (rc)
            return rc;	
    }
     //Div2-SW6-MM-MC-PortingCameraDriverForSF8-00*}

	return rc;
}

static int hm0356_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id = 0;
    int rc = 0;

    printk("hm0356_sensor_init_probe entry.\n");
    sensor_init_parameters(data,&hm0356_parameters);

    /* Pull High Switch Pin */
    rc = fih_cam_output_gpio_control(hm0356info->mclk_sw_pin, "hm0356", 1);
    if (rc < 0)
            goto init_probe_fail;
	
    hm0356_power_on();

    rc = hm0356_reg_init();
    if (rc < 0)
        goto init_probe_fail;

    rc = hm0356_i2c_read(hm0356_client->addr,0x0001, &model_id, FC_BYTE_LEN);
    if (rc < 0 || model_id != HM0356_MODEL_ID_1)//Div2-SW6-MM-MC-ImplementCameraFTMforSF8Serials-00*
        goto init_probe_fail;

    printk("HM0356 Chip ID high byte = 0x%x .\n", model_id);

    rc = hm0356_i2c_read(hm0356_client->addr,0x0002, &model_id, FC_BYTE_LEN);
    if (rc < 0 || model_id != HM0356_MODEL_ID_2)//Div2-SW6-MM-MC-ImplementCameraFTMforSF8Serials-00*
        goto init_probe_fail;

    printk("HM0356 Chip ID low byte = 0x%x .\n", model_id);

    return rc;

init_probe_fail:
    printk("hm0356_sensor_init_probe FAIL.\n");
    return rc;
}

int hm0356_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc = 0;
    mutex_lock(&hm0356_mut);
#ifdef CONFIG_MT9P111_RESUME
    mutex_lock(&front_mut);
#endif
    hm0356_ctrl = kzalloc(sizeof(struct hm0356_ctrl), GFP_KERNEL);
    if (!hm0356_ctrl) {
        CDBG("hm0356_init failed!\n");
        rc = -ENOMEM;
        goto init_done;
    }

    if (data)
        hm0356_ctrl->sensordata = data;

    rc = hm0356_sensor_init_probe(data);
    if (rc < 0) {
        CDBG("hm0356_sensor_init failed!\n");
        goto init_fail;
    }

init_done:
    return rc;

init_fail:
    kfree(hm0356_ctrl);
    return rc;
}

static int hm0356_init_client(struct i2c_client *client)
{
    /* Initialize the MSM_CAMI2C Chip */
    init_waitqueue_head(&hm0356_wait_queue);
    return 0;
}

int hm0356_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long   rc = 0;

    if (copy_from_user(&cfg_data,(void *)argp,sizeof(struct sensor_cfg_data)))
        return -EFAULT;

    CDBG("hm0356_ioctl, cfgtype = %d, mode = %d\n",cfg_data.cfgtype, cfg_data.mode);

    switch (cfg_data.cfgtype) 
    {
        case CFG_SET_MODE:
        {
            rc = hm0356_set_sensor_mode(cfg_data.mode);
        }
            break;

        case CFG_SET_EFFECT:
        {
            rc = hm0356_set_effect(cfg_data.mode,cfg_data.cfg.effect);
        }
            break;

        default:

        break;
    }

    return rc;
}

int hm0356_sensor_release(void)
{
    int rc = 0;

    printk("hm0356_sensor_release()+++\n");

    //SW5-Multimedia-TH-SWStandby-00+{
    #ifdef CONFIG_HM0356_STANDBY
    fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 1);
    rc = hm0356_sensor_standby(1);
    /* Disable MCLK */
    gpio_tlmm_config(GPIO_CFG(hm0356info->MCLK_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
    #else
    /* Pull High CAM_SHUTDOWN Pin 0 */
    fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 1);

    hm0356_power_off();

    /* Disable MCLK */
    gpio_tlmm_config(GPIO_CFG(hm0356info->MCLK_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
    #endif
    //SW5-Multimedia-TH-SWStandby-00+}

    kfree(hm0356_ctrl);
    hm0356_ctrl = NULL;

    mutex_unlock(&hm0356_mut);
#ifdef CONFIG_MT9P111_RESUME
      mutex_unlock(&front_mut);
#endif
    printk("hm0356_sensor_release()---\n");

    return rc;
}

static int hm0356_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
    int rc = 0;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    hm0356_sensorw =kzalloc(sizeof(struct hm0356_work), GFP_KERNEL);

    if (!hm0356_sensorw) {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, hm0356_sensorw);
    hm0356_init_client(client);
    hm0356_client = client;

    CDBG("hm0356_probe succeeded!\n");

    return 0;

probe_failure:
    kfree(hm0356_sensorw);
    hm0356_sensorw = NULL;
    CDBG("hm0356_probe failed!\n");
    return rc;
}

static const struct i2c_device_id hm0356_i2c_id[] = {
    { "hm0356", 0},
    { },
};

static struct i2c_driver hm0356_i2c_driver = {
    .id_table = hm0356_i2c_id,
    .probe  = hm0356_i2c_probe,
    .remove = __exit_p(hm0356_i2c_remove),
    .driver = {
        .name = "hm0356",
    },
};

static int hm0356_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
    int rc = i2c_add_driver(&hm0356_i2c_driver);
    uint16_t model_id = 0;

    printk(KERN_INFO "hm0356_msg: hm0356_sensor_probe() is called.\n");

    if (rc < 0 || hm0356_client == NULL) {
        rc = -ENOTSUPP;
        goto probe_done;
    }
    hm0356info = info;

    /* Init VGA pins state */

    rc = fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 0);
    if (rc)
        return rc;

    rc = fih_cam_output_gpio_control(hm0356info->mclk_sw_pin, "hm0356", 1);
    msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
    msleep(30);
    rc = fih_cam_vreg_control(hm0356info->cam_vreg_vddio_id, 1800, 1);
    if (rc)
        goto probe_done;

    mdelay(1); // t2    //5

    if (hm0356info->cam_v2p8_en_pin == 0xffff )
        rc = fih_cam_vreg_control(hm0356info->cam_vreg_acore_id, 2800, 1);
    else
        rc = fih_cam_output_gpio_control(hm0356info->cam_v2p8_en_pin, "hm0356", 1);

    if (rc)
        goto probe_done;
    mdelay(1);

    /* Power Down Pin */
    rc = fih_cam_output_gpio_control(hm0356info->vga_pwdn_pin, "hm0356", 0);
    if (rc)
        goto probe_done;

    mdelay(1);  //t1

    /* Input MCLK = 24MHz */
    msm_camio_clk_rate_set(24000000);
    gpio_tlmm_config(GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

    mdelay(2);  //t2
    rc = hm0356_i2c_read(hm0356_client->addr,0x0002, &model_id, FC_BYTE_LEN);
    if (rc)
        goto probe_done;
    printk(KERN_INFO "hm0356_msg: hm0356_sensor_probe() is model_id=0x%x.\n",model_id);
    if(model_id!=0x56)
    {
        rc=-1;
        goto probe_done;
    }

    s->s_init = hm0356_sensor_init;
    s->s_release = hm0356_sensor_release;
    s->s_config  = hm0356_sensor_config;

probe_done:
    hm0356_power_off();
    msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
    CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
    return rc;
}

static int __hm0356_probe(struct platform_device *pdev)
{
    printk(KERN_WARNING "hm0356_msg: in __hm0356_probe() because name of msm_camera_hm0356 match.\n");
    return msm_camera_drv_start(pdev, hm0356_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
    .probe = __hm0356_probe,
    .driver = {
        .name = "msm_camera_hm0356",
        .owner = THIS_MODULE,
    },
};

static int __init hm0356_init(void)
{
    return platform_driver_register(&msm_camera_driver);
}

module_init(hm0356_init);

