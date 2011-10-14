/*
 *     ds2482.c - DS2482-101 Single-Channel 1-Wire Master With Sleep Mode
 *
 *     Copyright (C) 2010 Audi PC Huang <audipchuang@fihtdc.com>
 *     Copyright (C) 2010 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/mfd/pmic8058.h> /* for controlling pmic8058 GPIO */ 
#include <mach/gpio.h>
#include <mach/ds2482.h>
#include <mach/ds2784.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
//#include <mach/msm_iomap.h>
//#include <mach/msm_smd.h>
#include <linux/wakelock.h>

#define ONE_WIRE_BUSY_RETRIES   10
#define ONE_WIRE_RESET_RETRIES  3

struct ds2482_driver_data {
    struct mutex ds2482_suspend_lock;
    struct i2c_client *ds2482_i2c_client;
    unsigned int pmic_gpio_ds2482_SLPZ;
    unsigned int sys_gpio_ds2482_SLPZ;
    unsigned int sys_gpio_gauge_ls_en;
    unsigned int HWID;
    struct wake_lock ds2482_wakelock;
};

static struct ds2482_driver_data ds2482_drvdata;

#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
int g_err_code = 0;

static ssize_t ds2482_ftm_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", g_err_code);
}

static DEVICE_ATTR(ftm, 0644, ds2482_ftm_show, NULL);
#endif

struct mutex* ds2482_get_suspend_lock (void)
{
    return &ds2482_drvdata.ds2482_suspend_lock;
}
EXPORT_SYMBOL(ds2482_get_suspend_lock);

/*************I2C functions*******************/
static int ds2482_get_i2c_bus(struct i2c_client *client)
{
    struct i2c_adapter *adap = client->adapter;
    int ret;

    if (adap->algo->master_xfer) {
        if (in_atomic() || irqs_disabled()) {
            ret = mutex_trylock(&adap->bus_lock);
            if (!ret)
                /* I2C activity is ongoing. */
                return -EAGAIN;
        } else {
            mutex_lock_nested(&adap->bus_lock, adap->level);
        }

        return 0;
    } else {
        dev_err(&client->dev, "I2C level transfers not supported\n");
        return -EOPNOTSUPP;
    }
}

static void ds2482_release_i2c_bus(struct i2c_client *client)
{
    struct i2c_adapter *adap = client->adapter;
 
    mutex_unlock(&adap->bus_lock);
}

static int ds2482_control( struct i2c_client *client, u8 *data, int w_len, int r_len)
{
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msgs_r[] = {
        {
            .addr   = client->addr,
            .flags  = I2C_M_RD,
            .len    = r_len,
            .buf    = data,
        }
    };
    struct i2c_msg msgs_wr[] = {
        {
            .addr   = client->addr,
            .flags  = 0,
            .len    = w_len,
            .buf    = data,
        },
        {
            .addr   = client->addr,
            .flags  = I2C_M_RD,
            .len    = r_len,
            .buf    = data + w_len,
        },
    };
    
    int ret;

    if (w_len == 0)
        ret = adap->algo->master_xfer( adap, msgs_r, ARRAY_SIZE(msgs_r));
    else
        ret = adap->algo->master_xfer( adap, msgs_wr, ARRAY_SIZE(msgs_wr));
        
    if ( 0 > ret ) {
        printk( KERN_INFO "i2c rx failed %d\n", ret );
        return -EIO;
    } else
        return 0;
}

static int ds2482_write( struct i2c_client *client, u8 *txdata, int length )
{
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg[] =
    {
        {
            .addr = client->addr,
            .flags = 0,
            .len = length,
            .buf = txdata,
        },
    };
    int ret;
    
    ret = adap->algo->master_xfer( adap, msg, 1 );
    if ( 0 > ret ) {
        dev_err(&client->dev, "i2c tx failed %d\n", ret );
        return ret;
    }

    return ret;
}

static int ds2482_set_read_pointer(u8 reg, u8 *reg_value)
{
    u8 cmd[3]   = {CMD_SET_READ_POINT, reg, 0xFF};
    
    if (ds2482_control(ds2482_drvdata.ds2482_i2c_client, cmd, 2, 1) < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: Set read pointer 0x%02x failed!!\n", __func__, reg);
        return -EIO;
    }
    
    *reg_value = cmd[2]; 
    
    return 0;
}

