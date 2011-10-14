/*
 * Copyright (c) 2010 Yamaha Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <mach/vreg.h>
#include <mach/gpio.h>
#include <mach/msm_smd.h>

/* for debugging */
#define DEBUG 0

#define SENSOR_TYPE (5)
#define PS_SENSOR_TYPE (1)
#define ALS_SENSOR_TYPE (2)
#define SENSOR_NAME "cm3623"
#define ALS_NAME "light"
#define PS_NAME "proximity"

#define SENSOR_DEFAULT_DELAY             200    /* 200 ms */
#define SENSOR_MAX_DELAY                (3000)  /* 3000 ms */ 
#define ABS_STATUS                      (ABS_BRAKE)
#define ABS_WAKE                        (ABS_MISC)
#define ABS_CONTROL_REPORT              (ABS_THROTTLE)
#define I2C_RETRY_TIME 5

#ifdef CONFIG_SENSORS_CM3623_IS_AD
#define ALS_CMD_ADDR	0x48
#define ALS_DATA_ADDR	0x49
#define PS_CMD1_ADDR	0x78
#define PS_CMD2_ADDR	0x79
#else
#define ALS_CMD_ADDR	0x10
#define ALS_DATA_ADDR	0x11
#define PS_CMD1_ADDR	0x58
#define PS_CMD2_ADDR	0x59
#endif

//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
enum
{
	ALS_CMD_I2C_ADDR = 0,
	ALS_DATA_I2C_ADDR,
	PS_CMD1_I2C_ADDR,
	PS_CMD2_I2C_ADDR,
	TOTAL_I2C_ADDR,
};

enum
{
	WITH_AD = 0,
	WITHOUT_AD,
	TOTAL_VERSION,
};

#define WORD_MODE 0 //Div2D5-OwenHuang-SF8_Chnage_ALS_Resolution-00+

typedef struct
{
	unsigned int i2c_table[TOTAL_I2C_ADDR];
}cm3623_i2c_address;

static cm3623_i2c_address i2c_set[TOTAL_VERSION] = {
	{
		.i2c_table = {0x48, 0x49, 0x78, 0x79}, //with AD
	},
	{
		.i2c_table = {0x10, 0x11, 0x58, 0x59}, //without AD
	},
};

static int i2c_set_index = WITH_AD;
//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}

static unsigned char current_ps_sensivity = 0; //Div2D5-OwenHuang-SF8_CM3623_Sensivity_Node-00+

#define delay_to_jiffies(d) ((d)?msecs_to_jiffies(d):1)
#ifdef LOGE
#undef LOGE
#endif
#ifdef LOGD
#undef LOGD
#endif
#ifdef LOGI
#undef LOGI
#endif
#define LOGI(msg, ...) printk("[CM3623 INF]%s() " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define LOGE(msg, ...) printk("[CM3623 ERR]%s() " msg "\n", __FUNCTION__, ## __VA_ARGS__)

#if DEBUG
#define LOGD(msg, ...) printk("[CM3623 DBG]%s() " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#else
#define LOGD(msg, ...) //printk("[CM3623 DBG]%s() " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#endif

//Div2D5-OwenHuang-ALSPS-CM3623_FTM_Porting-01+{
#if defined(CONFIG_FIH_PROJECT_SF8)
#define CM3623_GPIO_INT    180 //for project SF8
#else
#define CM3623_GPIO_INT    180 //for big-board
#endif
//Div2D5-OwenHuang-ALSPS-CM3623_FTM_Porting-01+}

static int cm3623_init_chip(void); //Div2D5-OwenHuang-SF8_ALSPS_Threshold-00+

static int suspend(int);
static int resume(int);

struct sensor_data {
    struct mutex mutex;
    int enabled;
    int delay;
    struct delayed_work work;
#if DEBUG
    int suspend;
#endif
};

static bool g_phone_call = false;
//static bool g_irq_enable = false; //Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-
static int g_cm3623_irq = -1;
static struct input_dev *this_ps_data = NULL;
static struct input_dev *this_als_data = NULL;
static struct i2c_client *this_client = NULL;

