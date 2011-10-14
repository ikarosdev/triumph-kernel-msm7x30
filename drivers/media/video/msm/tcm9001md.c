/*
 *     tcm9001md.c - Camera Sensor Config
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
#include "tcm9001md.h"
#include <mach/pmic.h>
#include <linux/completion.h>
#include <linux/hrtimer.h>
#include "fih_camera_parameter.h"
#include "fih_camera_power.h"
#include "../../../arch/arm/mach-msm/smd_private.h" 

/* Micron tcm9001md Registers and their values */
/* Sensor Core Registers */
#define  REG_TCM9001MD_MODEL_ID_1  0x00
#define  REG_TCM9001MD_MODEL_ID_2  0x01
#define  TCM9001MD_MODEL_ID_1     0x48
#define  TCM9001MD_MODEL_ID_2     0x10

/*  SOC Registers Page 1  */
#define  REG_tcm9001md_SENSOR_RESET     0x001A
#define  REG_tcm9001md_STANDBY_CONTROL  0x0018
#define  REG_tcm9001md_MCU_BOOT         0x001C

typedef enum 
{
    Low=0,
    High
}Pin_state;

struct tcm9001md_work {
    struct work_struct work;
};

static struct  tcm9001md_work *tcm9001md_sensorw;
static struct  i2c_client *tcm9001md_client;
static const struct msm_camera_sensor_info *tcm9001mdinfo;

struct tcm9001md_ctrl {
    const struct msm_camera_sensor_info *sensordata;
};

static struct tcm9001md_ctrl *tcm9001md_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(tcm9001md_wait_queue);
DECLARE_MUTEX(tcm9001md_sem);
DEFINE_MUTEX(tcm9001md_mut);
#ifdef CONFIG_MT9P111_RESUME
extern struct mutex front_mut;
#endif

static unsigned int tcm9001md_supported_effects[] = {
    CAMERA_EFFECT_OFF
};

static unsigned int tcm9001md_supported_autoexposure[] = {
    CAMERA_AEC_FRAME_AVERAGE

};

static unsigned int tcm9001md_supported_antibanding[] = {
    CAMERA_ANTIBANDING_OFF
};

static unsigned int tcm9001md_supported_wb[] = {
    CAMERA_WB_AUTO 
};

static unsigned int tcm9001md_supported_led[] = {
    LED_MODE_OFF,

};
static unsigned int tcm9001md_supported_ISO[] = {
    CAMERA_ISO_ISO_AUTO,
};

static unsigned int tcm9001md_supported_focus[] = {
    DONT_CARE
};
static unsigned int tcm9001md_supported_lensshade[] = {
    FALSE
};

static unsigned int tcm9001md_supported_scenemode[] = {
    CAMERA_BESTSHOT_OFF
};

static unsigned int tcm9001md_supported_continuous_af[] = {
    FALSE
};

static unsigned int tcm9001md_supported_touchafaec[] = {
    FALSE
};

static unsigned int tcm9001md_supported_frame_rate_modes[] = {
    FPS_MODE_AUTO
};

