/*
 * BQ275x0 battery driver
 *
 * Copyright (C) 2010 Audi PC Huang <AudiPCHuang@fihtdc.com>
 * Copyright (C) 2008 Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2008 Eurotech S.p.A. <info@eurotech.it>
 *
 * Based on a previous work by Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/mfd/pmic8058.h> /* for controlling pmic8058 GPIO */ 
#include <linux/wakelock.h>
#include <linux/proc_fs.h>
#include <linux/switch.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h> 
#include <linux/io.h>
#endif
#include <mach/gpio.h>
#include <mach/bq275x0_battery.h>
#include <mach/ds2784.h>
#include <mach/irqs.h>
#include "../../arch/arm/mach-msm/smd_private.h"

#if defined(CONFIG_FIH_POWER_LOG)
#include "linux/pmlog.h"
#endif

#define BATTERY_POLLING_TIMER   60000
#define RETRY_DELAY             100
#define FAILED_MARK             0xFFFFFFFF
#define BATTERY_GONE            0x474F4E45

int bq275x0_battery_elan_update_mode = 0;

/* If the system has several batteries we need a different name for each
 * of them...
 */
static DEFINE_IDR(battery_id);
static DEFINE_MUTEX(battery_mutex);
static DEFINE_MUTEX(flags_mutex);

struct bq275x0_device_info {
    struct device       *dev;
    int         id;
    int         voltage_uV;
    int         current_uA;
    int         temp_C;
    int         charge_rsoc;
    int         health;
    int         present;
    struct power_supply bat;
    
    int     polling_interval;

    unsigned    pmic_BATLOW;
    unsigned    pmic_BATGD;
    
    bool        dev_ready;
    bool        chg_inh;
    bool        is_suspend;
    bool        dbg_enable;
    bool        charging;
    bool        full;
    bool        rommode;
    bool        soc1;
    bool        socf;
    bool        temp_over400;

    struct timer_list polling_timer;
    struct delayed_work bq275x0_BATGD;
    struct delayed_work bq275x0_BATLOW;
    struct delayed_work bq275x0_update_soc;
    struct delayed_work check_dfi;
    struct work_struct bq275x0_update;

    struct i2c_client   *client;
    
    struct wake_lock bq275x0_wakelock;
    
    short chg_inh_temp_h;
    short chg_inh_temp_l;
    short temp_hys;
    short terminate_voltage;
    unsigned short BATLOW_enable_voltage;
    
    struct manufacturer_info gauge_info;
    struct switch_dev dfi_status;
    
    void __iomem *base;
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend bq275x0_early_suspend;
#endif
};

static enum power_supply_property bq275x0_battery_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_CAPACITY,
    POWER_SUPPLY_PROP_TEMP,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_TECHNOLOGY,
};

static int test_value[BQ275X0_CBVERIFY_MAX];
static bool test_item[BQ275X0_CBVERIFY_MAX];
static bool test_on = false;
static int test_accuracy = 0;
static char file_name[64];
static bool ignore_batt_det = false;
static struct proc_dir_entry *cbverify_proc_entry;
static int product_id = 0;

//Slate Code Start
int fih_charger; //Slate Code DEBUG
EXPORT_SYMBOL(fih_charger);
//Slate Code End
static struct bq275x0_device_info *di;
extern u32 msm_batt_get_batt_status(void);
extern void msm_batt_notify_over_temperature(bool over_temperature);
extern void msm_batt_notify_battery_full(bool full);
extern void msm_batt_notify_charging_state(bool charging);
extern void msm_hsusb_notify_charging_state(bool charging);
extern void msm_batt_notify_battery_presence(bool presence);
extern void msm_batt_notify_over_400(bool over400);
extern int msmrtc_timeremote_read_time(struct device *dev, struct rtc_time *tm);
extern void msm_batt_disable_discharging_monitor(bool disable);
extern u32 msm_batt_get_vbatt_voltage(void);
extern int msm_batt_get_batt_level(void);
extern void bq275x0_RomMode_get_bqfs_version(char* prj_name, int *version, u8* date);
extern void msmrtc_set_wakeup_cycle_time(int cycle_time);

void bq275x0_battery_ignore_battery_detection(bool ignore)
{
    ignore_batt_det = ignore;
}
EXPORT_SYMBOL(bq275x0_battery_ignore_battery_detection);

static void bq275x0_endian_adjust(u8* buf, int len)
{
    int i;
    u8 temp[4];
    
    for (i = 0; i < len; i++)
        temp[(len - 1) - i] = buf[i];
    for (i = 0; i < len; i++)
        buf[i] = temp[i];
}

/*
 * bq275x0 specific code
 */
static int bq275x0_read(struct bq275x0_device_info *di, u8 cmd, u8 *data, int length)
{
    int ret;
    
    struct i2c_msg msgs[] = {
        [0] = {
            .addr   = di->client->addr,
            .flags  = 0,
            .buf    = (void *)&cmd,
            .len    = 1
        },
        [1] = {
            .addr   = di->client->addr,
            .flags  = I2C_M_RD,
            .buf    = (void *)data,
            .len    = length
        }
    };

    ret = i2c_transfer(di->client->adapter, msgs, 2);
    mdelay(1);
    return (ret < 0) ? -1 : 0;    
}

static int bq275x0_write(struct bq275x0_device_info *di, u8 *cmd, int length)
{
    int ret;
    
    struct i2c_msg msgs[] = {
        [0] = {
            .addr   = di->client->addr,
            .flags  = 0,
            .buf    = (void *)cmd,
            .len    = length
        },
    };

    ret = i2c_transfer(di->client->adapter, msgs, 1);
    mdelay(16);
    return (ret < 0) ? -1 : 0;    
}

static int bq275x0_battery_control_command(u16 subcommand, u8* data, int len)
{
    u8 cmd[3];
    
    cmd[0] = BQ275X0_CMD_CNTL;
    memcpy(&cmd[1], &subcommand, sizeof(u16));
    
    if (bq275x0_write(di, cmd, sizeof(cmd)))
        return -1;
    if (len != 0)
        if (bq275x0_read(di, BQ275X0_CMD_CNTL, data, len))
            return -1;
    
    return 0;
}

/*
static int bq275x0_battery_unseal(void)
{
    int ret = 0;
    u16 subcommand;
    u8 *cmd = (u8*)&subcommand;
    
    cmd[0] = 0x14;
    cmd[1] = 0x04;
    ret = bq275x0_battery_control_command(subcommand,
                                            NULL,
                                            0
                                            );
    cmd[0] = 0x72;
    cmd[1] = 0x36;
    ret = bq275x0_battery_control_command(subcommand,
                                            NULL,
                                            0
                                            );
    cmd[0] = 0xFF;
    cmd[1] = 0xFF;
    ret = bq275x0_battery_control_command(subcommand,
                                            NULL,
                                            0
                                            );    
    cmd[0] = 0xFF;
    cmd[1] = 0xFF;
    ret = bq275x0_battery_control_command(subcommand,
                                            NULL,
                                            0
                                            );
    return ret;
}
*/

static u16 bq275x0_battery_control_status(void)
{
    int ret;
    u16 control_status = 0;

    ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_CONTROL_STATUS,
                                            (u8*)&control_status,
                                            sizeof(u16)
                                            );
    if (ret) {
        dev_err(di->dev, "error reading control status\n");
        return ret;
    }

    return control_status;
}

int bq275x0_battery_access_data_flash(u8 subclass, u8 offset, u8* buf, int len, bool write)
{
    int i       = 0;
    unsigned sum= 0;
    u8 cmd[33];
    u8 block_num= offset / 32;
    u8 cmd_code = BQ275X0_EXT_CMD_DFD + offset % 32;
    u8 checksum = 0;
    
    /* Set SEALED/UNSEALED mode. */
    cmd[0] = BQ275X0_EXT_CMD_DFDCNTL;
    cmd[1] = 0x00;
    if (bq275x0_write(di, cmd, 2))
        return -1;
    
    /* Set subclass */
    cmd[0] = BQ275X0_EXT_CMD_DFCLS;
    cmd[1] = subclass;
    if (bq275x0_write(di, cmd, 2))
        return -1;
    
    /* Set block to R/W */
    cmd[0] = BQ275X0_EXT_CMD_DFBLK;
    cmd[1] = block_num;
    if (bq275x0_write(di, cmd, 2))
        return -1;

    if (write) {
        /* Calculate checksum */
        for (i = 0; i < len; i++)
            sum += buf[i];
        checksum = 0xFF - (sum & 0x00FF);
    
        /* Write data */
        cmd[0] = cmd_code;
        memcpy((cmd + 1), buf, len);
        if (bq275x0_write(di, cmd, (len + 1)))
            return -1;

        /* Set checksum */
        cmd[0] = BQ275X0_EXT_CMD_DFDCKS;
        cmd[1] = checksum;
        if (bq275x0_write(di, cmd, 2))
            return -1;
    } else {
        /* Read data */
        if (bq275x0_read(di, cmd_code, buf, len))
            return -1;
    }
    
    return 0;
}

