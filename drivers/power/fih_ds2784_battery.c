/*
 *     ds2784.c - DS2784 1-Cell Stand-Alone Fuel Gauge IC with 
 *                Li+ Protector & SHA-1 Authentication
 *
 *     Copyright (C) 2010 Audi PC Huang <audipchuang@fihtdc.com>
 *     Copyright (C) 2010 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <mach/ds2784.h>
#include <linux/wakelock.h>
#include "../../arch/arm/mach-msm/smd_private.h"

#if defined(CONFIG_FIH_POWER_LOG)
#include "linux/pmlog.h"
#endif

//Div6D1-JL-USB_PORTING-01 +{
#include <mach/msm_hsusb.h>  
int batt_val;
//Div6D1-JL-USB_PORTING-01 +}

#define ds2784_name             "ds2784_battery"
#define BATTERY_POLLING_TIMER   60000
#define BATTERY_POLLING_TIMER2  15000

struct ds2784_battery_info {
    struct device *ds2784_pdev;
    struct power_supply ds2784_bat;
    struct delayed_work batt_update_work;
    struct timer_list polling_timer;
    
    int     voltage;
    int     curr;
    int     capacity;
    int     temperature;
    int     health;
    int     present;
    
    int     polling_interval;
    
    bool    OT_flag;
    bool    dbg_enable;
    bool    charging;
    bool    full;
    
    struct wake_lock ds2784_wakelock;
};

static struct ds2784_battery_info ds2784_batt_info = {
    .voltage        = 0,
    .curr           = 0,
    .capacity       = 0,
    .temperature    = 0,
    .present        = 1,
    .health         = POWER_SUPPLY_HEALTH_UNKNOWN,
    .polling_interval= BATTERY_POLLING_TIMER2,
    .OT_flag        = false,
    .charging       = false,
    .full           = false,
};

static int test_temperature = 500;
static int test_temp_on = 0;

extern int ds2482_one_wire_reset(void);
extern int ds2482_one_wire_write_byte(u8 data);
extern int ds2482_one_wire_read_byte(u8* data);
extern int ds2482_turn_on(void);
extern void ds2482_turn_off(void);
extern struct mutex* ds2482_get_suspend_lock (void);
extern int proc_comm_config_coin_cell(int vset, int *voltage, int status);
extern int proc_comm_config_chg_current(bool on, int curr);
extern u32 msm_batt_get_batt_status(void);
extern void msm_batt_notify_over_temperature(bool over_temperature);
extern void msm_batt_notify_battery_full(bool full);
extern void msm_batt_notify_charging_state(bool charging);

static int ds2482_read16(u8 reg, u16* Data)
{
    int rc, iStep;
    union {
        u16 s;
        struct {
            u8 low;
            u8 high;
        } u;
    } data;

    if ((rc = ds2482_one_wire_reset()) < 0) {
        iStep = 0; 
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_SKIP_NET_ADDR)) < 0) {
        iStep = 1;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_READ_DATA)) < 0) {
        iStep = 2;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(reg)) < 0) {
        iStep = 3;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_read_byte(&(data.u.high))) < 0) {
        iStep = 4;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_read_byte(&(data.u.low))) < 0) {
        iStep = 5;
        goto failexit;
    }

    //dev_info(ds2784_batt_info.ds2784_pdev, "%s: high(%d) low(%d) s(%d)\n", __func__, data.u.high, data.u.low, data.s);
    *Data = data.s;
    
    return rc;

failexit:
    dev_err(ds2784_batt_info.ds2784_pdev, "%s failed in Step %d!\n", __func__, iStep);

    return rc;
}

static int ds2482_read8(u8 reg, u8* Data)
{
    int rc, iStep;

    if ((rc = ds2482_one_wire_reset()) < 0) {
        iStep = 0; 
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_SKIP_NET_ADDR)) < 0) {
        iStep = 1;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_READ_DATA)) < 0) {
        iStep = 2;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(reg)) < 0) {
        iStep = 3;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_read_byte(Data)) < 0) {
        iStep = 4;
        goto failexit;
    }

    //dev_info(ds2784_batt_info.ds2784_pdev, "%s: Data(%d) \n", __func__, *Data);
    
    return rc;

failexit:
    dev_err(ds2784_batt_info.ds2784_pdev, "%s failed in Step %d!\n", __func__, iStep);
    
    return rc;
}

static int ds2482_read_family_code(u8* Data)
{
    int rc, iStep;

    if ((rc = ds2482_one_wire_reset()) < 0) {
        iStep = 0; 
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_READ_NET_ADDR)) < 0) {
        iStep = 1;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_read_byte(Data)) < 0) {
        iStep = 3;
        goto failexit;
    }

    //dev_info(ds2784_batt_info.ds2784_pdev, "%s: Data(%d) \n", __func__, *Data);
    
    return rc;

failexit:
    dev_err(ds2784_batt_info.ds2784_pdev, "%s failed in Step %d!\n", __func__, iStep);
    
    return rc;
}

/*static int ds2482_write16(u8 reg, u16* Data)
{
    int rc, iStep;
    union {
        short s;
        struct {
            u8 low;
            u8 high;
        } u;
    } data;

    data.s = *Data;
    if ((rc = ds2482_one_wire_reset()) < 0) {
        iStep = 0; 
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_SKIP_NET_ADDR)) < 0) {
        iStep = 1;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_WRITE_DATA)) < 0) {
        iStep = 2;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(reg)) < 0) {
        iStep = 3;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(data.u.high)) < 0) {
        iStep = 4;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(data.u.low)) < 0) {
        iStep = 5;
        goto failexit;
    }
    
    return rc;

failexit:
    dev_err(ds2784_pdev, "%s failed in Step %d!\n", __func__, iStep);  

    return rc;
}

static int ds2482_write8(u8 reg, u8* Data)
{
    int rc, iStep;

    if ((rc = ds2482_one_wire_reset()) < 0) {
        iStep = 0; 
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_SKIP_NET_ADDR)) < 0) {
        iStep = 1;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(DS2784CMD_WRITE_DATA)) < 0) {
        iStep = 2;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(reg)) < 0) {
        iStep = 3;
        goto failexit;
    }
    if ((rc = ds2482_one_wire_write_byte(*Data)) < 0) {
        iStep = 4;
        goto failexit;
    }
    
    return rc;

failexit:
    dev_err(ds2784_pdev, "%s failed in Step %d!\n", __func__, iStep);
    
    return rc;
}*/