static int ds2482_read_register(u8* reg, const char reg_name[])
{
    int rc = 0;
    
    rc = ds2482_control(ds2482_drvdata.ds2482_i2c_client, reg, 0, 1);
    if (rc < 0) {
        /* Read status register failed */
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: Get current ds2482 %s failed!!\n", __func__, reg_name);
    } else {
        dev_dbg(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: %s = 0x%02x!!\n", __func__, reg_name, *reg);
    }
    
    return rc;
}

static int ds2482_reset(void)
{
    int rc              = 0;
    u8 cmd[3]           = {CMD_DEVICE_RESET, 0xFF};

    /* Write device reset command to one-wire bridge. */
    rc = ds2482_control(ds2482_drvdata.ds2482_i2c_client, cmd, 1, 1);
    if (rc < 0) {
        /* Write command failed. */
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: one-wire bridge reset failed!\n", __func__);
    } else { 
        ndelay(530); //wait for device reseting
        dev_dbg(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: one-wire bridge status register = 0x%02x\n", __func__, cmd[1]);
        
        cmd[0] = CMD_WRITE_CONFIGURATION;
        cmd[1] = 0xE1;
        cmd[2] = 0xFF;
        if (ds2482_control(ds2482_drvdata.ds2482_i2c_client, cmd, 2, 1) == 0) {
            /* Write configuration register successfully */
            if ((cmd[2] & 0xFF) == 0x01) {
                /* Configuration register was reset */
                return 0;
            }
            
            /* One-wire reset failed because configuration and status registers was not reset successfully */
            dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: One-wire bridge reset failed!!\n", __func__);
            return -1;
        }
    }

    return rc;
}

static int ds2482_check_1WB(void)
{
    int rc      = 0;
    int i       = 0;
    u8 status   = 0x00;

    rc = ds2482_set_read_pointer(REGISTER_STATUS, &status);
    if (rc < 0) {
        /* Set read pointer failed */
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: Set read pointer failed!!\n", __func__);
        return rc;
    }
    
    /* Set read pointer successfully */
    while (status & (1 << STATUS_REG_BIT_1WB)) {
        ndelay(100);
        rc = ds2482_read_register(&status, "status");
        if (rc < 0) {
            dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: Read status failed!!\n", __func__);
            return rc;
        }

        i++; /* counter */
        if (i == ONE_WIRE_BUSY_RETRIES) {
            /* 1WB shows that one-wire is very busy and ten retries was finished */
            dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: One-wire bridge is very busy!!\n", __func__);
            
            return -1;
        }
    }

    return 0;
}

int ds2482_one_wire_reset(void)
{
    int rc      = 0;
    u8 cmd      = CMD_OW_RESET;
    
    /* 1. check if the 1-wire slave is busy */
    /*rc = ds2482_check_1WB();
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: One-wire reset command was not acknowledged!!\n", __func__);
        ds2482_reset();
        return rc;
    }*/
    
    /* 2. Write 1-wire reset command to bridge */
    rc = ds2482_write(ds2482_drvdata.ds2482_i2c_client, &cmd, sizeof(cmd));
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: One-wire reset command was failed!!\n", __func__);
    }
    
    /*  Idle Time for waiting 1-wire bus not busy */
    udelay(600);    //tRSTL
    udelay(584);    //tRSTH
    //ndelay(263);   //maximum
        
    rc = ds2482_check_1WB();
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: 1-wire is still busy!!\n", __func__);
    }
    
    return rc;
}
EXPORT_SYMBOL(ds2482_one_wire_reset);

int ds2482_one_wire_read_byte(u8* data)
{
    int rc      = 0;
    u8 cmd   = {CMD_OW_READ_BYTE};

    /* 1. check if the 1-wire slave is busy */
    /*rc = ds2482_check_1WB();
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: One-wire read byte command was not acknowledged!!\n", __func__);
        ds2482_reset();
        return rc;
    }*/

    /* 2. Write 1-wire read byte command to bridge */
    rc = ds2482_write(ds2482_drvdata.ds2482_i2c_client, &cmd, sizeof (cmd));
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: Write one-wire read byte command failed!!\n", __func__);
        return rc;
    }

    /*  Idle Time for waiting 1-wire bus not busy */
    udelay(73 * 8); //tSLOT x 8
    //ndelay(263);    //maximum
    
    rc = ds2482_check_1WB();
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: 1-wire is still busy!!\n", __func__);
        return rc;
    }

    /* 3. Read the byte data from DS2482 buffer */
    rc = ds2482_set_read_pointer(REGISTER_READ_DATA, data); /* Set read pointer to read data register */
    if (rc < 0)
        return rc;
        
    dev_dbg(&ds2482_drvdata.ds2482_i2c_client->dev, "data = 0x%02x!!\n", *data);

    return rc;
}
EXPORT_SYMBOL(ds2482_one_wire_read_byte);