static int cm_i2c_write_byte(struct i2c_client *client, int addr, int buf)
{
    int rc;
    u8 txdata[1];
	int retry = 0;
    struct i2c_msg msg;
    txdata[0] = buf & 0xFF;
    msg.addr = addr;
    msg.flags = client->flags & I2C_M_TEN;
    msg.len = 1;
    msg.buf = txdata;

	while (retry < I2C_RETRY_TIME)
	{
    	rc = i2c_transfer(client->adapter, &msg, 1);
		if (rc > 0)
		{
			LOGD("i2c write done!");
			break;
		}
		else
		{
			LOGE("i2c write byte failed! rc = %d", rc);
			retry ++;
		}
	}
	
    LOGD("rc = %d", rc);
	return rc;
}

static int cm_i2c_read_byte(struct i2c_client *client, int addr, char *data)
{
    int rc;
	int retry = 0;
    struct i2c_msg msg;
    msg.addr = addr;
    msg.flags = client->flags & I2C_M_TEN;
    msg.flags |= I2C_M_RD;
    msg.len = 1;
    msg.buf = data;

	while (retry < I2C_RETRY_TIME)
	{
    	rc = i2c_transfer(client->adapter, &msg, 1);
		if (rc > 0)
		{
			LOGD("i2c read done!");
			break;
		}
		else
		{
			LOGE("i2c read byte failed! rc = %d", rc);
			retry ++;
		}
	}

	return rc;
}

//Div2D5-OwenHuang-SF8_ALSPS_Threshold-00+{
//als sensor settings
#define GAIN_ALS 0x03 << 6
#define THD_ALS  0x00 << 4
#define IT_ALS   0x00 << 2
#define WDM      0x01 << 1
#define SD_ALS   0x00 << 0 
//Div2D5-OwenHuang-SF8_Chnage_ALS_Resolution-00+{
#if WORD_MODE
#define ALS_THRESHOLD (GAIN_ALS | THD_ALS | IT_ALS | WDM | SD_ALS)
#else
#define ALS_THRESHOLD (THD_ALS | IT_ALS | SD_ALS)
#endif
//Div2D5-OwenHuang-SF8_Chnage_ALS_Resolution-00+}

//proximity sensor settings
#define PS_INT_THRESHOLD 0x16 //5CM, see the spec, page13
static int cm3623_init_chip(void)
{
	int ret = 0;
	char buf[10];
	LOGI("");

	//initial als sensor threshold
	buf[0] = ALS_THRESHOLD;
	if (cm_i2c_write_byte(this_client, i2c_set[i2c_set_index].i2c_table[ALS_CMD_I2C_ADDR], buf[0]) < 0)
	{
		ret = -EIO;
		goto init_done;
	}

	//initial proximity sensor threshold
	buf[0] = PS_INT_THRESHOLD;
	if (cm_i2c_write_byte(this_client, i2c_set[i2c_set_index].i2c_table[PS_CMD2_I2C_ADDR], buf[0]) < 0)
	{
		ret = -EIO;
		goto init_done;
	}
	current_ps_sensivity = PS_INT_THRESHOLD; //Div2D5-OwenHuang-SF8_CM3623_Sensivity_Node-00+

init_done:	
	return ret;
}
//Div2D5-OwenHuang-SF8_ALSPS_Threshold-00+}

static int
suspend(int type)
{
    /* implement suspend of the sensor */
    struct sensor_data *data;
    LOGD("type=%d", type);
    switch(type) {
    case PS_SENSOR_TYPE: // ps
		//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
		//if (cm_i2c_write_byte(this_client, PS_CMD1_ADDR, 0x1) < 0)
		if (cm_i2c_write_byte(this_client, i2c_set[i2c_set_index].i2c_table[PS_CMD1_I2C_ADDR], 0x1) < 0)
		//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}
		{
        	LOGE("proximity sensor - i2c transfer failed\n");
        }
		
        data = input_get_drvdata(this_ps_data);

		//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-[
        /*if(g_irq_enable) {
            LOGD("disable irq");
            disable_irq(g_cm3623_irq);
            g_irq_enable = false;
        }*/
        //Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-]
        cancel_delayed_work_sync(&data->work);
        break;
    case ALS_SENSOR_TYPE: // als
    	//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
        //if (cm_i2c_write_byte(this_client, ALS_CMD_ADDR, 0x3B) < 0)
        if (cm_i2c_write_byte(this_client, i2c_set[i2c_set_index].i2c_table[ALS_CMD_I2C_ADDR], 0x3B) < 0)
		//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}
		{
        	LOGE("light sensor - i2c transfer failed\n");
        }
		
        data = input_get_drvdata(this_als_data);
        cancel_delayed_work_sync(&data->work);
        break;
    }
    return 0;
}

