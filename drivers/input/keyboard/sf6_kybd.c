#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/mfd/pmic8058.h> /* for controlling pmic8058 GPIO */
#include <mach/gpio.h>
#include <mach/sf6_kybd.h>
#include <mach/pmic.h>
#include <mach/irqs.h>
#include <mach/rpc_server_handset.h>

#define sf6_kybd_name       "sf6_kybd"
#define KEY_FOCUS           242		//SW5-BSP-Keypad_FTM
#define KBD_DEBOUNCE_TIME   8
#define KEYRELEASE       0
#define KEYPRESS            1

struct sf6_kybd_data {
    struct input_dev    *sf6_kybd_idev;
    struct device       *sf6_kybd_pdev;
    
    struct work_struct qkybd_volup;
    struct work_struct qkybd_voldn;
    struct work_struct qkybd_camf;
    struct work_struct qkybd_camt;
    
    unsigned int pmic_gpio_cam_f;
    unsigned int pmic_gpio_cam_t;
    unsigned int pmic_gpio_vol_up;
    unsigned int pmic_gpio_vol_dn;
    unsigned int sys_gpio_cam_f;
    unsigned int sys_gpio_cam_t;
    unsigned int sys_gpio_vol_up;
    unsigned int sys_gpio_vol_dn;
    
    bool kybd_connected;
};

struct sf6_kybd_data *rd = NULL;

static irqreturn_t sf6_kybd_irqhandler(int irq, void *dev_id)
{
    struct sf6_kybd_data *kbdrec = (struct sf6_kybd_data *) dev_id;

    if (kbdrec->kybd_connected) {
        if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_vol_up) == irq) {    
            schedule_work(&kbdrec->qkybd_volup);
        } else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_vol_dn) == irq) {    
            schedule_work(&kbdrec->qkybd_voldn);
        } else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_cam_f) == irq) {
            schedule_work(&kbdrec->qkybd_camf); 
        } else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_cam_t) == irq) {
            schedule_work(&kbdrec->qkybd_camt);  
	}
    }
    
    return IRQ_HANDLED;
}

static int sf6_kybd_irqsetup(void)
{
    int rc;
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_up), &sf6_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 sf6_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", sf6_kybd_name, rd->pmic_gpio_vol_up, rc);
        rc = -EIO;
    }
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_dn), &sf6_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 sf6_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", sf6_kybd_name, rd->pmic_gpio_vol_dn, rc);
        rc = -EIO;
    }
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_f), &sf6_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 sf6_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", sf6_kybd_name, rd->pmic_gpio_cam_f, rc);
        rc = -EIO;
    }

    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_t), &sf6_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 sf6_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", sf6_kybd_name, rd->pmic_gpio_cam_t, rc);
        rc = -EIO;
    }
        
    return rc;
}

static void sf6_kybd_release_gpio(void)
{
    gpio_free(rd->sys_gpio_vol_up);
    gpio_free(rd->sys_gpio_vol_dn);
    gpio_free(rd->sys_gpio_cam_t);
    gpio_free(rd->sys_gpio_cam_f);
}

static int sf6_kybd_config_gpio(void)
{
    struct pm8058_gpio button_configuration = {
        .direction      = PM_GPIO_DIR_IN,
        .pull           = PM_GPIO_PULL_DN,
        .vin_sel        = 2,
        .out_strength   = PM_GPIO_STRENGTH_NO,
        .function       = PM_GPIO_FUNC_NORMAL,
        .inv_int_pol    = 0,
    };
    int rc = 0;

    printk(KERN_INFO "%s\n", __func__);

    rc = pm8058_gpio_config(rd->pmic_gpio_vol_up, &button_configuration);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_up);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_vol_up, "vol_up_key");
        if (rc < 0) {
            dev_err(rd->sf6_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_vol_up);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_vol_up);
            if (rc < 0) {
                dev_err(rd->sf6_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_up);
                return rc;
            }
        }
    }
    
    rc = pm8058_gpio_config(rd->pmic_gpio_vol_dn, &button_configuration);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_dn);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_vol_dn, "vol_down_key");
        if (rc < 0) {
            dev_err(rd->sf6_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_vol_dn);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_vol_dn);
            if (rc < 0) {
                dev_err(rd->sf6_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_dn);
                return rc;
            }
        }
    }
 
    rc = pm8058_gpio_config(rd->pmic_gpio_cam_t, &button_configuration);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_t);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_cam_t, "camera_key");
        if (rc < 0) {
            dev_err(rd->sf6_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_cam_t);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_cam_t);
            if (rc < 0) {
                dev_err(rd->sf6_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_t);
                return rc;
            }
        }
    }
    
    rc = pm8058_gpio_config(rd->pmic_gpio_cam_f, &button_configuration);
    if (rc < 0) {
        dev_err(rd->sf6_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_f);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_cam_f, "camera_focus_key");
        if (rc < 0) {
            dev_err(rd->sf6_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_cam_f);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_cam_f); 
            if (rc < 0) {
                dev_err(rd->sf6_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_f);
                return rc;
            }
        }
    }
    
    return rc;
}