static int bq275x0_battery_standard_cmds(int cmd)
{
    int ret;
    s16 value;

    ret = bq275x0_read(di, cmd, (u8*)&value, sizeof(s16));
    if (ret) {
        dev_warn(di->dev, "error reading %d: retry\n", cmd);
        mdelay(RETRY_DELAY);    /* retry */
        ret = bq275x0_read(di, cmd, (u8*)&value, sizeof(s16));
        if (ret) {
            dev_err(di->dev, "error reading %d\n", cmd);
            return FAILED_MARK;
        }
    }

    return value;
}

static int bq275x0_battery_flags(u16 *flags)
{
    int ret;

    ret = bq275x0_read(di, BQ275X0_CMD_FLAGS, (u8*)flags, sizeof(u16));
    if (ret) {
        dev_warn(di->dev, "error reading flags: retry\n");
        mdelay(RETRY_DELAY);    /* retry */
        ret = bq275x0_read(di, BQ275X0_CMD_FLAGS, (u8*)flags, sizeof(u16));
        if (ret) {
            dev_err(di->dev, "error reading flags\n");
        }
    }

    return ret;
}

/*
 * Return the battery temperature in Celsius degrees
 * Or < 0 if something fails.
 */
int bq275x0_battery_temperature(void)
{
    int ret;
    s16 temp = 0;

    ret = bq275x0_read(di, BQ275X0_CMD_TEMP, (u8*)&temp, sizeof(s16));
    if (ret) {
        dev_warn(di->dev, "error reading temperature: retry\n");
        mdelay(RETRY_DELAY);    /* retry */
        ret = bq275x0_read(di, BQ275X0_CMD_TEMP, (u8*)&temp, sizeof(s16));
        if (ret) {
            dev_err(di->dev, "error reading temperature\n");
            return FAILED_MARK;
        }
    }

    if (test_on && test_item[BQ275X0_CBVERIFY_OTP_TEST])
        return test_value[BQ275X0_CBVERIFY_OTP_TEST];
    else
        return temp - (273 * 10);
}
EXPORT_SYMBOL(bq275x0_battery_temperature);

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
int bq275x0_battery_voltage(void)
{
    int ret;
    s16 volt = 0;

    ret = bq275x0_read(di, BQ275X0_CMD_VOLT, (u8*)&volt, sizeof(s16));
    if (ret) {
        dev_warn(di->dev, "error reading voltage: retry\n");
        mdelay(RETRY_DELAY);    /* retry */
        ret = bq275x0_read(di, BQ275X0_CMD_VOLT, (u8*)&volt, sizeof(s16));
        if (ret) {
            dev_err(di->dev, "error reading voltage\n");
            return FAILED_MARK;
        }
    }

    return volt * 1000;
}
EXPORT_SYMBOL(bq275x0_battery_voltage);

/*
 * Return the battery average current
 * Note that current can be negative signed as well
 * Or 0 if something fails.
 */
int bq275x0_battery_current(void)
{
    int ret;
    s16 curr = 0;

    ret = bq275x0_read(di, BQ275X0_CMD_AI, (u8*)&curr, sizeof(s16));
    if (ret) {
        dev_warn(di->dev, "error reading current: retry\n");
        mdelay(RETRY_DELAY);    /* retry */
        ret = bq275x0_read(di, BQ275X0_CMD_AI, (u8*)&curr, sizeof(s16));
        if (ret) {
            dev_err(di->dev, "error reading current\n");
            return FAILED_MARK;
        }
    }
    
    return curr;
}
EXPORT_SYMBOL(bq275x0_battery_current);

/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
int bq275x0_battery_soc(void)
{
    int ret;
    u16 soc = 0;

    ret = bq275x0_read(di, BQ275X0_CMD_SOC, (u8*)&soc, sizeof(s16));
    if (ret) {
        dev_warn(di->dev, "error reading soc: retry\n");
        mdelay(RETRY_DELAY);    /* retry */
        ret = bq275x0_read(di, BQ275X0_CMD_SOC, (u8*)&soc, sizeof(s16));
        if (ret) {
            dev_err(di->dev, "error reading relative State-of-Charge\n");
            return FAILED_MARK;
        }
    }
    
    return soc;
}
EXPORT_SYMBOL(bq275x0_battery_soc);

/*
 * Return the battery health
 */
int bq275x0_battery_health(void)
{
    if (!di->present)
        return POWER_SUPPLY_HEALTH_UNKNOWN;
    if (!(di->chg_inh_temp_h > di->temp_C))
        return POWER_SUPPLY_HEALTH_OVERHEAT;
    if (!(di->chg_inh_temp_l < di->temp_C))
        return POWER_SUPPLY_HEALTH_COLD;
        
    return POWER_SUPPLY_HEALTH_GOOD;
}
EXPORT_SYMBOL(bq275x0_battery_health);

int bq275x0_battery_fw_version(void)
{
    int ret;
    u16 fw_version = 0;

    ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_FW_VERSION,
                                            (u8*)&fw_version,
                                            sizeof(u16)
                                            );
    if (ret) {
        dev_err(di->dev, "error reading firmware version\n");
        return FAILED_MARK;
    }

    return fw_version;
}
EXPORT_SYMBOL(bq275x0_battery_fw_version);

int bq275x0_battery_device_type(void)
{
    int ret;
    u16 device_type;

    ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_DEVICE_TYPE,
                                            (u8*)&device_type,
                                            sizeof(u16)
                                            );
    if (ret) {
        dev_err(di->dev, "error reading firmware version\n");
        return FAILED_MARK;
    }

    return device_type;
}
EXPORT_SYMBOL(bq275x0_battery_device_type);

int bq275x0_battery_IT_enable(void)
{
    int ret;
    int control_status;
    int retries = 0;

    ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_IT_ENABLE,
                                            NULL,
                                            0
                                            );
    if (ret) {
        dev_err(di->dev, "error IT Enable\n");
        return ret;
    }
    
    mdelay(200);
    control_status = bq275x0_battery_control_status();
    while ((control_status & (0x1 << BQ275X0_CNTLSTATUS_QEN)) != (0x1 << BQ275X0_CNTLSTATUS_QEN)) {
        if (retries > 10) {
            dev_info(di->dev, "IT Enable Failed\n");
            return -1;
        } else
            retries++;
        mdelay(50);
        control_status = bq275x0_battery_control_status();
    }
    
    dev_info(di->dev, "IT Enable Finished\n");
    return ret;
}
EXPORT_SYMBOL(bq275x0_battery_IT_enable);

int bq275x0_battery_reset(void)
{
    int ret;
    int control_status;  
    int retries = 0;  

    ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_RESET,
                                            NULL,
                                            0
                                            );
    if (ret) {
        dev_err(di->dev, "error reset\n");
        return ret;
    }

    mdelay(3300);
    control_status = bq275x0_battery_control_status();
    while ((control_status & (0x1 << BQ275X0_CNTLSTATUS_RUP_DIS)) != (0x1 << BQ275X0_CNTLSTATUS_RUP_DIS)) {
        if (retries > 20) {
            dev_info(di->dev, "Reset Failed\n");
            return -1;
        } else
            retries++;
        mdelay(50);
        control_status = bq275x0_battery_control_status();        
    }
    
    dev_info(di->dev, "Gauge RESET Finished\n");
    return ret;
}
EXPORT_SYMBOL(bq275x0_battery_reset);

int bq275x0_battery_sealed(void)
{
    int ret;

    ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_SEALED,
                                            NULL,
                                            0
                                            );
    if (ret) {
        dev_err(di->dev, "error sealed\n");
        return ret;
    }

    dev_info(di->dev, "End Gauge SEALED\n");
    return ret;
}
EXPORT_SYMBOL(bq275x0_battery_sealed);

static int bq275x0_battery_manufacturer_info(char* buf, u8 offset)
{
    int ret = 0;
    
    ret = bq275x0_battery_access_data_flash(57,
                                            offset,
                                            (u8*)buf,
                                            32,
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error reading operation_configuration\n");
    }
    
    return ret;
}

void bq275x0_battery_exit_rommode(void)
{
    di->rommode = false;
    dev_info(di->dev, "Exit Rom Mode\n");
}
EXPORT_SYMBOL(bq275x0_battery_exit_rommode);

int bq275x0_battery_enter_rom_mode(void)
{
    int ret = 0;

    di->rommode = true;
    ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_ROM_MODE,
                                            NULL,
                                            0
                                            );
    if (ret) {
        dev_err(di->dev, "failed to place bq275x0 into ROM MODE\n");
    }

    return ret;
}
EXPORT_SYMBOL(bq275x0_battery_enter_rom_mode);

#if 0
static int bq275x0_battery_application_status(void)
{
    int ret;
    u16 application_status;

    ret = bq275x0_read(di,
                        BQ275X0_EXT_CMD_APPSTAT,
                        (u8*)&application_status,
                        sizeof(application_status)
                        );
    if (ret) {
        dev_err(di->dev, "error reading application status\n");
        return ret;
    }
    
    return application_status;
}

static int bq275x0_battery_operation_configuration(void)
{
    int ret;
    u16 operation_configuration;
    
    ret = bq275x0_battery_access_data_flash(64,
                                            0,
                                            (u8*)&operation_configuration,
                                            sizeof(operation_configuration),
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error reading operation_configuration\n");
        return ret;
    }
    
    return operation_configuration;
}
#endif