static int
resume(int type)
{
    /* implement resume of the sensor */
    struct sensor_data *data;
    LOGD("type=%d", type);
    switch(type) {
    case PS_SENSOR_TYPE: // ps
    	//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
        //if (cm_i2c_write_byte(this_client, PS_CMD1_ADDR, 0x0) < 0)
        if (cm_i2c_write_byte(this_client, i2c_set[i2c_set_index].i2c_table[PS_CMD1_I2C_ADDR], 0x00) < 0)
		//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}
		{
        	LOGE("proximity sensor i2c transfer failed");
        }
		
        data = input_get_drvdata(this_ps_data);

		//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-[
		/*if(!g_irq_enable) {
            LOGD("enable irq");
            enable_irq(g_cm3623_irq);
            g_irq_enable = true;
        }*/
        //Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-]
		schedule_delayed_work(&data->work,
				      delay_to_jiffies(data->delay) + 1);
        break;
    case ALS_SENSOR_TYPE: // als
    	//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
        //if (cm_i2c_write_byte(this_client, ALS_CMD_ADDR, 0x3A) < 0)
        if (cm_i2c_write_byte(this_client, i2c_set[i2c_set_index].i2c_table[ALS_CMD_I2C_ADDR], 0x3A) < 0)
		//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}
		{
        	LOGE("light sensor - i2c transfer failed");
        }
		
        data = input_get_drvdata(this_als_data);
		schedule_delayed_work(&data->work, 
			delay_to_jiffies(data->delay) + 1);
		
        break;
    }
#if DEBUG
        data->suspend = 0;
#endif
    return 0;
}

static void ps_work_func(struct work_struct *work)
{
	struct sensor_data *data = input_get_drvdata(this_ps_data);
	unsigned long delay = delay_to_jiffies(data->delay);

	LOGD("GPIO29 INT=%d", gpio_get_value(CM3623_GPIO_INT));

	input_report_abs(this_ps_data, ABS_X, gpio_get_value(CM3623_GPIO_INT)*1000*9);	
	input_sync(this_ps_data);
    schedule_delayed_work(&data->work, delay);
}
static void als_work_func(struct work_struct *work)
{
	struct sensor_data *data = input_get_drvdata(this_als_data);
	unsigned long delay = delay_to_jiffies(data->delay);
	int ret, lux_value; //Div2D5-OwenHuang-SF8_Enable_Auto_Backlight-00*
	unsigned char buf[2]; //Div2D5-OwenHuang-SF8_Enable_Auto_Backlight-00*
	buf[0] = buf[1] = 0;

//Div2D5-OwenHuang-SF8_Chnage_ALS_Resolution-00+{
#if WORD_MODE
	//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
	//ret = cm_i2c_read_byte(this_client, ALS_CMD_ADDR, &buf[0]);
	ret = cm_i2c_read_byte(this_client, i2c_set[i2c_set_index].i2c_table[ALS_CMD_I2C_ADDR], &buf[0]);
	//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}
	if (ret > 0)
	{
		//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
		//ret = cm_i2c_read_byte(this_client, ALS_DATA_ADDR, &buf[1]);
		ret = cm_i2c_read_byte(this_client, i2c_set[i2c_set_index].i2c_table[ALS_DATA_I2C_ADDR], &buf[1]); 
		//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}
	}

	//Div2D5-OwenHuang-SF8_Enable_Auto_Backlight-00+{
	//skip the first it and second unit
	lux_value = (int)(((int)(((int)buf[0])<<8 | ((int)buf[1]))) / 100);
	lux_value *= 100;
	//Div2D5-OwenHuang-SF8_Enable_Auto_Backlight-00+}
#else
	ret = cm_i2c_read_byte(this_client, i2c_set[i2c_set_index].i2c_table[ALS_CMD_I2C_ADDR], &buf[0]);
	lux_value = (int)buf[0];
#endif
//Div2D5-OwenHuang-SF8_Chnage_ALS_Resolution-00+}

	LOGD("ALS=%d",((int)(((int)buf[0])<<8 | ((int)buf[1]))));

	if (ret > 0)
	{
		input_report_abs(this_als_data, ABS_X,  lux_value * 1000); //Div2D5-OwenHuang-SF8_Enable_Auto_Backlight-00*
	}
	else
	{
		input_report_abs(this_als_data, ABS_X, (-1)); //if failed return negative value
	}
	
	input_sync(this_als_data);
    schedule_delayed_work(&data->work, delay);
}

