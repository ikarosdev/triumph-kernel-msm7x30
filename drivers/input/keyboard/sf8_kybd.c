#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/mfd/pmic8058.h> /* for controlling pmic8058 GPIO */
#include <mach/gpio.h>
#include <mach/sf8_kybd.h>
#include <mach/pmic.h>
#include <mach/irqs.h>
#include <mach/rpc_server_handset.h>
#include <linux/reboot.h> // for call emergency_restart
#include "../../../arch/arm/mach-msm/smd_private.h" // for get HWID


#define sf8_kybd_name       "sf8_kybd"

#define KBD_DEBOUNCE_TIME   8
#define KEYRELEASE			0
#define KEYPRESS			1

struct sf8_kybd_data {
    struct input_dev    *sf8_kybd_idev;
    struct device       *sf8_kybd_pdev;
    
    struct work_struct qkybd_volup;
    struct work_struct qkybd_voldn;
    struct work_struct qkybd_coverdet;    

    unsigned int pmic_gpio_vol_up;
    unsigned int pmic_gpio_vol_dn;
    unsigned int pmic_gpio_cover_det;    
    unsigned int sys_gpio_vol_up;
    unsigned int sys_gpio_vol_dn;
	unsigned int sys_gpio_cover_det;
		
    bool kybd_connected;
};

struct sf8_kybd_data *rd = NULL;

static irqreturn_t sf8_kybd_irqhandler(int irq, void *dev_id)
{
	struct sf8_kybd_data *kbdrec = (struct sf8_kybd_data *) dev_id;

	if (kbdrec->kybd_connected) {
		if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_vol_up) == irq) {
			schedule_work(&kbdrec->qkybd_volup);
		} 
		else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_vol_dn) == irq) {
			schedule_work(&kbdrec->qkybd_voldn);
		}
		else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_cover_det) == irq) {
			schedule_work(&kbdrec->qkybd_coverdet);
		}
	}  
	return IRQ_HANDLED;
}

static int sf8_kybd_irqsetup(void)
{
    int rc;
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_up), &sf8_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 sf8_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->sf8_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", sf8_kybd_name, rd->pmic_gpio_vol_up, rc);
        rc = -EIO;
    }
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_dn), &sf8_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 sf8_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->sf8_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", sf8_kybd_name, rd->pmic_gpio_vol_dn, rc);
        rc = -EIO;
    }

    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cover_det), &sf8_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 sf8_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->sf8_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", sf8_kybd_name, rd->pmic_gpio_cover_det, rc);
        rc = -EIO;
    }    
    return rc;
}

static void sf8_kybd_release_gpio(void)
{
    gpio_free(rd->sys_gpio_vol_up);
    gpio_free(rd->sys_gpio_vol_dn);
    gpio_free(rd->sys_gpio_cover_det);    
}

static int sf8_kybd_config_gpio(void)
{
    struct pm8058_gpio button_configuration = {
        .direction      = PM_GPIO_DIR_IN,
        //.pull           = PM_GPIO_PULL_NO,
        .pull           = PM_GPIO_PULL_UP_31P5,
        .vin_sel        = 2,
        .out_strength   = PM_GPIO_STRENGTH_NO,
        .function       = PM_GPIO_FUNC_NORMAL,
        .inv_int_pol    = 0,
    };

    int rc = 0;

    printk(KERN_INFO "%s set pull up\n ", __func__);

    rc = pm8058_gpio_config(rd->pmic_gpio_vol_up, &button_configuration);
    if (rc < 0) {
        dev_err(rd->sf8_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_up);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_vol_up, "vol_up_key");
        if (rc < 0) {
            dev_err(rd->sf8_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_vol_up);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_vol_up);
            if (rc < 0) {
                dev_err(rd->sf8_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_up);
                return rc;
            }
        }
    }
    
    rc = pm8058_gpio_config(rd->pmic_gpio_vol_dn, &button_configuration);
    if (rc < 0) {
        dev_err(rd->sf8_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_dn);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_vol_dn, "vol_down_key");
        if (rc < 0) {
            dev_err(rd->sf8_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_vol_dn);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_vol_dn);
            if (rc < 0) {
                dev_err(rd->sf8_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_dn);
                return rc;
            }
        }
    }

    rc = pm8058_gpio_config(rd->pmic_gpio_cover_det, &button_configuration);
    if (rc < 0) {
        dev_err(rd->sf8_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cover_det);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_cover_det, "cover_detect_key");
        if (rc < 0) {
            dev_err(rd->sf8_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_cover_det);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_cover_det);
            if (rc < 0) {
                dev_err(rd->sf8_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cover_det);
                return rc;
            }
        }
    }    
    return rc;
}

