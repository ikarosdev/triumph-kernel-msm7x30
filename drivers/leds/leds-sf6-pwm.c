
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/pmic8058-pwm.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include <mach/pmic.h>

#define PM_PWM_BLINK    (PM_PWM_LUT_LOOP | PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_PAUSE_LO_EN | PM_PWM_LUT_RAMP_UP | PM_PWM_LUT_REVERSE)

#define sf6_led_num		2

struct sf6_leds_pwm_driver_data {
    struct mutex led_state_lock;
    struct led_classdev leds[sf6_led_num];
    struct pwm_device* pwm[sf6_led_num];
    int led_state[sf6_led_num];
    int duty_pct[63];
    int pause_hi_ms;
    int pause_lo_ms;
};

enum {
    SF6_LED_OFF,
    SF6_LED_ON,
    SF6_LED_BLINK,
};

enum {
	R_LED		= LOW_CURRENT_LED_DRV0,
	G_LED		= LOW_CURRENT_LED_DRV1,
};

enum {
    LPG_PMIC_GPIO24,
    LPG_PMIC_GPIO25,
    LPG_PMIC_GPIO26,
    LPG_KYPD_DRV,
    LPG_LED_DRV0,
    LPG_LED_DRV1,
    LPG_LED_DRV2,
    LPG_FLASH_DRV0 = LPG_LED_DRV2,
    LPG_FLASH_DRV1,
};

struct sf6_leds_pwm_driver_data *sf6_leds_pwm_dd = NULL;