/* Sysfs interface */
static ssize_t
ps_delay_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int delay;

    mutex_lock(&data->mutex);

    delay = data->delay;

    mutex_unlock(&data->mutex);

    return sprintf(buf, "%d\n", delay);
}
static ssize_t
als_delay_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int delay;

    mutex_lock(&data->mutex);

    delay = data->delay;

    mutex_unlock(&data->mutex);

    return sprintf(buf, "%d\n", delay);
}

static ssize_t
ps_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int value = simple_strtoul(buf, NULL, 10);

    if (value < 0) {
        return count;
    }

    if (SENSOR_MAX_DELAY < value) {
        value = SENSOR_MAX_DELAY;
    }

    mutex_lock(&data->mutex);

    data->delay = value;

    input_report_abs(input_data, ABS_CONTROL_REPORT, (data->enabled<<16) | value);
    if(data->enabled) {
        cancel_delayed_work_sync(&data->work);
        schedule_delayed_work(&data->work,
		      delay_to_jiffies(data->delay) + 1);
    }
    mutex_unlock(&data->mutex);

    return count;
}
static ssize_t
als_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int value = simple_strtoul(buf, NULL, 10);

    if (value < 0) {
        return count;
    }

    if (SENSOR_MAX_DELAY < value) {
        value = SENSOR_MAX_DELAY;
    }

    mutex_lock(&data->mutex);

    data->delay = value;

    input_report_abs(input_data, ABS_CONTROL_REPORT, (data->enabled<<16) | value);

    if(data->enabled) {
        cancel_delayed_work_sync(&data->work);
        schedule_delayed_work(&data->work,
		      delay_to_jiffies(data->delay) + 1);
    }

    mutex_unlock(&data->mutex);

    return count;
}

static ssize_t
ps_enable_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int enabled;

    mutex_lock(&data->mutex);

    enabled = data->enabled;

    mutex_unlock(&data->mutex);

    return sprintf(buf, "%d\n", enabled);
}

static ssize_t
als_enable_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int enabled;

    mutex_lock(&data->mutex);

    enabled = data->enabled;

    mutex_unlock(&data->mutex);

    return sprintf(buf, "%d\n", enabled);
}

static ssize_t
ps_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int value = simple_strtoul(buf, NULL, 10);

    if (value != 0 && value != 1) {
        return count;
    }

    mutex_lock(&data->mutex);

    if (data->enabled && !value) {
        suspend(PS_SENSOR_TYPE);
    }
    if (!data->enabled && value) {
        resume(PS_SENSOR_TYPE);
    }
    data->enabled = value;

    input_report_abs(input_data, ABS_CONTROL_REPORT, (value<<16) | data->delay);

    mutex_unlock(&data->mutex);

    return count;
}
static ssize_t
als_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);
    int value = simple_strtoul(buf, NULL, 10);

    if (value != 0 && value != 1) {
        return count;
    }

    mutex_lock(&data->mutex);

    if (data->enabled && !value) {
        suspend(ALS_SENSOR_TYPE);
    }
    if (!data->enabled && value) {

        resume(ALS_SENSOR_TYPE);
    }
    data->enabled = value;

    input_report_abs(input_data, ABS_CONTROL_REPORT, (value<<16) | data->delay);

    mutex_unlock(&data->mutex);

    return count;
}

static ssize_t
ps_wake_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    static int cnt = 1;

    input_report_abs(input_data, ABS_WAKE, cnt++);

    return count;
}

static ssize_t
ps_phone_store(struct device *dev, struct device_attribute *attr,
        								const char *buf, size_t count)
{
    int value = simple_strtoul(buf, NULL, 10);
    LOGD("value=%d",value);
    if(value) 
	{
        g_phone_call = true;
		//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00+{
		LOGE("enable irq in phone mode!");
		enable_irq(g_cm3623_irq);
		set_irq_wake(g_cm3623_irq, 1);
		//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00+}
    } 
	else 
    {
        g_phone_call = false;
		//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00+{
		LOGE("disable irq in phone mode!");
		set_irq_wake(g_cm3623_irq, 0);
		disable_irq(g_cm3623_irq);
		//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00+}
    }

    return count;
}

static ssize_t
als_wake_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    static int cnt = 1;

    input_report_abs(input_data, ABS_WAKE, cnt++);

    return count;
}