static int ds2784_get_battery_info(enum _battery_info_type_ info, u8* data)
{
    int rc = 0;

    if (ds2482_turn_on() == 0) {
        switch (info) {
        case BATT_CAPACITY_INFO:
            /* Units: 1% */
            ds2482_read8(DS2784_RARC_REG, data);
            break;
        case BATT_VOLTAGE_INFO:
            /* Units: 4.886mV and the 5 lowest bits are not used. */
            ds2482_read16(DS2784_VOL_REG_H, (u16*)data);
            break;
        case BATT_TEMPERATURE_INFO:
            /* Units: 0.125 C and the 5 lowest bits are not used. */
            ds2482_read16(DS2784_TEMP_REG_H, (u16*)data);
            break;
        case BATT_CURRENT_INFO:
            /* Units: 1.5625uV/Rsns */
            ds2482_read16(DS2784_CURR_REG_H, (u16*)data);
            break;
        case BATT_AVCURRENT_INFO:
            /* Units: 1.5625uV/Rsns */
            ds2482_read16(DS2784_AVG_CURR_REG_H, (u16*)data);
            break;
        case BATT_STATUS_INFO:
            ds2482_read8(DS2784_STATUS_REG, data);
            break;
        case BATT_ACCCURRENT_INFO:
            /* Units: 6.25uVh/Rsns */
            ds2482_read16(DS2784_ACCUMULATED_CURRENT_REG_H, (u16*)data);
            break;
        case BATT_AGE_SCALAR_INFO:
            /* Units: 0.78% */
            ds2482_read8(DS2784_AGE_SCALAR_REG, data);
            break;
        case BATT_FULL_INFO:
            /* Units: 61ppm/2 */
            ds2482_read16(DS2784_FULL_REG_H, (u16*)data);
            break;
        case BATT_FULL40_INFO:
            /* Units: 6.25uV */
            ds2482_read16(DS2784_FULL_40_REG_H, (u16*)data);
            break;
        case BATT_FAMILY_CODE:
            ds2482_read_family_code(data);
            break;
        case BATT_GET_RSNS:
            ds2482_read8(DS2784_SENSE_RESISTOR_PRIME_REG, data);
            break;
        case BATT_ACTIVE_EMPTY_INFO:
            ds2482_read16(DS2784_ACTIVE_EMPTY_REG_H, (u16*)data);
            break;
        case BATT_STANDBY_EMPTY_INFO:
            ds2482_read16(DS2784_STANDBY_EMPTY_REG_H, (u16*)data);
            break;
        case BATT_ACTIVE_EMPTY_VOLTAGE_INFO:
            ds2482_read8(DS2784_ACTIVE_EMPTY_VOL_REG, data);
            break;
        default:
            rc = -1;
        }
    } else {
        rc = -2;
    }

    ds2482_turn_off();
    return rc;
}