int bq275x0_battery_snooze_mode(bool set)
{
    int ret;
    u16 control_status = 0;

    if (set)
        ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_SET_SLEEP_PLUS,
                                                NULL,
                                                0
                                                );
    else
        ret = bq275x0_battery_control_command( BQ275X0_CNTLDATA_CLEAR_SLEEP_PLUS,
                                                NULL,
                                                0
                                                );
    if (ret) {
        dev_err(di->dev, "error reading firmware version\n");
        return ret;
    }
    
    control_status = bq275x0_battery_control_status();
    if (set) {
        return !(control_status & (1 << BQ275X0_CNTLSTATUS_SNOOZE));
    } else {
        return control_status & (1 << BQ275X0_CNTLSTATUS_SNOOZE);
    }

    return ret;
}
EXPORT_SYMBOL(bq275x0_battery_snooze_mode);

int bq275x0_battery_hibernate_mode(bool set)
{
    int ret;
    u16 control_status = 0;

    if (set)
        ret = bq275x0_battery_control_command(BQ275X0_CNTLDATA_SET_HIBERNATE,
                                                NULL,
                                                0
                                                );
    else
        ret = bq275x0_battery_control_command( BQ275X0_CNTLDATA_CLEAR_HIBERNATE,
                                                NULL,
                                                0
                                                );
    if (ret) {
        dev_err(di->dev, "error set/clr Hibernate mode\n");
        return ret;
    }
    
    control_status = bq275x0_battery_control_status();
    if (set) {
        return !(control_status & (1 << BQ275X0_CNTLSTATUS_HIBERNATE));
    } else {
        return control_status & (1 << BQ275X0_CNTLSTATUS_HIBERNATE);
    }

    return ret;
}
EXPORT_SYMBOL(bq275x0_battery_hibernate_mode);

void bq275x0_battery_get_gauge_dfi_info(struct manufacturer_info* info)
{
    memcpy(info, &di->gauge_info, 32);
}
EXPORT_SYMBOL(bq275x0_battery_get_gauge_dfi_info);

static int bq275x0_battery_charge_inhibit_subclass(void)
{
    int ret;
    
    ret = bq275x0_battery_access_data_flash(32,
                                            0,
                                            (u8*)&di->chg_inh_temp_l,
                                            sizeof(s16),
                                            false
                                            );
    if (ret) {;
        dev_err(di->dev, "error reading charge inhibit temp low\n");
        return ret;
    }
    
    ret = bq275x0_battery_access_data_flash(32,
                                            2,
                                            (u8*)&di->chg_inh_temp_h,
                                            sizeof(s16),
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error reading charge inhibit temp high\n");
        return ret;
    }
    
    ret = bq275x0_battery_access_data_flash(32,
                                            4,
                                            (u8*)&di->temp_hys,
                                            sizeof(s16),
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error reading temp_hys\n");
        return ret;
    }

    bq275x0_endian_adjust((u8*)&di->chg_inh_temp_l, sizeof(u16));
    bq275x0_endian_adjust((u8*)&di->chg_inh_temp_h, sizeof(u16));
    bq275x0_endian_adjust((u8*)&di->temp_hys, sizeof(u16));
    
    return 0;
}

#if 0
static int bq275x0_battery_BATLOW_enable_voltage(u16 voltage)
{
    int ret;
    u8 buf[32];
    u16 *p_voltage = (u16*)&buf[18];
    
    ret = bq275x0_battery_access_data_flash(68,
                                            0,
                                            (u8*)buf,
                                            32,
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error reading BATLOW enable voltage 1\n");
        return ret;
    }
    bq275x0_endian_adjust((u8*)&voltage, sizeof(u16));
    
    if (*p_voltage != voltage) {
        *p_voltage = voltage;

        ret = bq275x0_battery_access_data_flash(68,
                                                0,
                                                (u8*)buf,
                                                32,
                                                true
                                                );
        if (ret) {
            dev_err(di->dev, "error writing BATLOW enable voltage\n");
            return ret;
        }
        mdelay(100);
    }
    
    return 0;
}

static int bq275x0_battery_terminate_voltage(s16 voltage)
{
    int ret;
    u8 buf[32];
    u16 *p_voltage = (u16*)&buf[18];
    
    ret = bq275x0_battery_access_data_flash(80,
                                            32,
                                            (u8*)buf,
                                            32,
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error reading terminate_voltage 1\n");
        return ret;
    }
    
    bq275x0_endian_adjust((u8*)&voltage, sizeof(s16));
    if (*p_voltage != (u16)voltage) {        
        *p_voltage = (u16)voltage;

        dev_info(di->dev, "write terminate voltage\n");
        ret = bq275x0_battery_access_data_flash(80,
                                                32,
                                                (u8*)buf,
                                                32,
                                                true
                                                );
        if (ret) {
            dev_err(di->dev, "error writing terminate_voltage\n");
            return ret;
        }
        mdelay(100);
        
        ret = bq275x0_battery_access_data_flash(80,
                                                18,
                                                &buf[18],
                                                2,
                                                false
                                                );
        if (ret) {
            dev_err(di->dev, "error reading terminate_voltage 1\n");
            return ret;
        }
    }
    
    bq275x0_endian_adjust((u8*)p_voltage, sizeof(s16));
    dev_info(di->dev, "Terminate Voltage = %d \n", *p_voltage);
    
    return 0;
}

static int bq275x0_battery_SOC1(void)
{
    int ret;
    u8 buf[32];
    
    ret = bq275x0_battery_access_data_flash(49,
                                            0,
                                            (u8*)buf,
                                            32,
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error reading terminate_voltage 1\n");
        return ret;
    }
    
    dev_info(di->dev, "SOC1 SET %d, SOC1 CLR %d, SOCF SET %d, SOCF CLR %d\n", buf[0], buf[1], buf[2], buf[3]);
    
    return 0;
}
#endif

static int bq275x0_battery_check_IT_enable(void)
{
    int ret;
    u8 buf[32];
    
    ret = bq275x0_battery_access_data_flash(82,
                                            0,
                                            (u8*)buf,
                                            32,
                                            false
                                            );
    if (ret) {
        dev_err(di->dev, "error checking IT enable (1)\n");
        return ret;
    }
    
    dev_info(di->dev, "IT_Enable %d\n", buf[0]);
    if (!buf[0]) {
        bq275x0_battery_IT_enable();
        bq275x0_battery_reset();
        ret = bq275x0_battery_access_data_flash(82,
                                                0,
                                                (u8*)buf,
                                                32,
                                                false
                                                );
        if (ret) {
            dev_err(di->dev, "error checking IT enable(2)\n");
            return ret;
        }
        dev_info(di->dev, "Check IT_Enable %d\n", buf[0]);
    }
    
    return 0;
}

static void bq275x0_battery_set_wakeup_cycle_time(void)
{
    if (di->chg_inh) {      /* OT */
        dev_info(&di->client->dev, "Wakeup Cycle: 1 min\n");
        msmrtc_set_wakeup_cycle_time(60);
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("Wakeup Cycle: 1 min\n");
#endif
    } else if (product_id == Product_FB3 && di->temp_over400) {
        dev_info(&di->client->dev, "Wakeup Cycle: 5 min\n");
        msmrtc_set_wakeup_cycle_time(300);
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("Wakeup Cycle: 5 min\n");
#endif
    } else if (di->socf) {  /* Critial Low Battery Alarm */
        dev_info(&di->client->dev, "Wakeup Cycle: 10 min\n");
        msmrtc_set_wakeup_cycle_time(600);
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("Wakeup Cycle: 10 min\n");
#endif
    } else if (di->soc1) {  /* Low Battery Alarm */
        dev_info(&di->client->dev, "Wakeup Cycle: 30 min\n");
        msmrtc_set_wakeup_cycle_time(1800);
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("Wakeup Cycle: 30 min\n");
#endif
    } else {                /* Normal */
		if (product_id == Product_FB3 && di->temp_over400)
			return;
		
        dev_info(&di->client->dev, "Wakeup Cycle: Stop\n");
        msmrtc_set_wakeup_cycle_time(0);
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("Wakeup Cycle: 0 min\n");
#endif
    }
}