static ssize_t sf6_leds_pwm_blink_solid_store(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{
    int on;
    int idx = 0;
    struct led_classdev *led_cdev = dev_get_drvdata(dev);

    if (!strcmp(led_cdev->name, "red"))
        idx = R_LED;
    else if (!strcmp(led_cdev->name, "green"))
        idx = G_LED;

    sscanf(buf, "%d", &on);
    dev_info(led_cdev->dev, "%s: IDX[%d] %s\n", __func__, idx, on?"BLINK":"STOP");
    
    mutex_lock(&sf6_leds_pwm_dd->led_state_lock);
    if (on) {
        if (sf6_leds_pwm_dd->led_state[idx] != SF6_LED_BLINK) {
            
            pm8058_pwm_lut_config(sf6_leds_pwm_dd->pwm[idx], 20000, sf6_leds_pwm_dd->duty_pct, 8, 0, 63, sf6_leds_pwm_dd->pause_lo_ms, sf6_leds_pwm_dd->pause_hi_ms, PM_PWM_BLINK);
            pm8058_pwm_lut_enable(sf6_leds_pwm_dd->pwm[idx], 1);
            
            sf6_leds_pwm_dd->led_state[idx] = SF6_LED_BLINK;
        }
    } else {
        if (sf6_leds_pwm_dd->led_state[idx] == SF6_LED_BLINK) {
            pm8058_pwm_lut_enable(sf6_leds_pwm_dd->pwm[idx], 0);
            
            sf6_leds_pwm_dd->led_state[idx] = SF6_LED_OFF;
        }
    }
    mutex_unlock(&sf6_leds_pwm_dd->led_state_lock);

    return size;
}

static DEVICE_ATTR(blink, 0644, NULL, sf6_leds_pwm_blink_solid_store);

static ssize_t sf6_leds_pwm_pause_hi_ms_store(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{
    sscanf(buf, "%d", &sf6_leds_pwm_dd->pause_hi_ms);
    dev_info(dev, "%s: Pause HI %d ms\n", __func__, sf6_leds_pwm_dd->pause_hi_ms);

    return size;
}

static DEVICE_ATTR(ledon, 0644, NULL, sf6_leds_pwm_pause_hi_ms_store);

static ssize_t sf6_leds_pwm_pause_lo_ms_store(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t size)
{
    sscanf(buf, "%d", &sf6_leds_pwm_dd->pause_lo_ms);
    dev_info(dev, "%s: Pause LO %d ms\n", __func__, sf6_leds_pwm_dd->pause_lo_ms);

    return size;
}

static DEVICE_ATTR(ledoff, 0644, NULL, sf6_leds_pwm_pause_lo_ms_store);

static void sf6_leds_pwm_led_brightness_set(struct led_classdev *led_cdev,
                   enum led_brightness brightness)
{
    int idx = 0;
    
    if (!strcmp(led_cdev->name, "red"))
        idx = R_LED;
    else if (!strcmp(led_cdev->name, "green"))
        idx = G_LED;

    dev_info(led_cdev->dev, "%s: IDX[%d] %s\n", __func__, idx, brightness?"ON":"OFF");

    mutex_lock(&sf6_leds_pwm_dd->led_state_lock);
    if (brightness) {
        if (sf6_leds_pwm_dd->led_state[idx] != SF6_LED_ON) {
        	
		dev_info(led_cdev->dev, "Setting  IDX[%d] ON\n", idx);
		pm8058_pwm_lut_config(sf6_leds_pwm_dd->pwm[idx], 20000, sf6_leds_pwm_dd->duty_pct, 8, 0, 63, 0, 0, PM_PWM_LUT_RAMP_UP);
		pm8058_pwm_lut_enable(sf6_leds_pwm_dd->pwm[idx], 1);
    
		sf6_leds_pwm_dd->led_state[idx] = SF6_LED_ON;
        }
    } else {
		dev_info(led_cdev->dev, "Setting IDX[%d] OFF\n", idx);    	
		pm8058_pwm_lut_enable(sf6_leds_pwm_dd->pwm[idx], 0);

		sf6_leds_pwm_dd->led_state[idx] = SF6_LED_OFF;
    }
    mutex_unlock(&sf6_leds_pwm_dd->led_state_lock);
}

static int sf6_leds_pwm_probe(struct platform_device *pdev)
{
    int ret = 0;
    int i   = 0;

    sf6_leds_pwm_dd = kzalloc(sizeof(struct sf6_leds_pwm_driver_data), GFP_KERNEL);
    if (sf6_leds_pwm_dd == NULL) {
        dev_err(&pdev->dev, "%s: no memory for device\n", __func__);
        ret = -ENOMEM;
        return ret;
    }
    memset(sf6_leds_pwm_dd, 0, sizeof(struct sf6_leds_pwm_driver_data));
    
    sf6_leds_pwm_dd->leds[R_LED].name           = "red";
    sf6_leds_pwm_dd->leds[G_LED].name           = "green";

    sf6_leds_pwm_dd->pause_lo_ms = 2000;
    sf6_leds_pwm_dd->pause_hi_ms = 500;
    mutex_init(&sf6_leds_pwm_dd->led_state_lock);
   
    mutex_lock(&sf6_leds_pwm_dd->led_state_lock);
    for (i = 0; i < sf6_led_num; i++) {
        sf6_leds_pwm_dd->leds[i].brightness_set = sf6_leds_pwm_led_brightness_set;

        ret = led_classdev_register(&pdev->dev, &sf6_leds_pwm_dd->leds[i]);
        if (ret) {
            dev_err(&pdev->dev,
                        "%s: led_classdev_register failed\n", __func__);
            goto err_led_classdev_register_failed;
        }
    
        dev_info(&pdev->dev,
                "%s: LED name(%s) was registered\n",
                __func__, sf6_leds_pwm_dd->leds[i].name);
                
        sf6_leds_pwm_dd->led_state[i] = SF6_LED_OFF;
    }
    mutex_unlock(&sf6_leds_pwm_dd->led_state_lock);

    for (i = 0; i < sf6_led_num; i++) {
        ret = device_create_file(sf6_leds_pwm_dd->leds[i].dev,
                                    &dev_attr_blink);
        if (ret) {
            dev_err(&pdev->dev,
                        "%s: device_create_file failed\n", __func__);

            for (i = 0; i < sf6_led_num; i++)
                device_remove_file(sf6_leds_pwm_dd->leds[i].dev, &dev_attr_blink);
                
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
    
    dev_set_drvdata(&pdev->dev, sf6_leds_pwm_dd);
    
    sf6_leds_pwm_dd->pwm[0] = pwm_request(LPG_LED_DRV0, "red");
    sf6_leds_pwm_dd->pwm[1] = pwm_request(LPG_LED_DRV1, "green");

    for (i = 0; i < sf6_led_num; i++) {
        if (sf6_leds_pwm_dd->pwm[i] == NULL) {
            dev_err(&pdev->dev,
               "%s: pwm_request failed. LPG[%d]\n", __func__, LPG_LED_DRV0 + i);
            for (i = 0; i < sf6_led_num; i++) {
                pwm_free(sf6_leds_pwm_dd->pwm[i]);
                goto err_out_pwm_request;
            }
        }
    }
    
    for (i = 0; i < 64; i++)
        sf6_leds_pwm_dd->duty_pct[i] = i;

    dev_info(&pdev->dev, "%s finished!!\n", __func__);

    return 0;
    
err_out_pwm_request:
    device_remove_file(&pdev->dev, &dev_attr_ledoff);
        
err_out_attr_ledoff:
    device_remove_file(&pdev->dev, &dev_attr_ledon);
    
err_out_attr_ledon:
    for (i = 0; i < sf6_led_num; i++)
        device_remove_file(sf6_leds_pwm_dd->leds[i].dev, &dev_attr_blink);

err_led_classdev_register_failed:
    for (i = 0; i < sf6_led_num; i++)
        led_classdev_unregister(&sf6_leds_pwm_dd->leds[i]);

    mutex_destroy(&sf6_leds_pwm_dd->led_state_lock);
    kfree(sf6_leds_pwm_dd);

    return ret;
}

static int __devexit sf6_leds_pwm_remove(struct platform_device *pdev)
{
    struct sf6_leds_pwm_driver_data *sf6_leds_pwm_dd;
    int i;

    sf6_leds_pwm_dd = platform_get_drvdata(pdev);

    for (i = 0; i < sf6_led_num; i++) {
        device_remove_file(sf6_leds_pwm_dd->leds[i].dev, &dev_attr_blink);
        led_classdev_unregister(&sf6_leds_pwm_dd->leds[i]);
        pwm_free(sf6_leds_pwm_dd->pwm[i]);
    }

    device_remove_file(&pdev->dev, &dev_attr_ledon);
    device_remove_file(&pdev->dev, &dev_attr_ledoff);
    mutex_destroy(&sf6_leds_pwm_dd->led_state_lock);
    kfree(sf6_leds_pwm_dd);

    return 0;
}

static struct platform_driver sf6_leds_pwm_driver = {
    .probe = sf6_leds_pwm_probe,
    .remove = __devexit_p(sf6_leds_pwm_remove),
    .driver =
            {
                .name = "sf6-leds-pwm",
                .owner = THIS_MODULE,
            },
};

static int __init sf6_leds_pwm_init(void)
{
    return platform_driver_register(&sf6_leds_pwm_driver);
}

static void __exit sf6_leds_pwm_exit(void)
{
    platform_driver_unregister(&sf6_leds_pwm_driver);
}

MODULE_AUTHOR("");
MODULE_DESCRIPTION("");
MODULE_LICENSE("");

module_init(sf6_leds_pwm_init);
module_exit(sf6_leds_pwm_exit);