int ds2482_one_wire_write_byte(u8 data)
{
    int rc      = 0;
    u8 cmd[2]   = {CMD_OW_WRITE_BYTE, data};
    
    /* 1. check if 1-wire is busy  */
    /*rc = ds2482_check_1WB();
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: One-wire write byte command was not acknowledged!!\n", __func__);          
        rc = ds2482_reset();
        return rc;
    }*/
    
    /* 2. ask DS2482 write one data byte to 1-wire slave */
    rc = ds2482_write(ds2482_drvdata.ds2482_i2c_client, cmd, sizeof(cmd));
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: Write one-wire write byte command failed!!\n", __func__);
        return rc;
    }

    /*  Idle Time for waiting 1-wire bus not busy */
    udelay(73 * 8); //tSLOT x 8
    //ndelay(263);    //maximum
    
    rc = ds2482_check_1WB();
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: 1-wire is still busy!!\n", __func__);
        return rc;
    }
        
    return rc;
}
EXPORT_SYMBOL(ds2482_one_wire_write_byte);

int ds2482_turn_on(void)
{
    int ret = 0;
    wake_lock(&ds2482_drvdata.ds2482_wakelock);
    mutex_lock(&ds2482_drvdata.ds2482_suspend_lock);
    gpio_set_value_cansleep(ds2482_drvdata.sys_gpio_gauge_ls_en, 1);
    gpio_set_value_cansleep(ds2482_drvdata.sys_gpio_ds2482_SLPZ, 1);

    udelay(100);
    ret = ds2482_get_i2c_bus(ds2482_drvdata.ds2482_i2c_client);
    if (ret < 0 ) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: get i2c bus failed!!\n", __func__);
#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
        g_err_code = -5;
#endif
        wake_unlock(&ds2482_drvdata.ds2482_wakelock);
        return ret;
    }
    
    return ds2482_reset();
}
EXPORT_SYMBOL(ds2482_turn_on);

void ds2482_turn_off(void)
{
    ds2482_release_i2c_bus(ds2482_drvdata.ds2482_i2c_client);
    gpio_set_value_cansleep(ds2482_drvdata.sys_gpio_ds2482_SLPZ, 0);
    gpio_set_value_cansleep(ds2482_drvdata.sys_gpio_gauge_ls_en, 0);
    mutex_unlock(&ds2482_drvdata.ds2482_suspend_lock);
    wake_unlock(&ds2482_drvdata.ds2482_wakelock);
}
EXPORT_SYMBOL(ds2482_turn_off);

#ifdef CONFIG_PM
static int ds2482_suspend(struct i2c_client *ow_bridge, pm_message_t mesg)
{
    return 0;
}

static int ds2482_resume(struct i2c_client *ow_bridge)
{
    return 0;
}
#else
# define ds2482_suspend NULL
# define ds2482_resume  NULL
#endif