static void bq275x0_battery_update_flags(u16 *pflags)
{
    u16 flags               = 0;
    int ret                 = 0;
    bool current_chg_inh    = false;
    bool current_FC         = false;

    mutex_lock(&flags_mutex);
    ret = bq275x0_battery_flags(&flags);
    *pflags = flags;
    if (!ret) {
        di->charging = flags & (0x1 << BQ275X0_FLAGS_DSG) ? false : true;
        di->present = flags & (0x1 << BQ275X0_FLAGS_BAT_DET) ? true : false;
        current_FC = flags & (0x1 << BQ275X0_FLAGS_FC) ? true : false;
        
        if (test_on && test_item[BQ275X0_CBVERIFY_LOW_BAT_TEST]) {
            di->soc1 = test_value[BQ275X0_CBVERIFY_LOW_BAT_TEST] & 0x2;
            di->socf = test_value[BQ275X0_CBVERIFY_LOW_BAT_TEST] & 0x1;
            switch (test_value[BQ275X0_CBVERIFY_LOW_BAT_TEST]) {
            case 3:    
                di->charge_rsoc = 5;
                break;
            case 2:
                di->charge_rsoc = 10;
            }
        } else {
            di->soc1 = flags & (0x1 << BQ275X0_FLAGS_SOC1) ? true : false;
            di->socf = flags & (0x1 << BQ275X0_FLAGS_SOCF) ? true : false;
        }
        
        if (!di->present) {
            dev_err(&di->client->dev, "The battery is removed!!\n");
            writel(BATTERY_GONE, di->base);
            msm_batt_notify_battery_presence(false);
        }

        if (test_on && test_item[BQ275X0_CBVERIFY_OTP_TEST]) {
            if (test_value[BQ275X0_CBVERIFY_OTP_TEST] <= di->chg_inh_temp_l || 
                test_value[BQ275X0_CBVERIFY_OTP_TEST] >= di->chg_inh_temp_h)
                current_chg_inh = true;
            else
                current_chg_inh = false;
        } else
            current_chg_inh = flags & (0x1 << BQ275X0_FLAGS_CHG_INH) ? true : false;
        
        if (current_chg_inh != di->chg_inh) {
            dev_info(&di->client->dev, "CHG_INH flag changed %d!!\n", current_chg_inh);
            di->chg_inh = current_chg_inh;
            if (!(test_on && test_item[BQ275X0_CBVERIFY_REM_OTP]))
                msm_batt_notify_over_temperature(di->chg_inh);
        }

        if (current_FC != di->full) {
            di->full = current_FC;
            msm_batt_notify_battery_full(di->full);
        }
        if ((di->full || (msm_batt_get_batt_level() == 100 && !(product_id == Product_FB3 && di->temp_over400))) && di->charge_rsoc != 100) {
            dev_info(&di->client->dev, "FC flag is set, but soc is not 100\n");
            di->charge_rsoc = 100;
        }
//Slate Code Start
        if (fih_charger == 1)
	    	msm_batt_notify_charging_state(false);
		else if (fih_charger == 2)
	    	msm_batt_notify_charging_state(true); //Slate Code Ends
        else if (test_on && test_item[BQ275X0_CBVERIFY_CHARGING_TEST])
            msm_batt_notify_charging_state(test_value[BQ275X0_CBVERIFY_CHARGING_TEST]);            
        else
            msm_batt_notify_charging_state(di->charging);

        dev_info(&di->client->dev, "flags = 0x%04x\n", flags);
    } else {
        dev_err(&di->client->dev, "Read FLAGS register failed\n");
    }
    mutex_unlock(&flags_mutex);
}

static void bq275x0_battery_check_temperature_over400(void)
{
	static bool initial = false;
	
    mutex_lock(&battery_mutex);
	if (!initial) {
		if (di->temp_C > 400)
	        di->temp_over400 = true;
		else
			di->temp_over400 = false;
	
		dev_info(&di->client->dev, "[INIT] FB3 Charging Thresholds\n");
		msm_batt_notify_over_400(di->temp_over400);
		initial = true;
	}
	
    if (!di->temp_over400 && di->temp_C > 400) {
        di->temp_over400 = true;
        msm_batt_notify_over_400(di->temp_over400);
    } else if (di->temp_over400 && di->temp_C < 351) {
        di->temp_over400 = false;
        msm_batt_notify_over_400(di->temp_over400);
    }
    mutex_unlock(&battery_mutex);
}

static unsigned long elapsed_time = 0;
static void bq275x0_battery_update_batt_status(void)
{
    char buf[64];
    mm_segment_t oldfs; 
    struct file *fp = NULL;
    int pmic_adc    = msm_batt_get_vbatt_voltage();
    int voltage_uV  = bq275x0_battery_voltage();
    int current_uA  = bq275x0_battery_current();
    int temp_C      = bq275x0_battery_temperature();
    int charge_rsoc = bq275x0_battery_soc();
    u16 flags       = 0;
    
    di->voltage_uV  = (voltage_uV == FAILED_MARK)  ? di->voltage_uV    : voltage_uV;
    di->current_uA  = (current_uA == FAILED_MARK)  ? di->current_uA    : current_uA;
    di->temp_C      = (temp_C == FAILED_MARK)      ? di->temp_C        : temp_C;
    di->health      = bq275x0_battery_health();
    
    if (charge_rsoc < 0) {
        dev_warn(di->dev, "I2C bus may be failed. Read adc from PMIC\n");
        if (msm_batt_get_vbatt_voltage() < 3400)
            di->charge_rsoc = 0;
        else if (di->charge_rsoc == 0)
            di->charge_rsoc = (pmic_adc - 3400) / (4200 - 3400);
    } else {
        di->charge_rsoc = charge_rsoc;
    }
    
    bq275x0_battery_update_flags(&flags);
    
    if (product_id == Product_FB3) {
        bq275x0_battery_check_temperature_over400();
    }
	
    bq275x0_battery_set_wakeup_cycle_time();
    
    dev_info(&di->client->dev, "voltage = %d uV, soc = %d%%, current = %d mA, temperature = %d dC\n"
                                "Remaning Capacity = %d mAh, Time to Empty = %d minutes, Control Status = 0x%04x\n",
                di->voltage_uV,
                charge_rsoc,
                di->current_uA,
                di->temp_C,
                bq275x0_battery_standard_cmds(BQ275X0_CMD_RM),
                bq275x0_battery_standard_cmds(BQ275X0_CMD_TTE),
                bq275x0_battery_control_status()
            );
            
    if (test_accuracy) {
        oldfs = get_fs(); 
        set_fs(KERNEL_DS); 
        
        fp = filp_open(file_name, O_RDWR | O_APPEND, 0777);
        
        if (fp == NULL) {
            set_fs(oldfs);
            dev_err(&di->client->dev, "%s: Failed to open file\n", __func__);
            return;
        }
                
        snprintf(buf, sizeof(buf), "%lu\t%d\t%d\t%x\t%d\t%d\t%d\t%d\t%d\t%d\t%x\n",
                    elapsed_time++,
                    di->temp_C / 10,
                    di->voltage_uV / 1000,
                    flags,
                    bq275x0_battery_standard_cmds(BQ275X0_CMD_NAC),
                    bq275x0_battery_standard_cmds(BQ275X0_CMD_FAC),
                    bq275x0_battery_standard_cmds(BQ275X0_CMD_RM),
                    bq275x0_battery_standard_cmds(BQ275X0_CMD_FCC),
                    di->current_uA,
                    charge_rsoc,
                    bq275x0_battery_control_status()
                );
        fp->f_op->write(fp, buf, strlen(buf), &fp->f_pos);

        filp_close(fp, NULL);
        set_fs(oldfs);
    }
}

#define to_bq275x0_device_info(x) container_of((x), \
                struct bq275x0_device_info, bat);