static void sf6_kybd_volup(struct work_struct *work)
{
    struct sf6_kybd_data *kbdrec  = container_of(work, struct sf6_kybd_data, qkybd_volup);
    struct input_dev *idev          = kbdrec->sf6_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value(kbdrec->sys_gpio_vol_up);
    
    dev_dbg(kbdrec->sf6_kybd_pdev, "%s: Key VOLUMEUP (%d) %s\n", __func__, KEY_VOLUMEUP, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (gpio_val) {
        input_report_key(idev, KEY_VOLUMEUP, KEYRELEASE); 
		printk("V_Up key release\n");						
    } else {
        input_report_key(idev, KEY_VOLUMEUP, KEYPRESS);
		printk("V_Up key press\n");								
    }
 
    input_sync(idev);
}

static void sf6_kybd_voldn(struct work_struct *work)
{
    struct sf6_kybd_data *kbdrec  = container_of(work, struct sf6_kybd_data, qkybd_voldn);
    struct input_dev *idev          = kbdrec->sf6_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value(kbdrec->sys_gpio_vol_dn);
    
    dev_dbg(kbdrec->sf6_kybd_pdev, "%s: Key VOLUMEDOWN (%d) %s\n", __func__, KEY_VOLUMEDOWN, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (gpio_val) {
        input_report_key(idev, KEY_VOLUMEDOWN, KEYRELEASE); 
		printk("V_Dn key release\n");				
    } else {
        input_report_key(idev, KEY_VOLUMEDOWN, KEYPRESS);
		printk("V_Dn key press\n");						
    }
 
    input_sync(idev);
}

static void sf6_kybd_camf(struct work_struct *work)
{
    struct sf6_kybd_data *kbdrec  = container_of(work, struct sf6_kybd_data, qkybd_camf);
    struct input_dev *idev          = kbdrec->sf6_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value(kbdrec->sys_gpio_cam_f);
    
    dev_dbg(kbdrec->sf6_kybd_pdev, "%s: Key FOCUS (%d) %s\n", __func__, KEY_FOCUS, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (gpio_val) {
        input_report_key(idev, KEY_FOCUS, KEYRELEASE); 
		printk("CAM F key release\n");
    } else {
        input_report_key(idev, KEY_FOCUS, KEYPRESS);
		printk("CAM F key press\n");		
    }
 
    input_sync(idev);
}

static void sf6_kybd_camt(struct work_struct *work)
{
    struct sf6_kybd_data *kbdrec  = container_of(work, struct sf6_kybd_data, qkybd_camt);
    struct input_dev *idev          = kbdrec->sf6_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value(kbdrec->sys_gpio_cam_t);

    dev_dbg(kbdrec->sf6_kybd_pdev, "%s: Key CAMERA (%d) %s\n", __func__, KEY_CAMERA, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (gpio_val) {
        input_report_key(idev, KEY_CAMERA, KEYRELEASE); 
		printk("CAM T key release\n");								
    } else {
        input_report_key(idev, KEY_CAMERA, KEYPRESS);
		printk("CAM T key press\n");								
    }
 
    input_sync(idev);
}

static void sf6_kybd_free_irq(void)
{
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_up), rd);
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_dn), rd);
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_f), rd);
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_t), rd);
}

static void sf6_kybd_flush_work(void)
{
    flush_work(&rd->qkybd_volup);
    flush_work(&rd->qkybd_voldn);
    flush_work(&rd->qkybd_camf);
    flush_work(&rd->qkybd_camt);
}

static int sf6_kybd_opencb(struct input_dev *dev)
{
    struct sf6_kybd_data *kbdrec = input_get_drvdata(dev);

    kbdrec->kybd_connected = 1; //this flag is important

    return 0;
}

static struct input_dev *create_inputdev_instance(void)
{
    struct input_dev *idev  = NULL;
    int kidx                = 0;

    idev = input_allocate_device();
    
    if (idev != NULL) {
        idev->name          = sf6_kybd_name;
        idev->open          = sf6_kybd_opencb;
        idev->keycode       = NULL;
        idev->keycodesize   = sizeof(uint8_t);
        idev->keycodemax    = 256;
        idev->evbit[0]      = BIT(EV_KEY);

        for (kidx = 1; kidx < 248; kidx++)
            __set_bit(kidx, idev->keybit);

        input_set_drvdata(idev, rd);
    } else {
        dev_err(rd->sf6_kybd_pdev, "Fail to allocate input device for %s\n",
            sf6_kybd_name);
    }
    
