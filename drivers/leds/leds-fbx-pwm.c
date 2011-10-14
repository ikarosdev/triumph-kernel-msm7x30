/* kernel/drivers/leds/fbx-leds-pwm.c
 *
 * Copyright (C) 2010 FIH Co., LTD.
 *
 * Author: Audi PC Huang
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/pmic8058-pwm.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include <mach/leds-fbx-pwm.h>

#define PM_PWM_BLINK    (PM_PWM_LUT_LOOP | PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_PAUSE_LO_EN | PM_PWM_LUT_RAMP_UP | PM_PWM_LUT_REVERSE)

struct fbx_leds_pwm_driver_data {
    struct mutex led_state_lock;
    struct led_classdev leds[3];
    struct pwm_device* pwm[3];
    unsigned int r_led_ctl;
    int led_state[3];
    int duty_pct[63];
    int pause_hi_ms;
    int pause_lo_ms;
};

enum {
    FBX_LED_OFF,
    FBX_LED_ON,
    FBX_LED_BLINK,
};

struct fbx_leds_pwm_driver_data *fbx_leds_pwm_dd = NULL;

static ssize_t fbx_leds_pwm_blink_solid_store(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{
    int on;
    int idx = 0;
    struct led_classdev *led_cdev = dev_get_drvdata(dev);

    if (!strcmp(led_cdev->name, "red"))
        idx = FBX_R_LED;
    else if (!strcmp(led_cdev->name, "green"))
        idx = FBX_G_LED;
    else if (!strcmp(led_cdev->name, "button-backlight"))
        idx = FBX_CAPS_KEY_LED;

    sscanf(buf, "%d", &on);
    dev_info(led_cdev->dev, "%s: IDX[%d] %s\n", __func__, idx, on?"BLINK":"STOP");
    
    mutex_lock(&fbx_leds_pwm_dd->led_state_lock);
    if (on) {
        if (fbx_leds_pwm_dd->led_state[idx] != FBX_LED_BLINK) {
            pm8058_pwm_lut_config(fbx_leds_pwm_dd->pwm[idx], 20000, fbx_leds_pwm_dd->duty_pct, 8, 0, 63, fbx_leds_pwm_dd->pause_lo_ms, fbx_leds_pwm_dd->pause_hi_ms, PM_PWM_BLINK);
            pm8058_pwm_lut_enable(fbx_leds_pwm_dd->pwm[idx], 1);
            
            fbx_leds_pwm_dd->led_state[idx] = FBX_LED_BLINK;
        }
    } else {
        if (fbx_leds_pwm_dd->led_state[idx] == FBX_LED_BLINK) {
            pm8058_pwm_lut_enable(fbx_leds_pwm_dd->pwm[idx], 0);
            
            fbx_leds_pwm_dd->led_state[idx] = FBX_LED_OFF;
        }
    }
    mutex_unlock(&fbx_leds_pwm_dd->led_state_lock);

    return size;
}

static DEVICE_ATTR(blink, 0644, NULL, fbx_leds_pwm_blink_solid_store);

static ssize_t fbx_leds_pwm_pause_hi_ms_store(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{
    sscanf(buf, "%d", &fbx_leds_pwm_dd->pause_hi_ms);
    dev_info(dev, "%s: Pause HI %d ms\n", __func__, fbx_leds_pwm_dd->pause_hi_ms);

    return size;
}

static DEVICE_ATTR(ledon, 0644, NULL, fbx_leds_pwm_pause_hi_ms_store);

static ssize_t fbx_leds_pwm_pause_lo_ms_store(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{
    sscanf(buf, "%d", &fbx_leds_pwm_dd->pause_lo_ms);
    dev_info(dev, "%s: Pause LO %d ms\n", __func__, fbx_leds_pwm_dd->pause_lo_ms);

    return size;
}

static DEVICE_ATTR(ledoff, 0644, NULL, fbx_leds_pwm_pause_lo_ms_store);

static void fbx_leds_pwm_led_brightness_set(struct led_classdev *led_cdev,
                   enum led_brightness brightness)	
{
    int idx = 0;
    
    if (!strcmp(led_cdev->name, "red"))
        idx = FBX_R_LED;
    else if (!strcmp(led_cdev->name, "green"))
        idx = FBX_G_LED;
    else if (!strcmp(led_cdev->name, "button-backlight"))
        idx = FBX_CAPS_KEY_LED;

    dev_info(led_cdev->dev, "%s: IDX[%d] %s %d\n", __func__, idx, brightness?"ON":"OFF", brightness);

    mutex_lock(&fbx_leds_pwm_dd->led_state_lock);
    if (brightness) {
        if (fbx_leds_pwm_dd->led_state[idx] != FBX_LED_ON) {
            if (idx == FBX_CAPS_KEY_LED) {                
/*
 * KD 2011-10-13
 * Oh do come on.  This board supports PWM brightness and the wonderful
 * people at Foxconn didn't bother with simple division?  Give me a f*ing 
 * break.  Fixed so you can CHOOSE how bright your keypad LED is.
 * 255 * 78 = approximately the previous 20000 value, 0 = 0, otherwise
 * proportional.  Note that we have to 
 */