static int bq275x0_battery_get_property(struct power_supply *psy,
                    enum power_supply_property psp,
                    union power_supply_propval *val)
{
    struct bq275x0_device_info *di = to_bq275x0_device_info(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = di->voltage_uV;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        if (di->dbg_enable || ignore_batt_det)
            val->intval = 1;
        else {
#ifdef CONFIG_FIH_BQ275X0_DISABLE_LOW_BATTERY_PROTECTION
            val->intval = 1;
#else
            val->intval = di->present;
#endif
    }
        break;
    case POWER_SUPPLY_PROP_CURRENT_NOW:
        val->intval = di->current_uA;
        break;
    case POWER_SUPPLY_PROP_CAPACITY:
        if (di->dbg_enable) {
            if (di->charge_rsoc == 0 && !di->present) { /* Read NTC Failed -> Battery may not exist */ 
                if (di->voltage_uV > 3600 * 1000)
                    val->intval = 77;
                else 
                    val->intval = 1;
            } else {
                val->intval = di->charge_rsoc;
            }
        } else {
#ifdef CONFIG_FIH_BQ275X0_DISABLE_LOW_BATTERY_PROTECTION
            if (di->voltage_uV/1000 > 3700 && di->charge_rsoc <= 0 && !di->present)
                val->intval = 15;
            else
                val->intval = di->charge_rsoc;
#else
            val->intval = di->charge_rsoc;
#endif
    }
            
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("batt : %d\%_%dmV_%dmA_%ddC_%s_%s_%s\n",
                di->charge_rsoc,    
                di->voltage_uV/1000,
                di->current_uA,
                di->temp_C/10,
                di->full ? "FULL" : "",
                di->charging ? "" : "DSG",
                di->chg_inh ? "OT" : ""
            );
#endif
        break;
    case POWER_SUPPLY_PROP_TEMP:
        val->intval = di->temp_C;
        break;
    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
        break;
    case POWER_SUPPLY_PROP_STATUS:
        val->intval =  msm_batt_get_batt_status();
        break;
    case POWER_SUPPLY_PROP_HEALTH:
        val->intval = di->health;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static void bq275x0_powersupply_init(struct bq275x0_device_info *di)
{
    di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
    di->bat.properties = bq275x0_battery_props;
    di->bat.num_properties = ARRAY_SIZE(bq275x0_battery_props);
    di->bat.get_property = bq275x0_battery_get_property;
    di->bat.external_power_changed = NULL;
}

static int bq275x0_battery_config_gpio(struct bq275x0_device_info *di)
{
#ifndef CONFIG_FIH_FTM_BATTERY_CHARGING
    struct pm8058_gpio gpio_configuration = {
        .direction      = PM_GPIO_DIR_IN,
        .pull           = PM_GPIO_PULL_NO,
        .vin_sel        = 2,
        .out_strength   = PM_GPIO_STRENGTH_NO,
        .function       = PM_GPIO_FUNC_NORMAL,
        .inv_int_pol    = 0,
    };
    int rc = 0;

    printk(KERN_INFO "%s\n", __func__);

    rc = pm8058_gpio_config(di->pmic_BATGD, &gpio_configuration);
    if (rc < 0) {
        dev_err(&di->client->dev, "%s: gpio %d configured failed\n", __func__, di->pmic_BATGD);
        return rc;
    } else {
        rc = gpio_request(di->pmic_BATGD + NR_GPIO_IRQS, "BAT_GD");
        if (rc < 0) {
            dev_err(&di->client->dev, "%s: gpio %d requested failed\n", __func__, di->pmic_BATGD + NR_GPIO_IRQS);
            return rc;
        } else {
            rc = gpio_direction_input(di->pmic_BATGD + NR_GPIO_IRQS);
            if (rc < 0) {
                dev_err(&di->client->dev, "%s: set gpio %d direction failed\n", __func__, di->pmic_BATGD + NR_GPIO_IRQS);
                return rc;
            }
        }
    }
    
    rc = pm8058_gpio_config(di->pmic_BATLOW, &gpio_configuration);
    if (rc < 0) {
        dev_err(&di->client->dev, "%s: gpio %d configured failed\n", __func__, di->pmic_BATLOW);
        return rc;
    } else {
        rc = gpio_request(di->pmic_BATLOW + NR_GPIO_IRQS, "BAT_LOW");
        if (rc < 0) {
            dev_err(&di->client->dev, "%s: gpio %d requested failed\n", __func__, di->pmic_BATLOW + NR_GPIO_IRQS);
            return rc;
        } else {
            rc = gpio_direction_input(di->pmic_BATLOW + NR_GPIO_IRQS);
            if (rc < 0) {
                dev_err(&di->client->dev, "%s: set gpio %d direction failed\n", __func__, di->pmic_BATLOW + NR_GPIO_IRQS);
                return rc;
            }
        }
    }
    
    return rc;
#else  /*CONFIG_FIH_FTM_BATTERY_CHARGING*/
    return 0;
#endif  
}

static void bq275x0_battery_release_gpio(struct bq275x0_device_info *di)
{
    gpio_free(di->pmic_BATGD + NR_GPIO_IRQS);
    gpio_free(di->pmic_BATLOW + NR_GPIO_IRQS);
}

static void bq275x0_battery_BATGD(struct work_struct *work)
{
    u16 flags = 0;
    int temp_C = 0;
    int ret = 0;
    bool current_chg_inh = false;
    bool gpio_val = (bool)gpio_get_value_cansleep(di->pmic_BATGD + NR_GPIO_IRQS);

    mutex_lock(&flags_mutex);
    ret = bq275x0_battery_flags(&flags);

    dev_info(&di->client->dev, "%s: BAT_GD  %d, flags  0x%04x\n",
                __func__,
                gpio_val,
                flags
            );
            
    if (ret) {
        if (gpio_val) {
            dev_err(&di->client->dev, "BAT_GD: the battery is removed\n");
            writel(BATTERY_GONE, di->base);
            msm_batt_notify_battery_presence(false);
            di->present = 0;
        } else {
            dev_err(&di->client->dev, "BAT_GD: i2c communication issue\n");
            di->present = 1;
            di->health  = POWER_SUPPLY_HEALTH_UNKNOWN;
        }
        power_supply_changed(&di->bat);
        wake_unlock(&di->bq275x0_wakelock);
        wake_lock_timeout(&di->bq275x0_wakelock, 1 * HZ);
        mutex_unlock(&flags_mutex);
        return;
    }

    if (test_on && test_item[BQ275X0_CBVERIFY_OTP_TEST]) {
        if (test_value[BQ275X0_CBVERIFY_OTP_TEST] <= di->chg_inh_temp_l || 
            test_value[BQ275X0_CBVERIFY_OTP_TEST] >= di->chg_inh_temp_h)
            current_chg_inh = true;
        else
            current_chg_inh = false;
    } else
        current_chg_inh = flags & (0x1 << BQ275X0_FLAGS_CHG_INH) ? 1 : 0;   /* Charge Inhibit Flag */
        
    if (current_chg_inh != di->chg_inh) {
        dev_info(&di->client->dev, "BAT_GD: CHG_INH flag changed %d!!\n", current_chg_inh);
        temp_C = bq275x0_battery_temperature();
        di->temp_C = (temp_C == FAILED_MARK) ? di->chg_inh_temp_h : temp_C;
        di->health = bq275x0_battery_health();
        di->chg_inh = current_chg_inh;
        if (!(test_on && test_item[BQ275X0_CBVERIFY_REM_OTP]))
            msm_batt_notify_over_temperature(di->chg_inh);
        bq275x0_battery_set_wakeup_cycle_time();
    }
    
    di->present = (flags & (0x1 << BQ275X0_FLAGS_BAT_DET)) ? 1 : 0;   /* Battery Detection Flag */
    if (di->present)
        dev_info(&di->client->dev, "BAT_GD: the battery is detected!!\n");
    else {
        dev_err(&di->client->dev, "BAT_GD: the battery is removed!!\n");
        writel(BATTERY_GONE, di->base);
        msm_batt_notify_battery_presence(false);
        di->health = POWER_SUPPLY_HEALTH_UNKNOWN;
    }
    
    power_supply_changed(&di->bat);
    wake_unlock(&di->bq275x0_wakelock);
    wake_lock_timeout(&di->bq275x0_wakelock, 1 * HZ);
    mutex_unlock(&flags_mutex);
}

static void bq275x0_battery_BATLOW(struct work_struct *work)
{
    int charge_rsoc = 0;
    u16 flags = 0;
    int ret = 0;
    bool gpio_val = (bool)gpio_get_value_cansleep(di->pmic_BATLOW + NR_GPIO_IRQS);

    mutex_lock(&flags_mutex);
    ret = bq275x0_battery_flags(&flags);
    charge_rsoc = bq275x0_battery_soc();
    di->charge_rsoc = (charge_rsoc == FAILED_MARK) ? 10 : charge_rsoc;
    
    if (test_on && test_item[BQ275X0_CBVERIFY_LOW_BAT_TEST]) {
        di->soc1 = test_value[BQ275X0_CBVERIFY_LOW_BAT_TEST] & 0x2;
        di->socf = test_value[BQ275X0_CBVERIFY_LOW_BAT_TEST] & 0x1;
        switch (test_value[BQ275X0_CBVERIFY_LOW_BAT_TEST]) {
        case 3:    
            di->charge_rsoc = 5;
            break;
        case 2:
            di->charge_rsoc = 10;
        }
    } else {
        di->soc1 = flags & (0x1 << BQ275X0_FLAGS_SOC1) ? true : false;
        di->socf = flags & (0x1 << BQ275X0_FLAGS_SOCF) ? true : false;
    }
    
    dev_info(&di->client->dev, "%s: BAT_LOW  %d soc = %d\n",
                __func__,
                gpio_val,
                charge_rsoc
            );
            
    if (gpio_val) {
        dev_info(&di->client->dev, "BAT_LOW: RAISED!!\n");
    } else {
        dev_info(&di->client->dev, "BAT_LOW: CLEAR!!\n");  
    }
    
    bq275x0_battery_set_wakeup_cycle_time();

    power_supply_changed(&di->bat); 
    wake_unlock(&di->bq275x0_wakelock);
    wake_lock_timeout(&di->bq275x0_wakelock, 1 * HZ);
    mutex_unlock(&flags_mutex);
}

static irqreturn_t bq275x0_battery_irqhandler(int irq, void *dev_id)
{
    struct bq275x0_device_info *di = (struct bq275x0_device_info *) dev_id;

    if (di->dev_ready && !di->rommode) {
        wake_lock(&di->bq275x0_wakelock);
        
        if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATGD) == irq) {
            if (di->is_suspend) {
                schedule_delayed_work(&di->bq275x0_BATGD,
                                      msecs_to_jiffies(100));
                di->is_suspend = false;
            } else {
                schedule_delayed_work(&di->bq275x0_BATGD,
                                      msecs_to_jiffies(1));
            }
        } else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATLOW) == irq) {
            if (di->is_suspend) {
                schedule_delayed_work(&di->bq275x0_BATLOW,
                                      msecs_to_jiffies(100));
                di->is_suspend = false;
            } else {
                schedule_delayed_work(&di->bq275x0_BATLOW,
                                      msecs_to_jiffies(1));            
            }
        } else {
            wake_unlock(&di->bq275x0_wakelock);
        }
    }
    
    return IRQ_HANDLED;
}