static ssize_t
ps_data_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    unsigned long flags;
    int x;

    spin_lock_irqsave(&input_data->event_lock, flags);

    x = input_data->abs[ABS_X];

    spin_unlock_irqrestore(&input_data->event_lock, flags);

    return sprintf(buf, "%d\n", x);
}
static ssize_t
als_data_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    unsigned long flags;
    int x;

    spin_lock_irqsave(&input_data->event_lock, flags);

    x = input_data->abs[ABS_X];

    spin_unlock_irqrestore(&input_data->event_lock, flags);

    return sprintf(buf, "%d\n", x);
}

static ssize_t
ps_status_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    unsigned long flags;
    int status;

    spin_lock_irqsave(&input_data->event_lock, flags);

    status = input_data->abs[ABS_STATUS];

    spin_unlock_irqrestore(&input_data->event_lock, flags);

    return sprintf(buf, "%d\n", status);
}
static ssize_t
als_status_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    unsigned long flags;
    int status;

    spin_lock_irqsave(&input_data->event_lock, flags);

    status = input_data->abs[ABS_STATUS];

    spin_unlock_irqrestore(&input_data->event_lock, flags);

    return sprintf(buf, "%d\n", status);
}

#if DEBUG
static ssize_t ps_debug_suspend_show(struct device *dev,
                                         struct device_attribute *attr, char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);

    return sprintf(buf, "%d\n", data->suspend);
}
static ssize_t als_debug_suspend_show(struct device *dev,
                                         struct device_attribute *attr, char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct sensor_data *data = input_get_drvdata(input_data);

    return sprintf(buf, "%d\n", data->suspend);
}

static ssize_t ps_debug_suspend_store(struct device *dev,
                                          struct device_attribute *attr,
                                          const char *buf, size_t count)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);

    if (value) {
        suspend();
    } else {
        resume();
    }

    return count;
}
static ssize_t als_debug_suspend_store(struct device *dev,
                                          struct device_attribute *attr,
                                          const char *buf, size_t count)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);

    if (value) {
        suspend();
    } else {
        resume();
    }

    return count;
}
#endif /* DEBUG */

//Div2D5-OwenHuang-SF8_CM3623_Sensivity_Node-00+{
static ssize_t ps_sensivity_show(struct device *dev,
        						struct device_attribute *attr, char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    unsigned long flags;
    int value;

    spin_lock_irqsave(&input_data->event_lock, flags);

    value = current_ps_sensivity;

    spin_unlock_irqrestore(&input_data->event_lock, flags);

    return sprintf(buf, "%d\n", value);
}

static ssize_t ps_sensivity_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    int value = simple_strtoul(buf, NULL, 10);
	unsigned char write_buf[10];

	LOGD("value=%d",value);

	if (value < 0x03 || value > 0xFF)
	{
		LOGE("value is overflow");
		return 0;
	}

	write_buf[0] = value;
	if (cm_i2c_write_byte(this_client, i2c_set[i2c_set_index].i2c_table[PS_CMD2_I2C_ADDR], write_buf[0]) < 0)
	{
		LOGE("i2c failed");
		return 0;
	}
	current_ps_sensivity = value; 
	
    return count;
}
//Div2D5-OwenHuang-SF8_CM3623_Sensivity_Node-00+}

static struct device_attribute ps_dev_attr_delay = __ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
        ps_delay_show, ps_delay_store);
static struct device_attribute ps_dev_attr_enable = __ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
        ps_enable_show, ps_enable_store);
static struct device_attribute ps_dev_attr_wake = __ATTR(wake, S_IWUSR|S_IWGRP,
        NULL, ps_wake_store);
static struct device_attribute ps_dev_attr_data = __ATTR(data, S_IRUGO, ps_data_show, NULL);
static struct device_attribute ps_dev_attr_status = __ATTR(status, S_IRUGO, ps_status_show, NULL);
static struct device_attribute ps_dev_attr_phone = __ATTR(phone, S_IWUSR|S_IWGRP, NULL, ps_phone_store);
//Div2D5-OwenHuang-SF8_CM3623_Sensivity_Node-00+{
static struct device_attribute ps_dev_attr_sensivity = __ATTR(sensivity, S_IWUGO|S_IRUGO|S_IWUSR|S_IWGRP, //Div2D5-OwenHuang-SF8_FQC_PSensor_Sensivity-00*
																ps_sensivity_show, ps_sensivity_store);
//Div2D5-OwenHuang-SF8_CM3623_Sensivity_Node-00+}

static struct device_attribute als_dev_attr_delay = __ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
        als_delay_show, als_delay_store);