static void sf8_kybd_volup(struct work_struct *work)
{
    struct sf8_kybd_data *kbdrec  = container_of(work, struct sf8_kybd_data, qkybd_volup);
    struct input_dev *idev          = kbdrec->sf8_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value(kbdrec->sys_gpio_vol_up);
    
    dev_dbg(kbdrec->sf8_kybd_pdev, "%s: Key VOLUMEUP (%d) %s\n", __func__, KEY_VOLUMEUP, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (gpio_val) {
        input_report_key(idev, KEY_VOLUMEUP, KEYRELEASE); 
		printk("V_Up key release\n");						
    } else {
        input_report_key(idev, KEY_VOLUMEUP, KEYPRESS);
		printk("V_Up key press\n");								
    }
 
    input_sync(idev);
}

static void sf8_kybd_voldn(struct work_struct *work)
{
    struct sf8_kybd_data *kbdrec  = container_of(work, struct sf8_kybd_data, qkybd_voldn);
    struct input_dev *idev          = kbdrec->sf8_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value(kbdrec->sys_gpio_vol_dn);
    
    dev_dbg(kbdrec->sf8_kybd_pdev, "%s: Key VOLUMEDOWN (%d) %s\n", __func__, KEY_VOLUMEDOWN, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (gpio_val) {
        input_report_key(idev, KEY_VOLUMEDOWN, KEYRELEASE); 
		printk("V_Dn key release\n");				
    } else {
        input_report_key(idev, KEY_VOLUMEDOWN, KEYPRESS);
		printk("V_Dn key press\n");						
    }
 
    input_sync(idev);
}

static void sf8_kybd_coverdet(struct work_struct *work)
{
    struct sf8_kybd_data *kbdrec  = container_of(work, struct sf8_kybd_data, qkybd_coverdet);
    struct input_dev *idev        = kbdrec->sf8_kybd_idev;
    bool gpio_val                 = (bool)gpio_get_value(kbdrec->sys_gpio_cover_det);
    
    dev_dbg(kbdrec->sf8_kybd_pdev, "%s: Key COVER DETECT (%d) %s\n", __func__, KEY_COVERDET, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (gpio_val)
	{
		input_report_key(idev, KEY_COVERDET, KEYRELEASE); 
		printk("Cover detect key release in SF8 %d phase.\n", fih_get_product_phase());
		if (fih_get_product_phase()==Product_PR1 || fih_get_product_phase()==Product_PR1p5)
		{
			#ifdef CONFIG_KEYBOARD_SF4H8_COVERDET
	 		printk("Cover detect key release then execute emergency_restart.\n");			
	 		emergency_restart();
			#endif
		}
		else
		{
		 	printk("Cover detect key release then execute shutdown.\n");				
		}
    } 
	else 
    {
        input_report_key(idev, KEY_COVERDET, KEYPRESS);
	 	printk("Cover detect key press\n");						
    }
 
    input_sync(idev);
}

static void sf8_kybd_free_irq(void)
{
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_up), rd);
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_dn), rd);
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cover_det), rd);    
}

static void sf8_kybd_flush_work(void)
{
    flush_work(&rd->qkybd_volup);
    flush_work(&rd->qkybd_voldn);
    flush_work(&rd->qkybd_coverdet);    
}

static int sf8_kybd_opencb(struct input_dev *dev)
{
    struct sf8_kybd_data *kbdrec = input_get_drvdata(dev);

    kbdrec->kybd_connected = 1; //this flag is important

    return 0;
}

static struct input_dev *create_inputdev_instance(void)
{
    struct input_dev *idev  = NULL;
    int kidx                = 0;

    idev = input_allocate_device();
    
    if (idev != NULL) {
        idev->name          = sf8_kybd_name;
        idev->open          = sf8_kybd_opencb;
        idev->keycode       = NULL;
        idev->keycodesize   = sizeof(uint8_t);
        idev->keycodemax    = 256;
        idev->evbit[0]      = BIT(EV_KEY);

        for (kidx = 1; kidx < 248; kidx++)
            __set_bit(kidx, idev->keybit);

        input_set_drvdata(idev, rd);
    } else {
        dev_err(rd->sf8_kybd_pdev, "Fail to allocate input device for %s\n",
            sf8_kybd_name);
    }
    