static int bq275x0_battery_irqsetup(struct bq275x0_device_info *di)
{
    int rc;
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATGD), &bq275x0_battery_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 "bq275x0-BATGD", di);
    if (rc < 0) {
        dev_err(&di->client->dev,
               "Could not register for bq275x0_battery interrupt %d"
               "(rc = %d)\n", di->pmic_BATGD, rc);
        rc = -EIO;
    }
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATLOW), &bq275x0_battery_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 "bq275x0-BATLOW", di);
    if (rc < 0) {
        dev_err(&di->client->dev,
               "Could not register for bq275x0_battery interrupt %d"
               "(rc = %d)\n", di->pmic_BATLOW, rc);
        rc = -EIO;
    }
        
    return rc;
}

static void bq275x0_battery_free_irq(struct bq275x0_device_info *di)
{
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATGD), di);
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATLOW), di);
}

static void bq275x0_battery_update(struct work_struct *work)
{
    if (!bq275x0_battery_elan_update_mode && !di->rommode)
    {
        bq275x0_battery_update_batt_status();
    }
        
    mod_timer(&di->polling_timer,
            jiffies + msecs_to_jiffies(di->polling_interval));
    
    power_supply_changed(&di->bat);
    wake_unlock(&di->bq275x0_wakelock);
    wake_lock_timeout(&di->bq275x0_wakelock, 1 * HZ);
}
//Slate Code Start Here
void bq27x0_battery_charging_status_update(void)
{
     //dev_info(di->dev, "Shashi DEBUG bq27x0_battery_charging_status_update\n");
     wake_lock(&di->bq275x0_wakelock);
     schedule_work(&di->bq275x0_update);
     #if 0
	 mod_timer(&di->polling_timer,
            jiffies + msecs_to_jiffies(1000));
     di->polling_interval = 1000;
     fih_update_timer = 1;
     #endif
}
//Slate code Ends here
EXPORT_SYMBOL(bq27x0_battery_charging_status_update);

static void bq275x0_battery_update_soc(struct work_struct *work)
{
    u16 flags;
    
    di->voltage_uV  = bq275x0_battery_voltage();
    di->charge_rsoc = bq275x0_battery_soc();
    di->temp_C      = bq275x0_battery_temperature();
    bq275x0_battery_update_flags(&flags);
    if (product_id == Product_FB3) {
        bq275x0_battery_check_temperature_over400();
    }
	bq275x0_battery_set_wakeup_cycle_time();
	
    dev_info(di->dev, "Update SOC=%d Voltage=%d Temp=%d\n", di->charge_rsoc, di->voltage_uV, di->temp_C);
    power_supply_changed(&di->bat);
    wake_unlock(&di->bq275x0_wakelock);
    wake_lock_timeout(&di->bq275x0_wakelock, 1 * HZ);
}

#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
enum {
    FTM_CHARGING_SET_CHARGING_STATUS,
    FTM_CHARGING_BACKUPBATTERY_ADC,  
};

struct manufacturer_info g_manufacturer_info;
static u16 g_device_type = 0;
static int g_capacity   = 0;
static int g_voltage    = 0;
static int g_temp       = 0;
static int g_family_code= 0;
static enum _battery_info_type_ g_current_cmd = 0;
static int g_smem_error = -1;
static int g_bb_voltage = -1;
static int g_charging_on= 0;

extern int proc_comm_config_coin_cell(int vset, int *voltage, int status);
extern int proc_comm_config_chg_current(bool on, int curr);

static ssize_t bq275x0_battery_ftm_battery_store(struct device *dev, 
        struct device_attribute *attr, const char *buf, size_t count)
{
    int cmd     = 0;
    
    sscanf(buf, "%d", &cmd);
    dev_info(dev, "%s: COMMAND: %d\n", __func__, cmd);

    g_current_cmd = cmd;
    switch (cmd) {
    case BATT_CAPACITY_INFO:
        g_capacity = bq275x0_battery_soc();
        break;
    case BATT_VOLTAGE_INFO:
        g_voltage = bq275x0_battery_voltage();
        break;
    case BATT_TEMPERATURE_INFO:
        g_temp = bq275x0_battery_temperature();
        break;
    case BATT_FAMILY_CODE:
        g_family_code = bq275x0_battery_fw_version();
        break;
    case BATT_GET_MANUFACTURER_INFO:
        bq275x0_battery_manufacturer_info((u8*)&g_manufacturer_info, 0);          /* block A */
        //bq275x0_endian_adjust(g_manufacturer_info.date, 4);
        break;
    case BATT_GET_DEVICE_TYPE:
        g_device_type = bq275x0_battery_device_type();
        break;
    default:
        break;
    };

    return count;
}

static ssize_t bq275x0_battery_ftm_battery_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int bqfs_version = 0;
    u8 bqfs_date[4];
    char bqfs_project_name[7];

    switch (g_current_cmd) {
    case BATT_CAPACITY_INFO:         // 0x06 RARC - Remaining Active Relative Capacity
        return sprintf(buf, "%d\n", g_capacity);
    case BATT_VOLTAGE_INFO:         // 0x0c Voltage MSB
        return sprintf(buf, "%d\n", g_voltage);
    case BATT_TEMPERATURE_INFO:      // 0x0a Temperature MSB
        return sprintf(buf, "%d\n", g_temp);
    case BATT_FAMILY_CODE:           // 0x33
        return sprintf(buf, "%x\n", g_family_code);
    case BATT_GET_MANUFACTURER_INFO:
        bq275x0_RomMode_get_bqfs_version(bqfs_project_name, &bqfs_version, bqfs_date);
        bq275x0_endian_adjust(bqfs_date, 4);
        
        if ((bqfs_version != g_manufacturer_info.dfi_ver) ||
            strncmp(bqfs_project_name, g_manufacturer_info.prj_name, 6) ||
            memcmp(bqfs_date, g_manufacturer_info.date, 4)
            )
            return sprintf(buf, "%d\n", 0);

        return sprintf(buf, "%d\n", 1);
    case BATT_GET_DEVICE_TYPE:
        return sprintf(buf, "%x\n", g_device_type);
    default:
        return 0;
    };
    
    return 0;
}

static ssize_t bq275x0_battery_ftm_charging_store(struct device *dev, 
        struct device_attribute *attr, const char *buf, size_t count)
{
    int cmd     = 0;
    int data[3] = {0, 0, 0};
    
    sscanf(buf, "%d %d %d %d", &cmd, &data[0], &data[1], &data[2]);
    dev_info(dev, "%s: COMMAND: %d data1 = %d data2 = %d\n", __func__, cmd, data[0], data[1]);
    
    switch (cmd) {
    case FTM_CHARGING_SET_CHARGING_STATUS:
        if (data[2] == 1) {
            g_charging_on = data[0];
        } 
        g_smem_error = proc_comm_config_chg_current((g_charging_on ? true : false), data[1]);
        break;
    case FTM_CHARGING_BACKUPBATTERY_ADC:
        g_smem_error = proc_comm_config_coin_cell(2, &g_bb_voltage, data[0]);
        break;
    }
    
    return count;
}

static ssize_t bq275x0_battery_ftm_charging_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d\n", g_bb_voltage, g_smem_error);
}

static DEVICE_ATTR(ftm_battery, 0644, bq275x0_battery_ftm_battery_show, bq275x0_battery_ftm_battery_store);
static DEVICE_ATTR(ftm_charging, 0644, bq275x0_battery_ftm_charging_show, bq275x0_battery_ftm_charging_store);
#endif

static void bq275x0_battery_polling_timer_func(unsigned long unused)
{
    wake_lock(&di->bq275x0_wakelock);
    schedule_work(&di->bq275x0_update);
}

static ssize_t bq275x0_accuracy_store(struct device *dev, 
        struct device_attribute *attr, const char *buf, size_t count)
{
    char data[256];
    mm_segment_t oldfs; 
    struct file *fp = NULL;
    struct rtc_time tm;
    int cmd;
    int interval;
    
    sscanf(buf, "%d %d %s", &cmd, &interval, file_name);
    msmrtc_timeremote_read_time(NULL, &tm);
    
    if (cmd) {
        wake_lock(&di->bq275x0_wakelock);

        oldfs = get_fs(); 
        set_fs(KERNEL_DS);
        
        dev_info(&di->client->dev, "Open file: %s\n", data);
        fp = filp_open(file_name, O_RDWR | O_APPEND | O_CREAT, 0777);
        
        if (fp == NULL) {
            set_fs(oldfs);
            dev_err(&di->client->dev, "%s: Failed to open file\n", __func__);
            wake_unlock(&di->bq275x0_wakelock);
            test_accuracy = 0;
            return 0;
        }
    
        snprintf(data, sizeof(data), "\n%.2u/%.2u/%.4u %.2u:%.2u:%.2u (%.2u)\n",
            tm.tm_mon, tm.tm_mday, tm.tm_year, tm.tm_hour,
            tm.tm_min, tm.tm_sec, tm.tm_wday);
        fp->f_op->write(fp, data, strlen(data), &fp->f_pos);
           
        snprintf(data, sizeof(data), "~Elapsed(s)\tTemperature\tVoltage\tFlags\tNomAvailCap\tFullAvailCap\tRemCap\tFullChgCap\tAvgCurrent\tStateOfChg\tControlStatus\n");
        fp->f_op->write(fp, data, strlen(data), &fp->f_pos);

        filp_close(fp, NULL);
        set_fs(oldfs);
    
        test_accuracy = 1;
        di->polling_interval = interval;
        mod_timer(&di->polling_timer,
            jiffies + msecs_to_jiffies(di->polling_interval));
    } else {
        elapsed_time = 0;
        test_accuracy = 0;
        wake_unlock(&di->bq275x0_wakelock);
        di->polling_interval = BATTERY_POLLING_TIMER;
        mod_timer(&di->polling_timer,
            jiffies + msecs_to_jiffies(di->polling_interval));
    }
    