static struct device_attribute als_dev_attr_enable = __ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
        als_enable_show, als_enable_store);
static struct device_attribute als_dev_attr_wake = __ATTR(wake, S_IWUSR|S_IWGRP,
        NULL, als_wake_store);
static struct device_attribute als_dev_attr_data = __ATTR(data, S_IRUGO, als_data_show, NULL);
static struct device_attribute als_dev_attr_status = __ATTR(status, S_IRUGO, als_status_show, NULL);

#if DEBUG
static struct device_attribute ps_dev_attr_data = __ATTR(debug_suspend, S_IRUGO|S_IWUSR,
                   ps_debug_suspend_show, ps_debug_suspend_store);
static struct device_attribute als_dev_attr_data = __ATTR(debug_suspend, S_IRUGO|S_IWUSR,
                   als_debug_suspend_show, als_debug_suspend_store);
#endif /* DEBUG */

static struct attribute *ps_attributes[] = {
    &ps_dev_attr_delay.attr,
    &ps_dev_attr_enable.attr,
    &ps_dev_attr_wake.attr,
    &ps_dev_attr_data.attr,
    &ps_dev_attr_status.attr,
    &ps_dev_attr_phone.attr,
#if DEBUG
    &ps_dev_attr_debug_suspend.attr,
#endif /* DEBUG */
	&ps_dev_attr_sensivity.attr, //Div2D5-OwenHuang-SF8_CM3623_Sensivity_Node-00+
    NULL
};

static struct attribute *als_attributes[] = {
    &als_dev_attr_delay.attr,
    &als_dev_attr_enable.attr,
    &als_dev_attr_wake.attr,
    &als_dev_attr_data.attr,
    &als_dev_attr_status.attr,
#if DEBUG
    &als_dev_attr_debug_suspend.attr,
#endif /* DEBUG */
    NULL
};

static struct attribute_group ps_attribute_group = {
    .attrs = ps_attributes
};
static struct attribute_group als_attribute_group = {
    .attrs = als_attributes
};

static int
//cm3623_suspend(struct platform_device *pdev, pm_message_t state)
cm3623_suspend(struct i2c_client *client, pm_message_t mesg)
{
    struct sensor_data *ps_data = input_get_drvdata(this_ps_data);
    struct sensor_data *als_data = input_get_drvdata(this_als_data);
    int rt = 0;
    LOGI("");
    mutex_lock(&ps_data->mutex);

	//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-{
    /*if(g_phone_call) {
        LOGD("under phone call irq%d", g_cm3623_irq);
        set_irq_wake(g_cm3623_irq, 1);
    } 
    else 
    {

    }*/
    //Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-}
    
    if(ps_data->enabled) {
        cancel_delayed_work_sync(&ps_data->work);
    }
    if(als_data->enabled) {
        cancel_delayed_work_sync(&als_data->work);
    }
    mutex_unlock(&ps_data->mutex);

    return rt;
}

static int
//cm3623_resume(struct platform_device *pdev)
cm3623_resume(struct i2c_client *client)
{
    struct sensor_data *ps_data = input_get_drvdata(this_ps_data);
    struct sensor_data *als_data = input_get_drvdata(this_als_data);
    int rt = 0;
    LOGI("");
    mutex_lock(&ps_data->mutex);

	//Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-{
	/*if(g_phone_call) {
        LOGD("under phone call irq%d", g_cm3623_irq);
        set_irq_wake(g_cm3623_irq, 0);
        //notify system to screen on
        //input_report_key(this_ps_data, KEY_F14,1);
        //input_report_key(this_ps_data, KEY_F14,0);
        //input_sync(this_ps_data);
    }*/
    //Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-}
    
    if (als_data->enabled) {
		schedule_delayed_work(&als_data->work,
				      delay_to_jiffies(als_data->delay) + 1);
    }
    if (ps_data->enabled) {
		schedule_delayed_work(&ps_data->work,
				      delay_to_jiffies(ps_data->delay) + 1);
    }

    mutex_unlock(&ps_data->mutex);

    return rt;
}

static irqreturn_t cm3623_isr( int irq, void * dev_id)
{
    LOGI("");
	return IRQ_HANDLED;
}