    return idev;
}

static void sf6_kybd_connect2inputsys(void)
{  
    rd->sf6_kybd_idev = create_inputdev_instance();

    if (rd->sf6_kybd_idev) {
        if (input_register_device(rd->sf6_kybd_idev) != 0) {
            dev_err(rd->sf6_kybd_pdev, "Fail to register input device for %s\n", sf6_kybd_name);
            
            input_free_device(rd->sf6_kybd_idev);
            rd->sf6_kybd_idev = NULL;
        }
    }
}

static int sf6_kybd_remove(struct platform_device *pdev)
{
    if (rd->sf6_kybd_idev) {
        input_unregister_device(rd->sf6_kybd_idev);
        rd->sf6_kybd_idev = NULL;
    }

    if (rd->kybd_connected) {
        rd->kybd_connected = 0;
        
        sf6_kybd_free_irq();
        sf6_kybd_flush_work();
    }
    
    sf6_kybd_release_gpio();

    kfree(rd);

    return 0;
}

static int sf6_kybd_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}

static int sf6_kybd_resume(struct platform_device *pdev)
{

    return 0;
}

#if 0
static ssize_t fbx_kybd_gpio_status_show(struct device *dev,
                    struct device_attribute *attr, char *buf)
{
    
    return sprintf(buf, "%u\n", 0);
}

static ssize_t fbx_kybd_gpio_status_store(struct device *dev, 
        struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}

static DEVICE_ATTR(gpio_status, 0644, fbx_kybd_gpio_status_show, fbx_kybd_gpio_status_store);
#endif

static int sf6_kybd_probe(struct platform_device *pdev)
{
	struct sf6_kybd_platform_data *setup_data = pdev->dev.platform_data;
	int rc = -ENOMEM;
	
	rd = kzalloc(sizeof(struct sf6_kybd_data), GFP_KERNEL);
	if (!rd) {
	    dev_err(&pdev->dev, "record memory allocation failed!!\n");
	    return rc;
	}
    
	rd->sf6_kybd_pdev					= &pdev->dev;
	rd->pmic_gpio_vol_up			= setup_data->pmic_gpio_vol_up;
	rd->pmic_gpio_vol_dn			= setup_data->pmic_gpio_vol_dn;
	rd->pmic_gpio_cam_f				= setup_data->pmic_gpio_cam_f;
	rd->pmic_gpio_cam_t				= setup_data->pmic_gpio_cam_t;
	rd->sys_gpio_vol_up				= setup_data->sys_gpio_vol_up;
	rd->sys_gpio_vol_dn				= setup_data->sys_gpio_vol_dn;
	rd->sys_gpio_cam_f				= setup_data->sys_gpio_cam_f;
	rd->sys_gpio_cam_t				= setup_data->sys_gpio_cam_t;
	
    rc = sf6_kybd_config_gpio();
    if (rc) {
        goto failexit1;
    } else {
        dev_info(&pdev->dev,
               "%s: GPIOs configured SUCCESSFULLY!!\n", __func__);
    }

    //Initialize WORKs
    INIT_WORK(&rd->qkybd_volup, sf6_kybd_volup);
    INIT_WORK(&rd->qkybd_voldn, sf6_kybd_voldn);
    INIT_WORK(&rd->qkybd_camf, sf6_kybd_camf);
    INIT_WORK(&rd->qkybd_camt, sf6_kybd_camt);
	
    //Request IRQs
    rc = sf6_kybd_irqsetup();
    if (rc)  
        goto failexit2;

    //Register Input Device to Input Subsystem
    sf6_kybd_connect2inputsys();
    if (rd->sf6_kybd_idev == NULL)
        goto failexit2;
        
//    rc = device_create_file(&pdev->dev, &dev_attr_gpio_status);
//    if (rc) {
//        dev_err(&pdev->dev,
//               "%s: dev_attr_gpio_status failed\n", __func__);
//    }
    
    return 0;

failexit2:
    sf6_kybd_free_irq();
    sf6_kybd_flush_work();
    
failexit1:
    sf6_kybd_release_gpio();
    kfree(rd);

    return rc;
}

static struct platform_driver sf6_kybd_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = sf6_kybd_name,
    },
    .probe    = sf6_kybd_probe,
    .remove   = __devexit_p(sf6_kybd_remove),
    .suspend  = sf6_kybd_suspend,
    .resume   = sf6_kybd_resume,
};

static int __init sf6_kybd_init(void)
{
    return platform_driver_register(&sf6_kybd_driver);
}

static void __exit sf6_kybd_exit(void)
{
    platform_driver_unregister(&sf6_kybd_driver);
}

module_init(sf6_kybd_init);
module_exit(sf6_kybd_exit);
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("sf6 keypad driver");
MODULE_LICENSE("GPL v2");