static int ds2784_unit_converter(enum _battery_info_type_ info, u16 data)
{
    u8 Rsnsp = 0;
    
    switch (info) {
    case BATT_VOLTAGE_INFO:
        /* Units: 4.886mV and the 5 lowest bits are not used. */
        if (data & 0x8000)
            return ((data & 0x7FFF) >> 5) * 4886 / 1000 * (-1);
        else
            return ((data & 0x7FFF) >> 5) * 4886 / 1000;
    case BATT_TEMPERATURE_INFO:
        /* Units: 0.125 C and the 5 lowest bits are not used. */
        if (data & 0x8000)
            return ((data & 0x7FFF) >> 5) * 125 / 1000 * (-1);
        else
            return ((data & 0x7FFF) >> 5) * 125 / 1000;
    case BATT_CURRENT_INFO:
    case BATT_AVCURRENT_INFO:
        /* Units: 1.5625uV/Rsns */
        ds2784_get_battery_info(BATT_GET_RSNS, &Rsnsp);
        
        if (data & 0x8000)
            return (data & 0x7FFF) * 15625 / 10000 * Rsnsp / 1000 * (-1);
        else
            return (data & 0x7FFF) * 15625 / 10000 * Rsnsp / 1000;
    case BATT_ACCCURRENT_INFO:
        /* Units: 6.25uVh/Rsns */
        ds2784_get_battery_info(BATT_GET_RSNS, &Rsnsp);

        return (data & 0x7FFF) * 625 / 100 * Rsnsp / 1000;
    case BATT_AGE_SCALAR_INFO:
        /* Units: 0.78% */
        if (data & 0x8000)
            return 100;
        else
            return (data * 78 / 100);
    case BATT_FULL_INFO:
        /* Units: 61ppm/2 */
        return data * 61 / 2;
    case BATT_FULL40_INFO:
        /* Units: 6.25uV */
        return data * 625 / 100;
    case BATT_ACTIVE_EMPTY_VOLTAGE_INFO:
        /* Units: 19.5mv */
        return data * 195 / 10;
    default:
        dev_err(ds2784_batt_info.ds2784_pdev, "%s: Incorrect !\n", __func__);
        return -1;
    }
}

/*static int ds2784_set_battery_info(enum _battery_info_type_ info, u8* data)
{
    int rc = 0;

    if (ds2482_turn_on() == 0) {
        switch (info) {
        case BATT_ACCCURRENT_INFO:
            ds2482_write16(DS2784_ACCUMULATED_CURRENT_REG_H, (u16*)data);
            break;
        case BATT_AGE_SCALAR_INFO:
            ds2482_write8(DS2784_AGE_SCALAR_REG, data);
            break;
        default:
            rc = -1;
        }
        
        ds2482_turn_off();
        return rc;
    }
    
    return -2;
}*/

int ds2784_batt_get_batt_voltage(void)
{
    int rc = -1;
    u16 data    = 0;
    u8 *pdata   = (u8*)&data;
    
    rc = ds2784_get_battery_info(BATT_VOLTAGE_INFO, pdata);
    if (rc < 0)
        return 0xFFFFFFFF; /* Easy to know the gauge has problems */
    else
        return ds2784_unit_converter(BATT_VOLTAGE_INFO, data) * 1000;
}
EXPORT_SYMBOL(ds2784_batt_get_batt_voltage);