static int
cm3623_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct sensor_data *ps_data = NULL;
    struct input_dev *ps_input_data = NULL;
    struct sensor_data *als_data = NULL;
    struct input_dev *als_input_data = NULL;
    
    int input_registered = 0, sysfs_created = 0;
    int rt;
	int retry = 0;

    ps_data = kzalloc(sizeof(struct sensor_data), GFP_KERNEL);
    als_data = kzalloc(sizeof(struct sensor_data), GFP_KERNEL);

    if (!ps_data || !als_data) {
        rt = -ENOMEM;
        goto err;
    }

    ps_data->enabled = 0;
    ps_data->delay = SENSOR_DEFAULT_DELAY;
    als_data->enabled = 0;
	als_data->delay = SENSOR_DEFAULT_DELAY;

    ps_input_data = input_allocate_device();
    als_input_data = input_allocate_device();

    if (!ps_input_data || !als_input_data) {
        rt = -ENOMEM;
        LOGE("Failed to allocate input_data device");
        goto err;
    }

    set_bit(EV_ABS, ps_input_data->evbit);
    set_bit(EV_KEY, ps_input_data->evbit);
    set_bit(KEY_F14, ps_input_data->keybit);
    input_set_capability(ps_input_data, EV_ABS, ABS_X);
    input_set_capability(ps_input_data, EV_ABS, ABS_STATUS); /* status */
    input_set_capability(ps_input_data, EV_ABS, ABS_WAKE); /* wake */
    input_set_capability(ps_input_data, EV_ABS, ABS_CONTROL_REPORT); /* enabled/delay */
    ps_input_data->name = PS_NAME;

    set_bit(EV_ABS, als_input_data->evbit);
    input_set_capability(als_input_data, EV_ABS, ABS_X);
    input_set_capability(als_input_data, EV_ABS, ABS_STATUS); /* status */
    input_set_capability(als_input_data, EV_ABS, ABS_WAKE); /* wake */
    input_set_capability(als_input_data, EV_ABS, ABS_CONTROL_REPORT); /* enabled/delay */
    als_input_data->name = ALS_NAME;

	/* setup driver interfaces */
	INIT_DELAYED_WORK(&ps_data->work, ps_work_func);
	INIT_DELAYED_WORK(&als_data->work, als_work_func);

    rt = input_register_device(ps_input_data);
    if (rt) {
        LOGE("Unable to register input_data device: %s",ps_input_data->name);
        goto err;
    }
    rt = input_register_device(als_input_data);
    if (rt) {
        LOGE("Unable to register input_data device: %s", als_input_data->name);
        goto err;
    }
    input_set_drvdata(ps_input_data, ps_data);
    input_set_drvdata(als_input_data, als_data);
    input_registered = 1;

    rt = sysfs_create_group(&ps_input_data->dev.kobj,
            &ps_attribute_group);
    if (rt) {
        LOGE("sysfs_create_group failed[%s]", ps_input_data->name);
        goto err;
    }
    rt = sysfs_create_group(&als_input_data->dev.kobj,
            &als_attribute_group);
    if (rt) {
        LOGE("sysfs_create_group failed[%s]", als_input_data->name);
        goto err;
    }

    sysfs_created = 1;
    mutex_init(&ps_data->mutex);
    mutex_init(&als_data->mutex);
    this_ps_data = ps_input_data;
    this_als_data = als_input_data;

	rt = gpio_request(CM3623_GPIO_INT, "cm3623_int");
	if (rt)
	{
		LOGE("gpio request failed\n");
		goto err;
	}

    rt = gpio_tlmm_config(GPIO_CFG(CM3623_GPIO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, 
															GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    if(rt) 
	{
        LOGE("GPIO%d gpio_tlmm_config=%d", CM3623_GPIO_INT, rt);
        goto err;
    }
    gpio_direction_input(CM3623_GPIO_INT);
    g_cm3623_irq = MSM_GPIO_TO_INT(CM3623_GPIO_INT);
    
    this_client = client;
    rt = request_irq(g_cm3623_irq, cm3623_isr, IRQF_TRIGGER_RISING, "cm3623", NULL);
    if(rt) {
        LOGE("request_irq failed! rc=%d",rt);
        goto err;
    }
    disable_irq(g_cm3623_irq);
    //g_irq_enable = false; //Div2D5-OwenHuang-SF8_Proximity_Phone_Mode-00-

    while(gpio_get_value(CM3623_GPIO_INT) == 0 && retry < 5)
	{
		msleep(20);
		retry ++;
        LOGE("wait GPIO29 high");
    }

	if (retry == 5)
		goto err_gpio;

//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+{
	//if (cm_i2c_write_byte(client, ALS_DATA_ADDR, 0x20) < 0)
	if (cm_i2c_write_byte(client, i2c_set[i2c_set_index].i2c_table[ALS_DATA_I2C_ADDR], 0x20) < 0)
	{
		LOGE("There is no CM6323 chip on i2c bus!, check another one(Non-AD)");
		//goto err_i2c;
		i2c_set_index = WITHOUT_AD;
		goto i2c_another;
	}
	else
	{
		goto done;
	}

i2c_another:
	//if (cm_i2c_write_byte(client, ALS_DATA_ADDR, 0x20) < 0)
	if (cm_i2c_write_byte(client, i2c_set[i2c_set_index].i2c_table[ALS_DATA_I2C_ADDR], 0x20) < 0)
	{
		LOGE("There is no CM6323 chip on i2c bus!");
		goto err_i2c;
	}
	else
	{
		goto done;
	}

done:
//Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00+}

	//Div2D5-OwenHuang-SF8_ALSPS_Threshold-00+{
	if (cm3623_init_chip() < 0) //inital als/ps threshold of cm3623
	{
		LOGE("initilize cm3623 chip failed!");
		goto err_i2c;
	}
	//Div2D5-OwenHuang-SF8_ALSPS_Threshold-00+}

	//Div2D5-OwenHuang-SF8_CM3623_PowerControl-00+{
	//shutdown the light/proximity sensor, after initializing chip
	suspend(ALS_SENSOR_TYPE);
	suspend(PS_SENSOR_TYPE);
	//Div2D5-OwenHuang-SF8_CM3623_PowerControl-00+}

	LOGI("CM3623 initialize OK!, %s", i2c_set_index ? "Without AD version(0x49)" : "Wtih AD Version(0x11)"); //Div2D5-OwenHuang-ALSPS-CM3623_I2C_Address-00*

    return 0;

err_i2c:
err_gpio:
	free_irq(g_cm3623_irq, NULL);	
err:
    if (ps_data != NULL) {
        if (ps_input_data != NULL) {
            if (sysfs_created) {
                sysfs_remove_group(&ps_input_data->dev.kobj,
                        &ps_attribute_group);
            }
            if (input_registered) {
                input_unregister_device(ps_input_data);
            }
            else {
                input_free_device(ps_input_data);
            }
            ps_input_data = NULL;
        }
        kfree(ps_data);
    }
    if (als_data != NULL) {
        if (als_input_data != NULL) {
            if (sysfs_created) {
                sysfs_remove_group(&als_input_data->dev.kobj,
                        &als_attribute_group);
            }
            if (input_registered) {
                input_unregister_device(als_input_data);
            }
            else {
                input_free_device(als_input_data);
            }
            als_input_data = NULL;
        }
        kfree(als_data);
    }
    return rt;
}

static int
cm3623_remove(struct i2c_client *client)
{
    struct sensor_data *data;

    if (this_ps_data != NULL) {
        data = input_get_drvdata(this_ps_data);
        sysfs_remove_group(&this_ps_data->dev.kobj,
                &ps_attribute_group);
        input_unregister_device(this_ps_data);
        if (data != NULL) {
            kfree(data);
        }
    }
    if (this_als_data != NULL) {
        data = input_get_drvdata(this_als_data);
        sysfs_remove_group(&this_als_data->dev.kobj,
                &als_attribute_group);
        input_unregister_device(this_als_data);
        if (data != NULL) {
            kfree(data);
        }
    }

	free_irq(g_cm3623_irq, NULL); 
	
    return 0;
}

/* I2C Device Driver */
static struct i2c_device_id cm3623_idtable[] = {
    {SENSOR_NAME, 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, cm3623_idtable);

static struct i2c_driver cm3623_i2c_driver = {
    .probe          = cm3623_probe,
    .remove         = cm3623_remove,
	.suspend        = cm3623_suspend,
	.resume         = cm3623_resume,
    .id_table       = cm3623_idtable,
    .driver = {
        .name       = SENSOR_NAME,
    },
};

static int __init sensor_init(void)
{   
    return i2c_add_driver(&cm3623_i2c_driver);
}
module_init(sensor_init);

static void __exit sensor_exit(void)
{
    i2c_del_driver(&cm3623_i2c_driver);
}
module_exit(sensor_exit);

MODULE_AUTHOR("Yamaha Corporation");
MODULE_LICENSE( "GPL" );
MODULE_VERSION("1.1.0");