    return count;
}

static DEVICE_ATTR(accuracy, 0644, NULL,  bq275x0_accuracy_store);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bq275x0_late_resume(struct early_suspend *h)
{
    //wake_lock(&di->bq275x0_wakelock);
    //schedule_work(&di->bq275x0_update);
    di->is_suspend = false;
}
#endif

#ifdef CONFIG_PM
static int bq275x0_battery_suspend(struct device *dev)
{
    if (device_may_wakeup(dev)) {
        enable_irq_wake(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATGD));
        enable_irq_wake(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATLOW));
    }
    di->is_suspend = true;
    return 0;
}

static int bq275x0_battery_resume(struct device *dev)
{
    if (device_may_wakeup(dev)) {
        disable_irq_wake(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATGD));
        disable_irq_wake(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, di->pmic_BATLOW));
    }
    schedule_delayed_work(&di->bq275x0_update_soc,
                            msecs_to_jiffies(50));
    wake_lock(&di->bq275x0_wakelock);
    return 0;
}

static struct dev_pm_ops bq275x0_battery_pm = {
    .suspend = bq275x0_battery_suspend,
    .resume = bq275x0_battery_resume,
};
#endif

static void bq275x0_battery_check_dfi (struct work_struct *work)
{
    int bqfs_version = 0;
    char bqfs_project_name[7];
    char project_name[7];
    
#if defined(CONFIG_FIH_PROJECT_FB0)
    if (product_id == Product_FB3)
        strcpy(project_name, "FIHFB3");
    else if (product_id == Product_FD1)
        strcpy(project_name, "FIHFD1");
    else
        strcpy(project_name, "FIHFB0");
#elif defined(CONFIG_FIH_PROJECT_SF4Y6)
    strcpy(project_name, "FIHSF6");
#elif defined(CONFIG_FIH_PROJECT_SF4V5)
    strcpy(project_name, "FIHSF5");
#elif defined(CONFIG_FIH_PROJECT_SF8)
    strcpy(project_name, "FIHSF8");
#endif

    bq275x0_RomMode_get_bqfs_version(bqfs_project_name, &bqfs_version, NULL);

    /* 
     * If file version is newer than current version and project 
     * name is matched, initial bq275x0_RomMode driver.
     */
/* DIV5-KERNEL-VH-DFI-00[ */
#ifdef CONFIG_FIH_PROJECT_SF4Y6
    if ((bqfs_version > di->gauge_info.dfi_ver) && (di->gauge_info.dfi_ver >= 0x03)) {
#else
    if ((bqfs_version > di->gauge_info.dfi_ver)) {
#endif
/* DIV5-KERNEL-VH-DFI-00] */
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("BQFS[%d], Gauge[%d]\n", bqfs_version, di->gauge_info.dfi_ver);
        pmlog(KERN_INFO "BQFS[%s]\nGauge[%s]\nComp[%s]\n", bqfs_project_name, di->gauge_info.prj_name, project_name);        
#endif
        if (di->gauge_info.dfi_ver == 0x00) {
            if (!strncmp(bqfs_project_name, project_name, 6)) {
                dev_info(di->dev, "New dfi detects (1)\n");
                switch_set_state(&di->dfi_status, 1);
            } else {
                dev_info(di->dev, "No new dfi detects (1)\n");
                switch_set_state(&di->dfi_status, 0);
            }
        } else if (!strncmp(bqfs_project_name, di->gauge_info.prj_name, 6)) {
            dev_info(di->dev, "New dfi detects (2)\n");
            switch_set_state(&di->dfi_status, 1);
        }
    } else if ((product_id == Product_FB3 || product_id == Product_FD1) &&
                !strncmp("FIHFB0", di->gauge_info.prj_name, 6) &&
                !strncmp(bqfs_project_name, project_name, 6)) {
        dev_info(di->dev, "New dfi detects (3)\n");
        switch_set_state(&di->dfi_status, 1);    
    } else {
        dev_info(di->dev, "No new dfi detects (2)\n");
        switch_set_state(&di->dfi_status, 0);
    }
    
    return;
}

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

static int
bq275x0_cbverify_proc_read(char *page, char **start, off_t off,
              int count, int *eof, void *data)
{
    char buf[256];
    int i = 0;
    int len;
    
    dev_info(di->dev, "procfile_read (/proc/cbverify) called\n");
    
    if (test_on)
        snprintf(buf, 256, "TEST ON\n");
    else
        snprintf(buf, 256, "TEST OFF\n");
            
    for (i = 0; (i < BQ275X0_CBVERIFY_MAX); i++) {
        snprintf(buf, 256, "%s%02d ", buf, i);
        switch (i) {
        case BQ275X0_CBVERIFY_OTP_TEST:
            snprintf(buf, 256, "%sOTP", buf);
            break;
        case BQ275X0_CBVERIFY_CHARGING_TEST:
            snprintf(buf, 256, "%sCHG", buf);
            break;
        case BQ275X0_CBVERIFY_REM_OTP:
            snprintf(buf, 256, "%sREMOTP", buf);
            break;
        case BQ275X0_CBVERIFY_REM_DISCHARING:
            snprintf(buf, 256, "%sREMDIS", buf);
            break;
        case BQ275X0_CBVERIFY_LOW_BAT_TEST:
            snprintf(buf, 256, "%sLOWBAT", buf);
        }
        
        if (test_item[i]) {
            snprintf(buf, 256, "%s[T]", buf);
            if (i != BQ275X0_CBVERIFY_REM_OTP &&
                i != BQ275X0_CBVERIFY_REM_DISCHARING)
                snprintf(buf, 256, "%s%d\n", buf, test_value[i]);
            else
                snprintf(buf, 256, "%s\n", buf);
        } else
            snprintf(buf, 256, "%s[F]\n", buf);
    }

#if defined(CONFIG_FIH_POWER_LOG)
    pmlog("%s\n", buf);
#endif
    len = snprintf(page, PAGE_SIZE, "%s", buf);
        
    return proc_calc_metrics(page, start, off, count, eof, len);
}

static int
bq275x0_cbverify_proc_write(struct file *file, const char __user *buffer,
               unsigned long count, void *data)
{
    char cmd[256];
    int i = 0;
    int item = 0;
    int on = 0;
    int value = 0;
    
    dev_info(di->dev, "procfile_write (/proc/cbverify) called\n");
    
    if (copy_from_user(cmd, buffer, count)) {
        return -EFAULT;
    } else {
        sscanf(cmd, "%d %d %d",  &item, &on, &value);
        
        switch (item) {
        case BQ275X0_CBVERIFY_OTP_TEST:
        case BQ275X0_CBVERIFY_CHARGING_TEST:
        case BQ275X0_CBVERIFY_LOW_BAT_TEST:
            test_value[item] = value;
        case BQ275X0_CBVERIFY_REM_OTP:
        case BQ275X0_CBVERIFY_REM_DISCHARING:
            test_item[item] = on;
            break;
        case BQ275X0_CBVERIFY_TEST_ENABLE:
            test_on = true;
            if (test_item[BQ275X0_CBVERIFY_OTP_TEST])
                bq275x0_battery_BATGD(NULL);
            if (test_item[BQ275X0_CBVERIFY_LOW_BAT_TEST])
                bq275x0_battery_BATLOW(NULL);
            msm_batt_disable_discharging_monitor(test_item[BQ275X0_CBVERIFY_REM_DISCHARING]);
            break;
        case BQ275X0_CBVERIFY_TEST_DISABLE:
            test_on = false;
            if (test_item[BQ275X0_CBVERIFY_OTP_TEST])
                bq275x0_battery_BATGD(NULL);
            if (test_item[BQ275X0_CBVERIFY_LOW_BAT_TEST])
                bq275x0_battery_BATLOW(NULL);
            if (test_item[BQ275X0_CBVERIFY_REM_DISCHARING])
                msm_batt_disable_discharging_monitor(false);
            for (i = 0; i < BQ275X0_CBVERIFY_MAX; i++) {
                test_item[i] = false;
                test_value[i] = 0;
            }
        }

        return count;
    }
        
    return count;
}

static int bq275x0_battery_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    struct resource *ioarea;
    struct bq275x0_platform_data *bq275x0_pd = client->dev.platform_data;
    char project_name[7];
    char *name;
    int i;
    int num;
    int retval = 0;

    /* Get new ID for the new battery device */
    retval = idr_pre_get(&battery_id, GFP_KERNEL);
    if (retval == 0)
        return -ENOMEM;
    mutex_lock(&battery_mutex);
    retval = idr_get_new(&battery_id, client, &num);
    mutex_unlock(&battery_mutex);
    if (retval < 0)
        return retval;

    name = kasprintf(GFP_KERNEL, "battery");
    if (!name) {
        dev_err(&client->dev, "failed to allocate device name\n");
        retval = -ENOMEM;
        goto batt_failed_1;
    }

    di = kzalloc(sizeof(*di), GFP_KERNEL);
    if (!di) {
        dev_err(&client->dev, "failed to allocate device info data\n");
        retval = -ENOMEM;
        goto batt_failed_2;
    }
    product_id			= fih_get_product_id();
    di->id              = num;
    di->dev             = &client->dev;
    di->bat.name        = name;
    di->client          = client;
    di->pmic_BATGD      = bq275x0_pd->pmic_BATGD;
    di->pmic_BATLOW     = bq275x0_pd->pmic_BATLOW;
    di->dev_ready       = false;
    di->chg_inh         = false;
    di->is_suspend      = false;
    di->charging        = false;
    di->full            = false;
    di->rommode         = false;
    di->dbg_enable      = (fih_get_keypad_info() == MODE_ENABLE_KERNEL_LOG) ? true : false;
    di->health          = POWER_SUPPLY_HEALTH_GOOD;
    di->present         = 1;
    di->polling_interval= BATTERY_POLLING_TIMER;
    di->soc1            = false;
    di->socf            = false;
    di->temp_over400    = false;
    
    if (di->dbg_enable)
        dev_info(&client->dev, "[DBG MECHANISM ENABLE]\n");
    
    wake_lock_init(&di->bq275x0_wakelock, WAKE_LOCK_SUSPEND, "bq275x0");
    i2c_set_clientdata(client, di);
    device_init_wakeup(&client->dev, 1);
    
    bq275x0_powersupply_init(di);
    retval = power_supply_register(&client->dev, &di->bat);
    if (retval) {
        dev_err(&client->dev, "failed to register battery\n");
        goto batt_failed_3;
    }
    
    retval = bq275x0_battery_config_gpio(di);
    if (retval) {
        dev_err(&client->dev, "failed to config gauge GPIOs\n");
        goto batt_failed_4;        
    }
    
    retval = bq275x0_battery_irqsetup(di);
    if (retval) {
        dev_err(&client->dev, "failed to config gauge IRQs\n");
        goto batt_failed_5;
    }
    
    ioarea = request_mem_region(0x7BFFFFC, (0x7BFFFFF - 0x7BFFFFC) + 1,
            client->name);
    if (!ioarea) {
        dev_err(&client->dev, "[BQ275x0] region already claimed\n");
    }
    di->base = ioremap(0x7BFFFFC, (0x7BFFFFF - 0x7BFFFFC) + 1);
    if (!di->base) {
        dev_err(&client->dev, "[BQ275x0] ioremap failed\n");
    }
    
    INIT_DELAYED_WORK(&di->bq275x0_BATGD, bq275x0_battery_BATGD);
    INIT_DELAYED_WORK(&di->bq275x0_BATLOW, bq275x0_battery_BATLOW);
    INIT_DELAYED_WORK(&di->bq275x0_update_soc, bq275x0_battery_update_soc);
    INIT_WORK(&di->bq275x0_update, bq275x0_battery_update);
    
#ifdef CONFIG_HAS_EARLYSUSPEND  
    di->bq275x0_early_suspend.level = 51;
    di->bq275x0_early_suspend.suspend = NULL;
    di->bq275x0_early_suspend.resume = bq275x0_late_resume;
    register_early_suspend(&di->bq275x0_early_suspend);
#endif

    INIT_DELAYED_WORK(&di->check_dfi, bq275x0_battery_check_dfi);
    di->dfi_status.name = "dfi_status";
    if (switch_dev_register(&di->dfi_status)) {
        dev_err(&client->dev, "%s: dfi_status switch device failed!!", __func__);
    } else {
        switch_set_state(&di->dfi_status, 0);
        schedule_delayed_work(&di->check_dfi,
                            msecs_to_jiffies(30 * 1000));
    }

    di->dev_ready = true;
    
    /* Enable Polling Timer */
    setup_timer(&di->polling_timer,
                bq275x0_battery_polling_timer_func, 0);
    mod_timer(&di->polling_timer,
                jiffies + msecs_to_jiffies(0));    

    /*if (bq275x0_battery_unseal()) {
        dev_info(&client->dev, "Unsealed failed\n");    
    }*/
    if (bq275x0_battery_snooze_mode(true)) {
        dev_info(&client->dev, "Set SNOOZE mode failed\n");
    }
    
    bq275x0_battery_charge_inhibit_subclass();
    bq275x0_battery_check_IT_enable();
    //bq275x0_battery_SOC1();
    bq275x0_battery_manufacturer_info((u8*)&di->gauge_info, 0);
    bq275x0_endian_adjust(di->gauge_info.date, 4);
    strncpy(project_name, di->gauge_info.prj_name, 6);
    project_name[6] = '\0';
    dev_info(&client->dev, "device type[%x], fw ver.[%x], dfi[%02x-%s-%08x]\n",
                            bq275x0_battery_device_type(),
                            bq275x0_battery_fw_version(),
                            di->gauge_info.dfi_ver,
                            project_name,
                            *((u32*)di->gauge_info.date)
                            );

    dev_info(&client->dev, "%s finished\n", __func__);
    
#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
    retval = device_create_file(&client->dev, &dev_attr_ftm_battery);
    if (retval) {
        dev_err(&client->dev,
               "%s: dev_attr_ftm_battery failed\n", __func__);
    }
    retval = device_create_file(&client->dev, &dev_attr_ftm_charging);
    if (retval) {
        dev_err(&client->dev,
               "%s: dev_attr_ftm_charging failed\n", __func__);
    }
#endif
    
    retval = device_create_file(&client->dev, &dev_attr_accuracy);
    if (retval) {
        dev_err(&client->dev,
               "%s: dev_attr_accuracy failed\n", __func__);
    }
    
    test_on = false;
    for (i = 0; i < BQ275X0_CBVERIFY_MAX; i++) {
        test_item[i] = false;
        test_value[i] = 0;
    }
    cbverify_proc_entry = create_proc_entry("cbverify", 0666, NULL);
    cbverify_proc_entry->read_proc   = bq275x0_cbverify_proc_read;
    cbverify_proc_entry->write_proc  = bq275x0_cbverify_proc_write;

    return 0;

batt_failed_5:
    bq275x0_battery_free_irq(di);
    bq275x0_battery_release_gpio(di);
batt_failed_4:
    power_supply_unregister(&di->bat);
batt_failed_3:
    kfree(di);
batt_failed_2:
    kfree(name);
batt_failed_1:
    mutex_lock(&battery_mutex);
    idr_remove(&battery_id, num);
    mutex_unlock(&battery_mutex);

    return retval;
}