int ds2784_batt_get_batt_capacity(void)
{
    int rc  = 0;
    u8 data = 0;

    rc = ds2784_get_battery_info(BATT_CAPACITY_INFO, &data);
    if (rc < 0)
        printk(KERN_ERR "%s: FAILED!!\n", __func__);
            
    //for dummy battery
    if ((ds2784_batt_info.voltage > 3600 * 1000) && (data == 0))
        data = 99;

    if (data < 15)
        ds2784_batt_info.polling_interval = BATTERY_POLLING_TIMER2;
    else if (!ds2784_batt_info.OT_flag)
        ds2784_batt_info.polling_interval = BATTERY_POLLING_TIMER;

    return (int)data;
}
EXPORT_SYMBOL(ds2784_batt_get_batt_capacity);

int ds2784_batt_get_batt_temp(void)
{
    int rc      = -1;
    u16 data    = 0;
    u8 *pdata   = (u8*)&data;

    rc = ds2784_get_battery_info(BATT_TEMPERATURE_INFO, pdata);
    if (rc < 0)
        return 0xFFFFFFFF;
    else {
        if (test_temp_on)
            return test_temperature;
        else
            return ds2784_unit_converter(BATT_TEMPERATURE_INFO,
                                                data) * 10;
    }
}
EXPORT_SYMBOL(ds2784_batt_get_batt_temp);

int ds2784_batt_get_batt_current(void)
{
    int rc      = -1;
    u16 data    = 0;
    u8 *pdata   = (u8*)&data;

    rc = ds2784_get_battery_info(BATT_CURRENT_INFO, pdata);
    if (rc < 0)
        return 0xFFFFFFFF;
    else
        return ds2784_unit_converter(BATT_CURRENT_INFO,
                                                data);
}
EXPORT_SYMBOL(ds2784_batt_get_batt_current);

int ds2784_batt_set_batt_health(void)
{
    int rc          = -1;
    u8 family_code  = 0xFF;
    
    ds2784_batt_info.present = 1;
    
    rc = ds2784_get_battery_info(BATT_FAMILY_CODE, &family_code);
    if (rc < 0) {
        ds2784_batt_info.present = 0;
        return POWER_SUPPLY_HEALTH_UNKNOWN;
    }
        
    if (0xFF == family_code) {
        ds2784_batt_info.present = 0;
        return POWER_SUPPLY_HEALTH_UNKNOWN;
    }
        
    if ((BATTERY_OVER_TEMP_HIGH < ds2784_batt_info.temperature) || 
        (BATTERY_OVER_TEMP_LOW > ds2784_batt_info.temperature)) {
        if (!ds2784_batt_info.OT_flag) { /* false -> true */
            ds2784_batt_info.OT_flag = true;
            msm_batt_notify_over_temperature(true); 
        }
        ds2784_batt_info.polling_interval = BATTERY_POLLING_TIMER2;
    } else {
        if (ds2784_batt_info.OT_flag) {/* true -> false */
            ds2784_batt_info.OT_flag = false;
            msm_batt_notify_over_temperature(false);
        }
        if (!(ds2784_batt_info.capacity < 15))
            ds2784_batt_info.polling_interval = BATTERY_POLLING_TIMER;
    }
    
    if (BATTERY_OVER_TEMP_HIGH < ds2784_batt_info.temperature)
        return POWER_SUPPLY_HEALTH_OVERHEAT;
    else if (BATTERY_OVER_TEMP_LOW > ds2784_batt_info.temperature)
        return POWER_SUPPLY_HEALTH_COLD;
        
    return POWER_SUPPLY_HEALTH_GOOD;
}
EXPORT_SYMBOL(ds2784_batt_set_batt_health);