    return idev;
}

static void sf8_kybd_connect2inputsys(void)
{  
    rd->sf8_kybd_idev = create_inputdev_instance();

    if (rd->sf8_kybd_idev) {
        if (input_register_device(rd->sf8_kybd_idev) != 0) {
            dev_err(rd->sf8_kybd_pdev, "Fail to register input device for %s\n", sf8_kybd_name);
            
            input_free_device(rd->sf8_kybd_idev);
            rd->sf8_kybd_idev = NULL;
        }
    }
}

static int sf8_kybd_remove(struct platform_device *pdev)
{
    if (rd->sf8_kybd_idev) {
        input_unregister_device(rd->sf8_kybd_idev);
        rd->sf8_kybd_idev = NULL;
    }

    if (rd->kybd_connected) {
        rd->kybd_connected = 0;
        
        sf8_kybd_free_irq();
        sf8_kybd_flush_work();
    }
    
    sf8_kybd_release_gpio();

    kfree(rd);

    return 0;
}

static int sf8_kybd_suspend(struct platform_device *pdev, pm_message_t state)
{

	printk(KERN_INFO "%s cover detect key enable_irq_wake.\n",  __func__);
	enable_irq_wake(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cover_det));
		
	return 0;
}

static int sf8_kybd_resume(struct platform_device *pdev)
{
    return 0;
}

static int sf8_kybd_probe(struct platform_device *pdev)
{
	struct sf8_kybd_platform_data *setup_data = pdev->dev.platform_data;
	int rc = -ENOMEM;
	
	rd = kzalloc(sizeof(struct sf8_kybd_data), GFP_KERNEL);
	if (!rd) {
	    dev_err(&pdev->dev, "record memory allocation failed!!\n");
	    return rc;
	}
    
	rd->sf8_kybd_pdev			= &pdev->dev;
	rd->pmic_gpio_vol_up		= setup_data->pmic_gpio_vol_up;
	rd->pmic_gpio_vol_dn		= setup_data->pmic_gpio_vol_dn;
	rd->pmic_gpio_cover_det		= setup_data->pmic_gpio_cover_det;

	rd->sys_gpio_vol_up			= setup_data->sys_gpio_vol_up;
	rd->sys_gpio_vol_dn			= setup_data->sys_gpio_vol_dn;
	rd->sys_gpio_cover_det		= setup_data->sys_gpio_cover_det;
		
    rc = sf8_kybd_config_gpio();
    if (rc) {
        goto failexit1;
    } else {
        dev_info(&pdev->dev,
               "%s: GPIOs configured SUCCESSFULLY!!\n", __func__);
    }

    //Initialize WORKs
    INIT_WORK(&rd->qkybd_volup, sf8_kybd_volup);
    INIT_WORK(&rd->qkybd_voldn, sf8_kybd_voldn);
    INIT_WORK(&rd->qkybd_coverdet, sf8_kybd_coverdet);
    	
    //Request IRQs
    rc = sf8_kybd_irqsetup();
    if (rc)  
        goto failexit2;

    //Register Input Device to Input Subsystem
    sf8_kybd_connect2inputsys();
    if (rd->sf8_kybd_idev == NULL)
        goto failexit2;
    
    return 0;

failexit2:
    sf8_kybd_free_irq();
    sf8_kybd_flush_work();
    
failexit1:
    sf8_kybd_release_gpio();
    kfree(rd);

    return rc;
}

static struct platform_driver sf8_kybd_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = sf8_kybd_name,
    },
    .probe    = sf8_kybd_probe,
    .remove   = __devexit_p(sf8_kybd_remove),
    .suspend  = sf8_kybd_suspend,
    .resume   = sf8_kybd_resume,
};

static int __init sf8_kybd_init(void)
{
    return platform_driver_register(&sf8_kybd_driver);
}

static void __exit sf8_kybd_exit(void)
{
    platform_driver_unregister(&sf8_kybd_driver);
}

module_init(sf8_kybd_init);
module_exit(sf8_kybd_exit);
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("sf8 keypad driver");
MODULE_LICENSE("GPL v2");