static int bq275x0_battery_remove(struct i2c_client *client)
{
    struct bq275x0_device_info *di = i2c_get_clientdata(client);
    
    if (bq275x0_battery_snooze_mode(false)) {
        dev_info(&client->dev, "Clear SNOOZE mode failed\n");
    } else {
        dev_info(&client->dev, "Clear SNOOZE mode successfully\n");    
    }
    
    di->dev_ready = false;
    
    switch_dev_unregister(&di->dfi_status);

#ifdef CONFIG_HAS_EARLYSUSPEND  
    unregister_early_suspend(&di->bq275x0_early_suspend);
#endif
    
    bq275x0_battery_free_irq(di);
    bq275x0_battery_release_gpio(di);

    //flush_work(&di->bq275x0_BATGD);
    //flush_work(&di->bq275x0_BATLOW);
    flush_work(&di->bq275x0_update);

    power_supply_unregister(&di->bat);

    kfree(di->bat.name);

    mutex_lock(&battery_mutex);
    idr_remove(&battery_id, di->id);
    mutex_unlock(&battery_mutex);
    
    wake_lock_destroy(&di->bq275x0_wakelock);

    kfree(di);

    return 0;
}

/*
 * Module stuff
 */
static const struct i2c_device_id bq275x0_id[] = {
    { "bq275x0-battery", 0 },
    {},
};

static struct i2c_driver bq275x0_battery_driver = {
    .driver = {
        .name   = "bq275x0-battery",
#ifdef CONFIG_PM
        .pm     = &bq275x0_battery_pm,
#endif
    },
    .probe = bq275x0_battery_probe,
    .remove = bq275x0_battery_remove,
    .id_table = bq275x0_id,
};

static int __init bq275x0_battery_init(void)
{
    int ret;

    ret = i2c_add_driver(&bq275x0_battery_driver);
    if (ret)
        printk(KERN_ERR "Unable to register bq275x0 driver\n");

    return ret;
}
module_init(bq275x0_battery_init);

static void __exit bq275x0_battery_exit(void)
{
    i2c_del_driver(&bq275x0_battery_driver);
}
module_exit(bq275x0_battery_exit);

MODULE_AUTHOR("Audi PC Huang <AudiPCHuang@fihtdc.com>");
MODULE_DESCRIPTION("bq275x0 battery monitor driver");
MODULE_LICENSE("GPL");