void ds2784_update_batt_status(void)
{
    static int counter = 0;

    if ((ds2784_batt_info.polling_interval == BATTERY_POLLING_TIMER2) &&
        (counter == 4)) {
            ds2784_batt_info.voltage    = ds2784_batt_get_batt_voltage();
            ds2784_batt_info.curr       = ds2784_batt_get_batt_current();
    } else {
        ds2784_batt_info.voltage    = ds2784_batt_get_batt_voltage();
        ds2784_batt_info.curr       = ds2784_batt_get_batt_current();    
    }
    if (ds2784_batt_info.curr < 0)
        msm_batt_notify_charging_state(false);
    else
        msm_batt_notify_charging_state(true);
    
    ds2784_batt_info.capacity = ds2784_batt_get_batt_capacity();
    if (ds2784_batt_info.capacity == 100) {
        if (!ds2784_batt_info.full) {
            ds2784_batt_info.full = true;
            msm_batt_notify_battery_full(true);
        }
    } else {
        if (ds2784_batt_info.full) {
            ds2784_batt_info.full = false;
            msm_batt_notify_battery_full(false);
        }
    }
    
    ds2784_batt_info.temperature= ds2784_batt_get_batt_temp();
    ds2784_batt_info.health     = ds2784_batt_set_batt_health();
    
    dev_info(ds2784_batt_info.ds2784_pdev, "voltage = %d, soc = %d, current = %d, temperature = %d\n",
            ds2784_batt_info.voltage,
            ds2784_batt_info.capacity,
            ds2784_batt_info.curr,
            ds2784_batt_info.temperature
            );
    
    if (ds2784_batt_info.polling_interval == BATTERY_POLLING_TIMER2) {
        if (counter == 4)
            counter = 0;
        else
            counter++;
    }
}

static int ds2784_remove(struct platform_device *pdev)
{
    power_supply_unregister(&ds2784_batt_info.ds2784_bat);
    wake_lock_destroy(&ds2784_batt_info.ds2784_wakelock);
    return 0;
}

static int ds2784_suspend(struct platform_device *pdev, pm_message_t state)
{
    mutex_lock(ds2482_get_suspend_lock());
    return 0;
}

static int ds2784_resume(struct platform_device *pdev)
{
    mutex_unlock(ds2482_get_suspend_lock());
    wake_lock(&ds2784_batt_info.ds2784_wakelock);
    schedule_delayed_work(&ds2784_batt_info.batt_update_work,
                            msecs_to_jiffies(50));
    return 0;
}

#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
enum {
    FTM_CHARGING_SET_CHARGING_STATUS,
    FTM_CHARGING_BACKUPBATTERY_ADC,  
};

static int g_capacity   = 0;
static int g_voltage    = 0;
static int g_temp       = 0;
static int g_curr       = 0;
static int g_avcurr     = 0;
static int g_acccurr    = 0;
static int g_family_code= 0;
static enum _battery_info_type_ g_current_cmd = 0;
static int g_smem_error = -1;
static int g_bb_voltage = -1;
static int g_charging_on= 0;

static ssize_t ds2784_ftm_battery_store(struct device *dev, 
        struct device_attribute *attr, const char *buf, size_t count)
{
    int cmd     = 0;
    u16 data    = 0;
    u8 *pdata   = (u8*)&data;
    
    sscanf(buf, "%d", &cmd);
    dev_info(dev, "%s: COMMAND: %d\n", __func__, cmd);

    g_current_cmd = cmd;
    switch (cmd) {
    case BATT_CAPACITY_INFO:         // 0x06 RARC - Remaining Active Relative Capacity
        ds2784_get_battery_info(cmd, pdata);
        g_capacity = pdata[0];
        break;
    case BATT_VOLTAGE_INFO:         // 0x0c Voltage MSB
        ds2784_get_battery_info(cmd, pdata);
        g_voltage = ds2784_unit_converter(cmd, data);
        break;
    case BATT_TEMPERATURE_INFO:      // 0x0a Temperature MSB
        ds2784_get_battery_info(cmd, pdata);
        g_temp = ds2784_unit_converter(cmd, data);
        break;
    case BATT_CURRENT_INFO:          // 0x0e Current MSB
        ds2784_get_battery_info(cmd, pdata);
        g_curr = ds2784_unit_converter(cmd, data);
        break;
    case BATT_AVCURRENT_INFO:        // 0x08 Average Current MSB
        ds2784_get_battery_info(cmd, pdata);
        g_avcurr = ds2784_unit_converter(cmd, data);
        break;
    case BATT_STATUS_INFO:           // 0x01 Status
        break;
    case BATT_ACCCURRENT_INFO:       // 0x10 Accumulated Current Register MSB
        ds2784_get_battery_info(cmd, pdata);
        g_acccurr = ds2784_unit_converter(cmd, data);
        break;
    case BATT_AGE_SCALAR_INFO:       // 0x14 Age Scalar
    case BATT_FULL_INFO:             // 0x16 Full Capacity MSB
    case BATT_FULL40_INFO:           // 0x6A Full40 MSB
    case BATT_GET_RSNS:
        break;
    case BATT_FAMILY_CODE:           // 0x33
        ds2784_get_battery_info(cmd, pdata);
        g_family_code = pdata[0];
        break;
    };