//              pwm_config(fbx_leds_pwm_dd->pwm[idx], 20000, 20000);
                pwm_config(fbx_leds_pwm_dd->pwm[idx], (brightness * 78), 20000);
                pwm_enable(fbx_leds_pwm_dd->pwm[idx]);
            } else {
                pm8058_pwm_lut_config(fbx_leds_pwm_dd->pwm[idx], 20000, fbx_leds_pwm_dd->duty_pct, 8, 0, 63, 0, 0, PM_PWM_LUT_RAMP_UP);
                pm8058_pwm_lut_enable(fbx_leds_pwm_dd->pwm[idx], 1);
            }
            fbx_leds_pwm_dd->led_state[idx] = FBX_LED_ON;
        }
    } else {
        if (idx == FBX_CAPS_KEY_LED)
            pwm_disable(fbx_leds_pwm_dd->pwm[idx]);
        else
            pm8058_pwm_lut_enable(fbx_leds_pwm_dd->pwm[idx], 0);

        fbx_leds_pwm_dd->led_state[idx] = FBX_LED_OFF;
    }
    mutex_unlock(&fbx_leds_pwm_dd->led_state_lock);
}

static int fbx_leds_pwm_probe(struct platform_device *pdev)
{
    struct leds_fbx_pwm_platform_data *fbx_leds_pwm_pd = pdev->dev.platform_data;
    int ret = 0;
    int i   = 0;

    fbx_leds_pwm_dd = kzalloc(sizeof(struct fbx_leds_pwm_driver_data), GFP_KERNEL);
    if (fbx_leds_pwm_dd == NULL) {
        dev_err(&pdev->dev, "%s: no memory for device\n", __func__);
        ret = -ENOMEM;
        return ret;
    }
    memset(fbx_leds_pwm_dd, 0, sizeof(struct fbx_leds_pwm_driver_data));
    
    fbx_leds_pwm_dd->r_led_ctl = fbx_leds_pwm_pd->r_led_ctl;
    fbx_leds_pwm_dd->leds[FBX_R_LED].name           = "red";
    fbx_leds_pwm_dd->leds[FBX_G_LED].name           = "green";
    fbx_leds_pwm_dd->leds[FBX_CAPS_KEY_LED].name    = "button-backlight";
    fbx_leds_pwm_dd->pause_lo_ms = 2000;
    fbx_leds_pwm_dd->pause_hi_ms = 500;
    mutex_init(&fbx_leds_pwm_dd->led_state_lock);
   
    mutex_lock(&fbx_leds_pwm_dd->led_state_lock);
    for (i = 0; i < 3; i++) {
        fbx_leds_pwm_dd->leds[i].brightness_set = fbx_leds_pwm_led_brightness_set;

        ret = led_classdev_register(&pdev->dev, &fbx_leds_pwm_dd->leds[i]);
        if (ret) {
            dev_err(&pdev->dev,
                        "%s: led_classdev_register failed\n", __func__);
            goto err_led_classdev_register_failed;
        }
    
        dev_info(&pdev->dev,
                "%s: LED name(%s) was registered\n",
                __func__, fbx_leds_pwm_dd->leds[i].name);
                
        fbx_leds_pwm_dd->led_state[i] = FBX_LED_OFF;
    }
    mutex_unlock(&fbx_leds_pwm_dd->led_state_lock);

    for (i = 0; i < 3; i++) {
        ret = device_create_file(fbx_leds_pwm_dd->leds[i].dev,
                                    &dev_attr_blink);
        if (ret) {
            dev_err(&pdev->dev,
                        "%s: device_create_file failed\n", __func__);

            for (i = 0; i < 3; i++)
                device_remove_file(fbx_leds_pwm_dd->leds[i].dev, &dev_attr_blink);
                
            return ret;
        }
    }
    
    ret = device_create_file(&pdev->dev, &dev_attr_ledon);
    if (ret) {
        dev_err(&pdev->dev,
               "%s: create dev_attr_ledon failed\n", __func__);
        goto err_out_attr_ledon;
    }
    
    ret = device_create_file(&pdev->dev, &dev_attr_ledoff);
    if (ret) {
        dev_err(&pdev->dev,
               "%s: create dev_attr_ledoff failed\n", __func__);
        goto err_out_attr_ledoff;
    }
    
    dev_set_drvdata(&pdev->dev, fbx_leds_pwm_dd);
    
    fbx_leds_pwm_dd->pwm[0] = pwm_request(LPG_LED_DRV0, "red");
    fbx_leds_pwm_dd->pwm[1] = pwm_request(LPG_LED_DRV1, "green");
    fbx_leds_pwm_dd->pwm[2] = pwm_request(LPG_LED_DRV2, "button-backlight");

    for (i = 0; i < 3; i++) {
        if (fbx_leds_pwm_dd->pwm[i] == NULL) {
            dev_err(&pdev->dev,
               "%s: pwm_request failed. LPG[%d]\n", __func__, LPG_LED_DRV0 + i);
            for (i = 0; i < 3; i++) {
                pwm_free(fbx_leds_pwm_dd->pwm[i]);
                goto err_out_pwm_request;
            }
        }
    }
    
    for (i = 0; i < 64; i++)
        fbx_leds_pwm_dd->duty_pct[i] = i;

    dev_info(&pdev->dev, "%s finished!!\n", __func__);

    return 0;
    
err_out_pwm_request:
    device_remove_file(&pdev->dev, &dev_attr_ledoff);
        
err_out_attr_ledoff:
    device_remove_file(&pdev->dev, &dev_attr_ledon);
    
err_out_attr_ledon:
    for (i = 0; i < 3; i++)
        device_remove_file(fbx_leds_pwm_dd->leds[i].dev, &dev_attr_blink);

err_led_classdev_register_failed:
    for (i = 0; i < 3; i++)
        led_classdev_unregister(&fbx_leds_pwm_dd->leds[i]);

    mutex_destroy(&fbx_leds_pwm_dd->led_state_lock);
    kfree(fbx_leds_pwm_dd);

    return ret;
}