struct sensor_parameters tcm9001md_parameters = {
    .autoexposuretbl = tcm9001md_supported_autoexposure,
    .autoexposuretbl_size = sizeof(tcm9001md_supported_autoexposure)/sizeof(unsigned int),
    .effectstbl = tcm9001md_supported_effects,
    .effectstbl_size = sizeof(tcm9001md_supported_effects)/sizeof(unsigned int),
    .wbtbl = tcm9001md_supported_wb,
    .wbtbl_size = sizeof(tcm9001md_supported_wb)/sizeof(unsigned int),
    .antibandingtbl = tcm9001md_supported_antibanding,
    .antibandingtbl_size = sizeof(tcm9001md_supported_antibanding)/sizeof(unsigned int),
    .flashtbl = tcm9001md_supported_led,
    .flashtbl_size = sizeof(tcm9001md_supported_led)/sizeof(unsigned int),
    .focustbl = tcm9001md_supported_focus,
    .focustbl_size = sizeof(tcm9001md_supported_focus)/sizeof(unsigned int),
    .ISOtbl = tcm9001md_supported_ISO,
    .ISOtbl_size = sizeof( tcm9001md_supported_ISO)/sizeof(unsigned int),
    .lensshadetbl = tcm9001md_supported_lensshade,
    .lensshadetbl_size = sizeof(tcm9001md_supported_lensshade)/sizeof(unsigned int),
    .scenemodetbl = tcm9001md_supported_scenemode,
    .scenemodetbl_size = sizeof( tcm9001md_supported_scenemode)/sizeof(unsigned int),
    .continuous_aftbl = tcm9001md_supported_continuous_af,
    .continuous_aftbl_size = sizeof(tcm9001md_supported_continuous_af)/sizeof(unsigned int),
    .touchafaectbl = tcm9001md_supported_touchafaec,
    .touchafaectbl_size = sizeof(tcm9001md_supported_touchafaec)/sizeof(unsigned int),
    .frame_rate_modestbl = tcm9001md_supported_frame_rate_modes,
    .frame_rate_modestbl_size = sizeof( tcm9001md_supported_frame_rate_modes)/sizeof(unsigned int),
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
extern struct tcm9001md_reg tcm9001md_regs;

#ifdef CONFIG_TCM9001MD_STANDBY
static int tcm9001md_first_init = 1;
#endif

/*=============================================================*/
//Div2-SW6-MM-MC-AddErrorHandlingWhenCameraI2cReadAndWriteFail-00*{
static int tcm9001md_i2c_read(struct i2c_client *c, unsigned char reg,
		unsigned char *value)
{
    int ret;

    ret = i2c_smbus_read_byte_data(c, reg);
    if (ret >= 0) {
        *value = (unsigned char) ret;
        ret = 0;
    }
    else
    {
        printk("tcm9001md_i2c_read: ERR: reg = 0x%x !\n", reg);
        printk("tcm9001md_i2c_read: ERR: ret = 0x%x !\n", ret);
        ret = -EIO;
    }
    return ret;
}

static int tcm9001md_i2c_write(struct i2c_client *c, unsigned char reg,
		unsigned char value)
{
    int ret = i2c_smbus_write_byte_data(c, reg, value);
    if (ret < 0)
    {
        printk("tcm9001md_i2c_write: ERR: reg = 0x%x, value = 0x%x !\n", reg, value);
        printk("tcm9001md_i2c_write: ERR: ret = 0x%x !\n", ret);
        ret = -EIO;
    }
    //if (reg == REG_COM7 && (value & COM7_RESET))
        //mdelay(2);  /* Wait for reset to run */
    return ret;
}
//Div2-SW6-MM-MC-AddErrorHandlingWhenCameraI2cReadAndWriteFail-00*}

/*
 * Write a list of register settings; ff/ff stops the process.
 */
static int32_t  tcm9001md_i2c_write_array(struct tcm9001md_i2c_reg_conf const *reg_conf_tbl, int num_of_items_in_table)
{
    int i;
    int32_t rc = -EIO;

    for (i = 0; i < num_of_items_in_table; i++) {
        rc = tcm9001md_i2c_write(tcm9001md_client, reg_conf_tbl->waddr, reg_conf_tbl->wdata);

        if (rc < 0)
            break;
        if (reg_conf_tbl->mdelay_time != 0)
            mdelay(reg_conf_tbl->mdelay_time);

        reg_conf_tbl++;
    }

    return rc;
}

static long tcm9001md_reg_init(void)
{
    long rc = 0;

    rc = tcm9001md_i2c_write_array(&tcm9001md_regs.inittbl[0], tcm9001md_regs.inittbl_size);
    if (rc < 0)
    {   
        printk("tcm9001md_reg_init: ERR: Initial setting failed !\n");
        return rc;
    }
    else
        printk("tcm9001md_reg_init: Finish Initial Setting for tcm9001md.\n");

    return rc;
}

//Div2-SW6-MM-MC-ImplementCameraTestModeForTcm9001mdSensor-00+{
static long tcm9001md_set_effect(int mode, int effect)
{
    long rc = 0;
    printk("%s: mode = %d, effect = %d\n", __func__, mode, effect);

    switch (effect) 
    {
        case CAMERA_EFFECT_COLORBAR: 
        {
            printk("%s: case = CAMERA_EFFECT_COLORBAR ~~ \n", __func__);

            //Enable test mode for color bar pattern.
            rc = tcm9001md_i2c_write(tcm9001md_client, 0x53, 0x4C);
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
//Div2-SW6-MM-MC-ImplementCameraTestModeForTcm9001mdSensor-00+}

static long tcm9001md_set_sensor_mode(int mode)
{

    switch (mode) 
    {
        case SENSOR_PREVIEW_MODE:
        {
            printk(KERN_INFO "tcm9001md_set_sensor_mode: case SENSOR_PREVIEW_MODE.\n");
        }		
            break;

        case SENSOR_SNAPSHOT_MODE:
        {
            printk(KERN_INFO "tcm9001md_set_sensor_mode: case SENSOR_SNAPSHOT_MODE.\n");
        }
            break;

        case SENSOR_RAW_SNAPSHOT_MODE:
        {
            printk(KERN_INFO "tcm9001md_set_sensor_mode: case SENSOR_RAW_SNAPSHOT_MODE.\n");
        }
            break;

        default:
        {
            printk(KERN_ERR "tcm9001md_set_sensor_mode: ERR: Invalid case !\n");
        }
        return -EINVAL;
    }

    return 0;
}

#ifdef CONFIG_TCM9001MD_STANDBY
int tcm9001md_sensor_standby(int on)
{
    int rc = 0;

    if(on == 1)//Entering stanby mode
    {
        //Software standby setting. Set this register with STANDBY.
        rc = tcm9001md_i2c_write(tcm9001md_client, 0xFF, 0x05);//SLEEP: 0h: ON (Standby), 1h: OFF (Normal)
        if (rc < 0)
            return rc;

        mdelay(1);

        rc = tcm9001md_i2c_write(tcm9001md_client, 0x18, 0x1C);//STANDBY: 0h: ON (Standby), 1h: OFF (Normal)
        if (rc < 0)
            return rc;
        printk(KERN_INFO "%s: SW standby done . \n", __func__);

        mdelay(1);

        // Enter HW standby
        /* Pull Low REDET = CAM_VGA_RST_N  */
        rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_rst_pin, "tcm9001md", 0);
        if (rc)
            return rc;

        mdelay(1);

        /* Pull hight PWRDWN = CAM_VGA_STANDBY  */
        rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_pwdn_pin, "tcm9001md", 1);
        if (rc)
            return rc;

        mdelay(1);

        /* Disable MCLK = 24MHz */
        gpio_tlmm_config(GPIO_CFG(tcm9001mdinfo->MCLK_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        rc = fih_cam_output_gpio_control(tcm9001mdinfo->MCLK_PIN, "tcm9001md", 0);
        if (rc)
            return rc;
        printk("%s: Disable mclk\n", __func__);

        printk(KERN_INFO "%s: Enter Hw standby done . \n", __func__);
    }
    else//Exiting stanby mode
    {
        /* Setting MCLK = 24MHz */
        msm_camio_clk_rate_set(24000000);
        msm_camio_camif_pad_reg_reset();
        printk(KERN_INFO "%s: Setting MCLK = 24MHz \n", __func__);

        gpio_tlmm_config(GPIO_CFG(tcm9001mdinfo->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);//Div2-SW6-MM-MC-Camera-ModifiedPowerSequence-01*
        printk(KERN_INFO "%s: Output MCLK end.  \n", __func__);

        mdelay(1);

        /* Pull Low PWRDWN = CAM_VGA_STANDBY  */
        rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_pwdn_pin, "tcm9001md", 0);
        if (rc)
            return rc;

        mdelay(1);

        /* Pull High REDET = CAM_VGA_RST_N  */
        rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_rst_pin, "tcm9001md", 1);
        if (rc)
            return rc;

        mdelay(5);//Div2-SW6-MM-MC-AddErrorHandlingWhenCameraI2cReadAndWriteFail-00+
        
        //Exit SW standby
        rc = tcm9001md_i2c_write(tcm9001md_client, 0x18, 0x9C);//STANDBY: 0h: ON (Standby), 1h: OFF (Normal)
        if (rc < 0)
            return rc;

        mdelay(1);

        rc = tcm9001md_i2c_write(tcm9001md_client, 0xFF, 0x85);//SLEEP: 0h: ON (Standby), 1h: OFF (Normal)
        if (rc < 0)
            return rc;
        printk(KERN_INFO "%s: Exit HW standby done. \n", __func__);
    }

    return rc;
}
#endif

int tcm9001md_power_on(void)
{
    int rc = 0;

    /* Setting MCLK = 24MHz */
    msm_camio_clk_rate_set(24000000);
    msm_camio_camif_pad_reg_reset();
    printk(KERN_INFO "%s: Setting MCLK = 24MHz \n", __func__);

    gpio_tlmm_config(GPIO_CFG(tcm9001mdinfo->vga_rst_pin, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    printk(KERN_INFO "%s: Re-set config for gpio %d .\n", __func__, tcm9001mdinfo->vga_rst_pin);

    gpio_tlmm_config(GPIO_CFG(tcm9001mdinfo->vga_pwdn_pin, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    printk(KERN_INFO "%s: Re-set config for gpio %d .\n", __func__, tcm9001mdinfo->vga_pwdn_pin);

    gpio_tlmm_config(GPIO_CFG(tcm9001mdinfo->vga_power_en_pin, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    printk(KERN_INFO "%s: Re-set config for gpio %d .\n", __func__, tcm9001mdinfo->vga_power_en_pin);

    /* Pull hight power enable = GPIO98  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_power_en_pin, "tcm9001md", 1);
    if (rc)
        return rc;

    mdelay(1);  //t1+t2+t3 = 1ms

    /* Pull hight PWRDWN = CAM_VGA_STANDBY  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_pwdn_pin, "tcm9001md", 1);
    if (rc)
        return rc;

    mdelay(2);  //t4 = 1ms

    /* Enable  MCLK = 24MHz */
    gpio_tlmm_config(GPIO_CFG(tcm9001mdinfo->MCLK_PIN, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);//Div2-SW6-MM-MC-Camera-ModifiedPowerSequence-01*
    printk(KERN_INFO "%s: Output MCLK end.  \n", __func__);

    mdelay(2);

    /* Pull Low PWRDWN = CAM_VGA_STANDBY  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_pwdn_pin, "tcm9001md", 0);
    if (rc)
        return rc;

    mdelay(2);  //t5 = 2ms

    /* Pull High REDET = CAM_VGA_RST_N  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_rst_pin, "tcm9001md", 1);
    if (rc)
        return rc;

    mdelay(2);  //t6 > 2ms

    return rc;
}

int tcm9001md_power_off(void)
{
    int rc = 0;

    /* Pull Low REDET = CAM_VGA_RST_N  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_rst_pin, "tcm9001md", 0);
    if (rc)
        return rc;

    mdelay(1);  //t1 > 0ms

    /* Pull hight PWRDWN = CAM_VGA_STANDBY  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_pwdn_pin, "tcm9001md", 1);
    if (rc)
        return rc;

    mdelay(1);  //t2 = 1ms

    /* Disable MCLK = 24MHz */

    gpio_tlmm_config(GPIO_CFG(tcm9001mdinfo->MCLK_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->MCLK_PIN, "tcm9001md", 0);
    if (rc)
        return rc;
    printk("%s: Disable mclk\n", __func__);

    mdelay(1);  //t3 = 1ms

    /* Pull Low power enable = GPIO98  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_power_en_pin, "tcm9001md", 0);
    if (rc)
        return rc;

    //mdelay(1);  //t4+t5 > 0ms

    /* Pull Low PWRDWN = CAM_VGA_STANDBY  */
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_pwdn_pin, "tcm9001md", 0);
    if (rc)
        return rc;

#ifdef CONFIG_TCM9001MD_STANDBY
    tcm9001md_first_init = 1;
#endif

    return rc;
}

static int tcm9001md_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    int rc = 0;
    unsigned char v = 0;
    printk("tcm9001md_sensor_init_probe: Entry.\n");

    printk("tcm9001md_sensor_init_probe entry.\n");
    sensor_init_parameters(data,&tcm9001md_parameters);

#ifdef CONFIG_TCM9001MD_STANDBY
    if (tcm9001md_first_init == 1)
    {
        tcm9001md_first_init = 0;

        rc = tcm9001md_power_on();
        if (rc < 0)
        {
            printk("tcm9001md_sensor_init_probe: ERR: tcm9001md_power_on() failed for first init !\n");
            goto init_probe_fail;
        }
    }
    else
    {
        rc = tcm9001md_sensor_standby(0);
        if (rc < 0)
        {
            printk("tcm9001md_sensor_init_probe: ERR: tcm9001md_sensor_standby(0) failed !\n");
            goto init_probe_fail;
        }
    }
#else
    rc = tcm9001md_power_on();
    if (rc < 0)
    {
        printk("tcm9001md_sensor_init_probe: ERR: tcm9001md_power_on() failed !\n");
        goto init_probe_fail;
    }
#endif

    rc = tcm9001md_reg_init();
    if (rc < 0)
    {
        rc = - EIO;
        printk("tcm9001md_sensor_init_probe: ERR: tcm9001md_reg_init() failed, err = %d !\n", rc);
        goto init_probe_fail;
    }

    // Here to check chip version identification.
    rc = tcm9001md_i2c_read(tcm9001md_client, REG_TCM9001MD_MODEL_ID_1, &v);
    if (rc < 0 || v != TCM9001MD_MODEL_ID_1)
    {
        printk("tcm9001md_sensor_init_probe: ERR: Red MODEL_ID_1 = 0x%x failed !\n", v);
        goto init_probe_fail;
    }
    printk("tcm9001md_sensor_init_probe: MODEL_ID_1 = 0x%x .\n", v);

    rc = tcm9001md_i2c_read(tcm9001md_client, REG_TCM9001MD_MODEL_ID_2, &v);
    if (rc < 0 || v != TCM9001MD_MODEL_ID_2)
    {
        printk("tcm9001md_sensor_init_probe: ERR: Red MODEL_ID_2 = 0x%x failed !\n", v);
        goto init_probe_fail;
    }
    printk("tcm9001md_sensor_init_probe: MODEL_ID_2 = 0x%x .\n", v);

    return rc;

init_probe_fail:
    printk("tcm9001md_sensor_init_probe: ERR: FAIL !\n");
    return rc;
}

int tcm9001md_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc = 0;
#ifdef CONFIG_MT9P111_RESUME
    mutex_lock(&front_mut);
#endif

    tcm9001md_ctrl = kzalloc(sizeof(struct tcm9001md_ctrl), GFP_KERNEL);
    if (!tcm9001md_ctrl) {
        CDBG("tcm9001md_sensor_init: ERR: kzalloc failed!\n");
        rc = -ENOMEM;
        goto init_done;
    }

    if (data)
        tcm9001md_ctrl->sensordata = data;

    rc = tcm9001md_sensor_init_probe(data);
    if (rc < 0) {
        CDBG("tcm9001md_sensor_init: ERR: Sensor probe failed!\n");
        goto init_fail;
    }

init_done:
    return rc;

init_fail:
    kfree(tcm9001md_ctrl);
    return rc;
}

static int tcm9001md_init_client(struct i2c_client *client)
{
    /* Initialize the MSM_CAMI2C Chip */
    init_waitqueue_head(&tcm9001md_wait_queue);
    return 0;
}

int tcm9001md_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long   rc = 0;

    if (copy_from_user(&cfg_data,(void *)argp,sizeof(struct sensor_cfg_data)))
        return -EFAULT;

	/* down(&tcm9001md_sem); */

    CDBG("tcm9001md_sensor_config: cfgtype = %d, mode = %d\n",cfg_data.cfgtype, cfg_data.mode);

    switch (cfg_data.cfgtype) 
    {
        case CFG_SET_MODE:
        {
            rc = tcm9001md_set_sensor_mode(
            cfg_data.mode);
        }
            break;

        //Div2-SW6-MM-MC-ImplementCameraTestModeForTcm9001mdSensor-00+{
        case CFG_SET_EFFECT:
        {
            rc = tcm9001md_set_effect(cfg_data.mode,cfg_data.cfg.effect);
        }
            break;
        //Div2-SW6-MM-MC-ImplementCameraTestModeForTcm9001mdSensor-00+}

        default:
            break;
    }

    return rc;
}

int tcm9001md_sensor_release(void)
{
    int rc = 0;

    printk("tcm9001md_sensor_release()+++\n");

    mutex_lock(&tcm9001md_mut);

#ifdef CONFIG_TCM9001MD_STANDBY
    rc = tcm9001md_sensor_standby(1);
#else
    rc = tcm9001md_power_off();
#endif

    if (rc)
        return rc;
    
    kfree(tcm9001md_ctrl);
    tcm9001md_ctrl = NULL;

    mutex_unlock(&tcm9001md_mut);
#ifdef CONFIG_MT9P111_RESUME
    mutex_unlock(&front_mut);
#endif

    printk("tcm9001md_sensor_release()---\n");

    return rc;
}

static int tcm9001md_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
    int rc = 0;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    tcm9001md_sensorw =
    kzalloc(sizeof(struct tcm9001md_work), GFP_KERNEL);

    if (!tcm9001md_sensorw) {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, tcm9001md_sensorw);
    tcm9001md_init_client(client);
    tcm9001md_client = client;

    CDBG("tcm9001md_probe succeeded!\n");

    return 0;

probe_failure:
    kfree(tcm9001md_sensorw);
    tcm9001md_sensorw = NULL;
    CDBG("tcm9001md_probe failed!\n");
    return rc;
}

static const struct i2c_device_id tcm9001md_i2c_id[] = {
    { "tcm9001md", 0},
    { },
};

static struct i2c_driver tcm9001md_i2c_driver = {
    .id_table = tcm9001md_i2c_id,
    .probe  = tcm9001md_i2c_probe,
    .remove = __exit_p(tcm9001md_i2c_remove),
    .driver = {
        .name = "tcm9001md",
    },
};

static int tcm9001md_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
    int rc = i2c_add_driver(&tcm9001md_i2c_driver);

    printk(KERN_INFO "tcm9001md_sensor_probe: Called.....\n");

    if (rc < 0 || tcm9001md_client == NULL) {
        rc = -ENOTSUPP;
        goto probe_done;
    }
    tcm9001mdinfo= info;

    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_rst_pin, "tcm9001md", 0);
    if (rc)
        return rc;
    
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_pwdn_pin, "tcm9001md", 0);
    if (rc)
        return rc;
    
    rc = fih_cam_output_gpio_control(tcm9001mdinfo->vga_power_en_pin, "tcm9001md", 0);
    if (rc)
        return rc;

    s->s_init = tcm9001md_sensor_init;
    s->s_release = tcm9001md_sensor_release;
    s->s_config  = tcm9001md_sensor_config;

probe_done:
    CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
    return rc;
}

static int __tcm9001md_probe(struct platform_device *pdev)
{
    printk(KERN_WARNING "__tcm9001md_probe: Because name of msm_camera_tcm9001md match.\n");
    return msm_camera_drv_start(pdev, tcm9001md_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
    .probe = __tcm9001md_probe,
    .driver = {
        .name = "msm_camera_tcm9001md",
        .owner = THIS_MODULE,
    },
};

static int __init tcm9001md_init(void)
{
    return platform_driver_register(&msm_camera_driver);
}

module_init(tcm9001md_init);