    return count;
}

static ssize_t ds2784_ftm_battery_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    switch (g_current_cmd) {
    case BATT_CAPACITY_INFO:         // 0x06 RARC - Remaining Active Relative Capacity
        return sprintf(buf, "%d\n", g_capacity);
    case BATT_VOLTAGE_INFO:         // 0x0c Voltage MSB
        return sprintf(buf, "%d\n", g_voltage);
    case BATT_TEMPERATURE_INFO:      // 0x0a Temperature MSB
        return sprintf(buf, "%d\n", g_temp);
    case BATT_CURRENT_INFO:          // 0x0e Current MSB
        return sprintf(buf, "%d\n", g_curr);
    case BATT_AVCURRENT_INFO:        // 0x08 Average Current MSB
        return sprintf(buf, "%d\n", g_avcurr);
    case BATT_STATUS_INFO:           // 0x01 Status
        return 0;
    case BATT_ACCCURRENT_INFO:       // 0x10 Accumulated Current Register MSB
        return sprintf(buf, "%d\n", g_acccurr);
    case BATT_AGE_SCALAR_INFO:       // 0x14 Age Scalar
    case BATT_FULL_INFO:             // 0x16 Full Capacity MSB
    case BATT_FULL40_INFO:           // 0x6A Full40 MSB
    case BATT_GET_RSNS:
        return 0;
    case BATT_FAMILY_CODE:           // 0x33
        return sprintf(buf, "%d\n", g_family_code);
    default:
        return 0;
    };
    
    return 0;
}

static ssize_t ds2784_ftm_charging_store(struct device *dev, 
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

static ssize_t ds2784_ftm_charging_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d\n", g_bb_voltage, g_smem_error);
}

static DEVICE_ATTR(ftm_battery, 0644, ds2784_ftm_battery_show, ds2784_ftm_battery_store);
static DEVICE_ATTR(ftm_charging, 0644, ds2784_ftm_charging_show, ds2784_ftm_charging_store);
#endif

static ssize_t ds2784_test_store(struct device *dev, 
        struct device_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d %d",  &test_temp_on, &test_temperature);
    if (test_temp_on)
        dev_info(dev, "%s: test_temperature = %d\n", __func__, test_temperature);
    return count;
}

static DEVICE_ATTR(test, 0644, NULL, ds2784_test_store);

static enum power_supply_property ds2784_battery_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_CAPACITY,
    POWER_SUPPLY_PROP_TEMP,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_TECHNOLOGY,
};

static int ds2784_battery_get_property(struct power_supply *psy,
                    enum power_supply_property psp,
                    union power_supply_propval *val)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        val->intval =  msm_batt_get_batt_status();
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = ds2784_batt_info.voltage;
        break;
    case POWER_SUPPLY_PROP_CURRENT_NOW:
        val->intval = ds2784_batt_info.curr;
        break;
    case POWER_SUPPLY_PROP_CAPACITY:
        if (ds2784_batt_info.dbg_enable) {
            if ((ds2784_batt_info.capacity == 255) ||
                (ds2784_batt_info.capacity == 0 && !ds2784_batt_info.present)) {
                if (ds2784_batt_info.voltage > 3600 * 1000)
                    val->intval = 77;
				else if (ds2784_batt_info.voltage < -4990)
					val->intval = 44;
                else
                    val->intval = 1;
            } else
                val->intval = ds2784_batt_info.capacity;
        } else
            val->intval = ds2784_batt_info.capacity;
        batt_val = val->intval;    //Div6D1-JL-USB_PORTING-01+
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("batt : %d\%_%dmV\n", ds2784_batt_info.capacity, ds2784_batt_info.voltage/1000);
#endif
        break;
    case POWER_SUPPLY_PROP_TEMP:
        val->intval = ds2784_batt_info.temperature;
        break;
    case POWER_SUPPLY_PROP_HEALTH:
        val->intval = ds2784_batt_info.health;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        if (ds2784_batt_info.dbg_enable)
            val->intval = 1;
        else
            val->intval = ds2784_batt_info.present;
        break;
    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static void ds2784_powersupply_init(void)
{
    ds2784_batt_info.ds2784_bat.name = "battery";
    ds2784_batt_info.ds2784_bat.type = POWER_SUPPLY_TYPE_BATTERY;
    ds2784_batt_info.ds2784_bat.properties = ds2784_battery_props;
    ds2784_batt_info.ds2784_bat.num_properties = ARRAY_SIZE(ds2784_battery_props);
    ds2784_batt_info.ds2784_bat.get_property = ds2784_battery_get_property;
    ds2784_batt_info.ds2784_bat.external_power_changed = NULL;
}