static int __devexit fbx_leds_pwm_remove(struct platform_device *pdev)
{
    struct fbx_leds_pwm_driver_data *fbx_leds_pwm_dd;
    int i;

    fbx_leds_pwm_dd = platform_get_drvdata(pdev);

    for (i = 0; i < 3; i++) {
        device_remove_file(fbx_leds_pwm_dd->leds[i].dev, &dev_attr_blink);
        led_classdev_unregister(&fbx_leds_pwm_dd->leds[i]);
        pwm_free(fbx_leds_pwm_dd->pwm[i]);
    }

    device_remove_file(&pdev->dev, &dev_attr_ledon);
    device_remove_file(&pdev->dev, &dev_attr_ledoff);
    mutex_destroy(&fbx_leds_pwm_dd->led_state_lock);
    kfree(fbx_leds_pwm_dd);

    return 0;
}

static struct platform_driver fbx_leds_pwm_driver = {
    .probe = fbx_leds_pwm_probe,
    .remove = __devexit_p(fbx_leds_pwm_remove),
    .driver =
            {
                .name = "fbx-leds-pwm",
                .owner = THIS_MODULE,
            },
};

static int __init fbx_leds_pwm_init(void)
{
    return platform_driver_register(&fbx_leds_pwm_driver);
}

static void __exit fbx_leds_pwm_exit(void)
{
    platform_driver_unregister(&fbx_leds_pwm_driver);
}

MODULE_AUTHOR("Audi PC Huang");
MODULE_DESCRIPTION("FBX PWM LEDs driver");
MODULE_LICENSE("GPL");

module_init(fbx_leds_pwm_init);
module_exit(fbx_leds_pwm_exit);