static int ds2482_config_gpio(void)
{
    int rc = 0;

    struct pm8058_gpio SLPZ_configuration = {
        .direction      = PM_GPIO_DIR_OUT,
        .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
        .output_value   = 1,
        .out_strength   = PM_GPIO_STRENGTH_LOW,
        .function       = PM_GPIO_FUNC_NORMAL,
    };
    
    rc = pm8058_gpio_config(ds2482_drvdata.pmic_gpio_ds2482_SLPZ - 1, &SLPZ_configuration);
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: gpio %d configured failed\n", __func__, ds2482_drvdata.pmic_gpio_ds2482_SLPZ);
        return rc;
    } else {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: gpio %d configured successfully\n", __func__, ds2482_drvdata.pmic_gpio_ds2482_SLPZ);
        
        rc = gpio_request(ds2482_drvdata.sys_gpio_ds2482_SLPZ, "ds2482_SLPZ");
        if (rc < 0) {
            dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: gpio %d requested failed\n", __func__, ds2482_drvdata.sys_gpio_ds2482_SLPZ);
            return rc;
        }
        
        gpio_direction_output(ds2482_drvdata.sys_gpio_ds2482_SLPZ, 1);
        gpio_set_value_cansleep(ds2482_drvdata.sys_gpio_ds2482_SLPZ, 1);
    }
    
    rc = gpio_tlmm_config(GPIO_CFG(ds2482_drvdata.sys_gpio_gauge_ls_en, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    if (rc < 0) {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: gpio %d configured failed\n", __func__, ds2482_drvdata.sys_gpio_gauge_ls_en);
        return rc;
    } else {
        dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: gpio %d configured successfully\n", __func__, ds2482_drvdata.sys_gpio_gauge_ls_en);
        
        rc = gpio_request(ds2482_drvdata.sys_gpio_gauge_ls_en, "Gauge_LS_EN");
        if (rc < 0) {
            dev_err(&ds2482_drvdata.ds2482_i2c_client->dev, "%s: gpio %d requested failed\n", __func__, ds2482_drvdata.sys_gpio_gauge_ls_en);
            return rc;
        }
     
        gpio_direction_output(ds2482_drvdata.sys_gpio_gauge_ls_en, 1);
        gpio_set_value_cansleep(ds2482_drvdata.sys_gpio_gauge_ls_en, 1);
    }
    
    
    return rc;
}

/******************Driver Functions*******************/
#ifdef CONFIG_BATTERY_FIH_DS2784
static struct platform_device ds2784_battery_device = {
    .name   = "ds2784_battery",
    .id     = -1,
};
#endif

static int __devinit ds2482_probe(struct i2c_client *client,
                    const struct i2c_device_id *id)
{
    struct ds2482_platform_data *ds2482_pd = client->dev.platform_data;
    int ret = 0;

    i2c_set_clientdata(client, &ds2482_drvdata);
    ds2482_drvdata.ds2482_i2c_client    = client;
    ds2482_drvdata.pmic_gpio_ds2482_SLPZ    = ds2482_pd->pmic_gpio_ds2482_SLPZ; /* pm8058 GPIO */
    ds2482_drvdata.sys_gpio_ds2482_SLPZ     = ds2482_pd->sys_gpio_ds2482_SLPZ;
    ds2482_drvdata.sys_gpio_gauge_ls_en     = ds2482_pd->sys_gpio_gauge_ls_en; /* For PR1 */
    //ds2482_drvdata.HWID               = FIH_READ_HWID_FROM_SMEM();
    mutex_init(&ds2482_drvdata.ds2482_suspend_lock);
    wake_lock_init(&ds2482_drvdata.ds2482_wakelock, WAKE_LOCK_SUSPEND, "charger_state_suspend_work");
    
    ret = ds2482_config_gpio();
    if (ret < 0) {
        dev_err(&client->dev, "Config SLPZ pin failed\n");
        mutex_destroy(&ds2482_drvdata.ds2482_lock);
#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
        g_err_code = -4;
#else
        return ret;
#endif
    }

    ret = ds2482_turn_on();
    if (ret < 0) {
#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
        if (g_err_code == 0) {
            g_err_code = -6;
        }
#endif
        dev_err(&client->dev, "ds2482 reset failed!!\n");
        
#ifndef CONFIG_FIH_FTM_BATTERY_CHARGING
        ds2482_turn_off(); 
        return ret;
#endif
    }
    ds2482_turn_off();

#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
    ret = device_create_file(&client->dev, &dev_attr_ftm);
    if (ret) {
        dev_err(&client->dev,
               "%s: dev_attr_ftm failed\n", __func__);
    }
#endif

#ifdef CONFIG_BATTERY_FIH_DS2784
    platform_device_register(&ds2784_battery_device);
#endif

    return 0;
}

static int __devexit ds2482_remove(struct i2c_client *client)
{
    int ret = 0;
    
    platform_device_unregister(&ds2784_battery_device);
    mutex_destroy(&ds2482_drvdata.ds2482_suspend_lock);
    wake_lock_destroy(&ds2482_drvdata.ds2482_wakelock);

    return ret;
}

static const struct i2c_device_id ds2482_idtable[] = {
       { "ds2482", 0 },
       { }
};

static struct i2c_driver ds2482_driver = {
    .driver = {
        .name   = "ds2482",
    },
    .probe      = ds2482_probe,
    .remove     = __devexit_p(ds2482_remove),
    .suspend    = ds2482_suspend,
    .resume     = ds2482_resume,
    .id_table   = ds2482_idtable,
};

static int __init ds2482_init(void)
{
    int ret = 0;

    printk(KERN_INFO "Driver init: %s\n", __func__ );

    // i2c
    ret = i2c_add_driver(&ds2482_driver);
    if (ret) {
        printk(KERN_ERR "%s: Driver registration failed, module not inserted.\n", __func__);
        goto driver_del;
    }

    //all successfully.
    return ret;

driver_del:
    i2c_del_driver(&ds2482_driver);
    
    return -1;
}

static void __exit ds2482_exit(void)
{
    printk(KERN_INFO "Driver exit: %s\n", __func__ );
    i2c_del_driver(&ds2482_driver);
}

module_init(ds2482_init);
module_exit(ds2482_exit);
MODULE_VERSION("1.0");
MODULE_AUTHOR( "Audi PC Huang <audipchuang@fihtdc.com>" );
MODULE_DESCRIPTION( "DS2482 driver" );
MODULE_LICENSE("GPL");