static void ds2784_batt_update(struct work_struct *work)
{
    ds2784_update_batt_status();
    mod_timer(&ds2784_batt_info.polling_timer,
            jiffies + msecs_to_jiffies(ds2784_batt_info.polling_interval));
    power_supply_changed(&ds2784_batt_info.ds2784_bat);
    wake_unlock(&ds2784_batt_info.ds2784_wakelock);
    wake_lock_timeout(&ds2784_batt_info.ds2784_wakelock, 10 * HZ);
}

static void ds2784_polling_timer_func(unsigned long unused)
{
    wake_lock(&ds2784_batt_info.ds2784_wakelock);
    schedule_delayed_work(&ds2784_batt_info.batt_update_work,
                            msecs_to_jiffies(1));
}

static int ds2784_probe(struct platform_device *pdev)
{
    int rc = 0;
    
    ds2784_batt_info.ds2784_pdev = &pdev->dev;
    
    ds2784_update_batt_status();
    
    ds2784_powersupply_init();
    rc = power_supply_register(&pdev->dev, &ds2784_batt_info.ds2784_bat);
    if (rc) {
        dev_err(&pdev->dev, "failed to register battery\n");
        return rc;
    }
    
    ds2784_batt_info.dbg_enable = (fih_get_keypad_info() == MODE_ENABLE_KERNEL_LOG) ? true : false;
    if (ds2784_batt_info.dbg_enable)
		dev_info(&pdev->dev, "[DBG MECHANISM ENABLE]\n");
	
    INIT_DELAYED_WORK(&ds2784_batt_info.batt_update_work, ds2784_batt_update);
    setup_timer(&ds2784_batt_info.polling_timer,
                ds2784_polling_timer_func, 0);
    mod_timer(&ds2784_batt_info.polling_timer,
                jiffies + msecs_to_jiffies(ds2784_batt_info.polling_interval));
    
#ifdef CONFIG_FIH_FTM_BATTERY_CHARGING
    rc = device_create_file(&pdev->dev, &dev_attr_ftm_battery);
    if (rc) {
        dev_err(&pdev->dev,
               "%s: dev_attr_ftm_battery failed\n", __func__);
    }
    rc = device_create_file(&pdev->dev, &dev_attr_ftm_charging);
    if (rc) {
        dev_err(&pdev->dev,
               "%s: dev_attr_ftm_charging failed\n", __func__);
    }
#endif

    rc = device_create_file(&pdev->dev, &dev_attr_test);
    if (rc) {
        dev_err(&pdev->dev,
               "%s: dev_attr_test failed\n", __func__);
    }

    wake_lock_init(&ds2784_batt_info.ds2784_wakelock, WAKE_LOCK_SUSPEND, "ds2784");
    
    return rc;
}

static struct platform_driver ds2784_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = ds2784_name,
    },
    .probe    = ds2784_probe,
    .remove   = ds2784_remove,
    .suspend  = ds2784_suspend,
    .resume   = ds2784_resume,
};

static int __init ds2784_init(void)
{
    return platform_driver_register(&ds2784_driver);
}

static void __exit ds2784_exit(void)
{
    platform_driver_unregister(&ds2784_driver);
}

module_init(ds2784_init);
module_exit(ds2784_exit);
MODULE_VERSION("1.0");
MODULE_AUTHOR( "Audi PC Huang <audipchuang@fihtdc.com>" );
MODULE_DESCRIPTION("DS2784 Gas Gauge Driver");
MODULE_LICENSE("GPL");
