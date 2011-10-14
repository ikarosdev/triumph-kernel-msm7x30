#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/mfd/pmic8058.h> /* for controlling pmic8058 GPIO */
#include <mach/gpio.h>
#include <mach/fbx_kybd.h>
#include <mach/pmic.h>
#include <mach/irqs.h>
#include "../../../arch/arm/mach-msm/smd_private.h"
/* Div1-FW3-BSP-AUDIO */
#include <mach/rpc_server_handset.h>

#define fbx_kybd_name       "fbx_kybd"
#define KEY_FOCUS           KEY_F13
#define KBD_DEBOUNCE_TIME   8
#define KEYRELEASE          0
#define KEYPRESS            1

/* Div1-FW3-BSP-AUDIO */
#define HS_HEADSET_SWITCH_K 0x84
#define HS_REL_K        0xFF 
#define HS_NONE_K 0x00

struct workqueue_struct *headset_hook_wq;

struct fbx_kybd_record {
    struct input_dev    *fbx_kybd_idev;
    struct device       *fbx_kybd_pdev;
    
    struct work_struct qkybd_volup;
    struct work_struct qkybd_voldn;
    struct work_struct qkybd_camf;
    struct work_struct qkybd_camt;
    struct work_struct hook_switchkey; /* Div1-FW3-BSP-AUDIO */
    
    unsigned int pmic_gpio_cam_f;
    unsigned int pmic_gpio_cam_t;
    unsigned int pmic_gpio_vol_up;
    unsigned int pmic_gpio_vol_dn;
    unsigned int sys_gpio_cam_f;
    unsigned int sys_gpio_cam_t;
    unsigned int sys_gpio_vol_up;
    unsigned int sys_gpio_vol_dn;
    unsigned int hook_sw_pin; /* Div1-FW3-BSP-AUDIO */
    
    int product_phase;
    
    bool kybd_connected;
    bool hooksw_irq_enable; /* Div1-FW3-BSP-AUDIO */
};

struct fbx_kybd_record *rd = NULL;

static irqreturn_t fbx_kybd_irqhandler(int irq, void *dev_id)
{
    struct fbx_kybd_record *kbdrec = (struct fbx_kybd_record *) dev_id;

    if (kbdrec->kybd_connected) {
        if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_vol_up) == irq)
            schedule_work(&kbdrec->qkybd_volup);
        else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_vol_dn) == irq)    
            schedule_work(&kbdrec->qkybd_voldn);
        /* Div1-FW3-BSP-AUDIO */
        else if (MSM_GPIO_TO_INT(kbdrec->hook_sw_pin) == irq){ 
            queue_work(headset_hook_wq, &kbdrec->hook_switchkey);
            set_irq_type(MSM_GPIO_TO_INT(rd->hook_sw_pin), 
                gpio_get_value_cansleep(rd->hook_sw_pin) ?
                    IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
        } else if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
            if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_cam_f) == irq)
                schedule_work(&kbdrec->qkybd_camf); 
            else if (PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, kbdrec->pmic_gpio_cam_t) == irq)
                schedule_work(&kbdrec->qkybd_camt);
        }
    }
    
    return IRQ_HANDLED;
}

static int fbx_kybd_irqsetup(void)
{
    int rc;
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_up), &fbx_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 fbx_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->fbx_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", fbx_kybd_name, rd->pmic_gpio_vol_up, rc);
        rc = -EIO;
    }
    
    rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_dn), &fbx_kybd_irqhandler,
                 (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                 fbx_kybd_name, rd);
    if (rc < 0) {
        dev_err(rd->fbx_kybd_pdev,
               "Could not register for  %s interrupt %d"
               "(rc = %d)\n", fbx_kybd_name, rd->pmic_gpio_vol_dn, rc);
        rc = -EIO;
    }

    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_f), &fbx_kybd_irqhandler,
                     (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                     fbx_kybd_name, rd);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev,
                   "Could not register for  %s interrupt %d"
                   "(rc = %d)\n", fbx_kybd_name, rd->pmic_gpio_cam_f, rc);
            rc = -EIO;
        }

        rc = request_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_t), &fbx_kybd_irqhandler,
                     (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
                     fbx_kybd_name, rd);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev,
                   "Could not register for  %s interrupt %d"
                   "(rc = %d)\n", fbx_kybd_name, rd->pmic_gpio_cam_t, rc);
            rc = -EIO;
        }
    }
        
    return rc;
}

/* Div1-FW3-BSP-AUDIO */
int fbx_hookswitch_irqsetup(bool activate_irq)
{
    int rc = 0;
    
    printk(KERN_INFO "fbx_hookswitch_irqsetup \n"); 

    if ( activate_irq && (gpio_get_value(rd->hook_sw_pin) == 1)) {
        rc = request_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), &fbx_kybd_irqhandler,
                    IRQF_TRIGGER_LOW, fbx_kybd_name, rd);
        if (rc < 0) {
            printk("Could not register for  %s interrupt(rc = %d) \n",fbx_kybd_name, rc);
            rc = -EIO;
        }
        printk("Hook Switch IRQ Enable!\n");
        rd->hooksw_irq_enable = true;
    } else {
        if (rd->hooksw_irq_enable){
            free_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), rd);
            printk("Hook Switch IRQ disable\n");
            rd->hooksw_irq_enable = false;
        }
    }
    return rc;
}

static void fbx_kybd_release_gpio(void)
{
    gpio_free(rd->sys_gpio_vol_up);
    gpio_free(rd->sys_gpio_vol_dn);
    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        gpio_free(rd->sys_gpio_cam_t);
        gpio_free(rd->sys_gpio_cam_f);
    }
    /* Div1-FW3-BSP-AUDIO */
    gpio_free(rd->hook_sw_pin);
}

static int fbx_kybd_config_gpio(void)
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
        dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_up);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_vol_up, "vol_up_key");
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_vol_up);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_vol_up);
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_up);
                return rc;
            }
        }
    }
    
    rc = pm8058_gpio_config(rd->pmic_gpio_vol_dn, &button_configuration);
    if (rc < 0) {
        dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_dn);
        return rc;
    } else {
        rc = gpio_request(rd->sys_gpio_vol_dn, "vol_down_key");
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_vol_dn);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_vol_dn);
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_dn);
                return rc;
            }
        }
    }
 
    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        rc = pm8058_gpio_config(rd->pmic_gpio_cam_t, &button_configuration);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_t);
            return rc;
        } else {
            rc = gpio_request(rd->sys_gpio_cam_t, "camera_key");
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_cam_t);
                return rc;
            } else {
                rc = gpio_direction_input(rd->sys_gpio_cam_t);
                if (rc < 0) {
                    dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_t);
                    return rc;
                }
            }
        }
        
        rc = pm8058_gpio_config(rd->pmic_gpio_cam_f, &button_configuration);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_f);
            return rc;
        } else {
            rc = gpio_request(rd->sys_gpio_cam_f, "camera_focus_key");
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: gpio %d requested failed\n", __func__, rd->sys_gpio_cam_f);
                return rc;
            } else {
                rc = gpio_direction_input(rd->sys_gpio_cam_f); 
                if (rc < 0) {
                    dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_f);
                    return rc;
                }
            }
        }
    }
    
    /* Div1-FW3-BSP-AUDIO */
    rc = gpio_request(rd->hook_sw_pin, "gpio_hook_sw");
    if (rc) {
        printk("gpio_request failed on pin(rc=%d)\n", rc);
        return rc;
    }
    rc = gpio_direction_input(rd->hook_sw_pin);
    if (rc) {
        printk("gpio_direction_input failed on pin rc=%d\n", rc);
        return rc;
    }
    
    return rc;
}

static void fbx_kybd_volup(struct work_struct *work)
{
    struct fbx_kybd_record *kbdrec  = container_of(work, struct fbx_kybd_record, qkybd_volup);
    struct input_dev *idev          = kbdrec->fbx_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value_cansleep(kbdrec->sys_gpio_vol_up);
    
    dev_dbg(kbdrec->fbx_kybd_pdev, "%s: Key VOLUMEUP (%d) %s\n", __func__, KEY_VOLUMEUP, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (!gpio_val) {
        input_report_key(idev, KEY_VOLUMEUP, KEYRELEASE); 
    } else {
        input_report_key(idev, KEY_VOLUMEUP, KEYPRESS);
    }
 
    input_sync(idev);
}

static void fbx_kybd_voldn(struct work_struct *work)
{
    struct fbx_kybd_record *kbdrec  = container_of(work, struct fbx_kybd_record, qkybd_voldn);
    struct input_dev *idev          = kbdrec->fbx_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value_cansleep(kbdrec->sys_gpio_vol_dn);
    
    dev_dbg(kbdrec->fbx_kybd_pdev, "%s: Key VOLUMEDOWN (%d) %s\n", __func__, KEY_VOLUMEDOWN, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (!gpio_val) {
        input_report_key(idev, KEY_VOLUMEDOWN, KEYRELEASE); 
    } else {
        input_report_key(idev, KEY_VOLUMEDOWN, KEYPRESS);
    }
 
    input_sync(idev);
}

static void fbx_kybd_camf(struct work_struct *work)
{
    struct fbx_kybd_record *kbdrec  = container_of(work, struct fbx_kybd_record, qkybd_camf);
    struct input_dev *idev          = kbdrec->fbx_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value_cansleep(kbdrec->sys_gpio_cam_f);
    
    dev_dbg(kbdrec->fbx_kybd_pdev, "%s: Key FOCUS (%d) %s\n", __func__, KEY_FOCUS, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (!gpio_val) {
        input_report_key(idev, KEY_FOCUS, KEYRELEASE); 
    } else {
        input_report_key(idev, KEY_FOCUS, KEYPRESS);
    }
 
    input_sync(idev);
}

static void fbx_kybd_camt(struct work_struct *work)
{
    struct fbx_kybd_record *kbdrec  = container_of(work, struct fbx_kybd_record, qkybd_camt);
    struct input_dev *idev          = kbdrec->fbx_kybd_idev;
    bool gpio_val                   = (bool)gpio_get_value_cansleep(kbdrec->sys_gpio_cam_t);

    dev_dbg(kbdrec->fbx_kybd_pdev, "%s: Key CAMERA (%d) %s\n", __func__, KEY_CAMERA, !gpio_val ? "was RELEASED" : "is PRESSING");

    if (!gpio_val) {
        input_report_key(idev, KEY_CAMERA, KEYRELEASE); 
    } else {
        input_report_key(idev, KEY_CAMERA, KEYPRESS);
    }
 
    input_sync(idev);
}

/* Div1-FW3-BSP-AUDIO */
static void fbx_kybd_hooksw(struct work_struct *work)
{
    //MM-SL-DisableHookInFTM-oo*{
    #ifndef CONFIG_FIH_FTM  
    struct fbx_kybd_record *kbdrec= container_of(work, struct fbx_kybd_record, hook_switchkey);  
    struct input_dev *idev        = kbdrec->fbx_kybd_idev;  
    bool hook_sw_val              = (bool)gpio_get_value_cansleep(kbdrec->hook_sw_pin);
    bool headset_status           = (bool)gpio_get_value_cansleep(26);


    if((rd->hooksw_irq_enable == true) && (headset_status == 1)){
        disable_irq(MSM_GPIO_TO_INT(kbdrec->hook_sw_pin));
        if(idev)
        {
            if (hook_sw_val) {
                report_hs_key(HS_HEADSET_SWITCH_K, HS_REL_K);
                printk("FIH: keyrelease KEY_HEADSETHOOK\n");
            } else {
                report_hs_key(HS_HEADSET_SWITCH_K, HS_NONE_K);
                printk("FIH: keypress KEY_HEADSETHOOK\n");              
            }
            input_sync(idev);       
        }
        enable_irq(MSM_GPIO_TO_INT(kbdrec->hook_sw_pin));
        enable_irq_wake(MSM_GPIO_TO_INT(kbdrec->hook_sw_pin));
    }
    #endif
    //MM-SL-DisableHookInFTM-oo*} 	
}

static void fbx_kybd_free_irq(void)
{
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_up), rd);
    free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_vol_dn), rd);
    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_f), rd);
        free_irq(PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, rd->pmic_gpio_cam_t), rd);
    }
    /* Div1-FW3-BSP-AUDIO */
    free_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), rd);
}

static void fbx_kybd_flush_work(void)
{
    flush_work(&rd->qkybd_volup);
    flush_work(&rd->qkybd_voldn);
    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        flush_work(&rd->qkybd_camf);
        flush_work(&rd->qkybd_camt);
    }
    /* Div1-FW3-BSP-AUDIO */
    flush_work(&rd->hook_switchkey);    
}

static int fbx_kybd_opencb(struct input_dev *dev)
{
    struct fbx_kybd_record *kbdrec = input_get_drvdata(dev);

    kbdrec->kybd_connected = 1; //this flag is important

    return 0;
}

static struct input_dev *create_inputdev_instance(void)
{
    struct input_dev *idev  = NULL;
    //int kidx                = 0;

    idev = input_allocate_device();
    
    if (idev != NULL) {
        idev->name          = fbx_kybd_name;
        idev->open          = fbx_kybd_opencb;
        idev->keycode       = NULL;
        idev->keycodesize   = sizeof(uint8_t);
        idev->keycodemax    = 256;
        idev->evbit[0]      = BIT(EV_KEY);

        /*for (kidx = 1; kidx < 248; kidx++)
            __set_bit(kidx, idev->keybit);*/
        __set_bit(KEY_VOLUMEUP, idev->keybit);
        __set_bit(KEY_VOLUMEDOWN, idev->keybit);
        if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
            __set_bit(KEY_CAMERA, idev->keybit);
            __set_bit(KEY_FOCUS, idev->keybit);
        }
        
        input_set_drvdata(idev, rd);
    } else {
        dev_err(rd->fbx_kybd_pdev, "Fail to allocate input device for %s\n",
            fbx_kybd_name);
    }
    
    return idev;
}

static void fbx_kybd_connect2inputsys(void)
{  
    rd->fbx_kybd_idev = create_inputdev_instance();

    if (rd->fbx_kybd_idev) {
        if (input_register_device(rd->fbx_kybd_idev) != 0) {
            dev_err(rd->fbx_kybd_pdev, "Fail to register input device for %s\n", fbx_kybd_name);
            
            input_free_device(rd->fbx_kybd_idev);
            rd->fbx_kybd_idev = NULL;
        }
    }
}

static int fbx_kybd_remove(struct platform_device *pdev)
{
    if (rd->fbx_kybd_idev) {
        input_unregister_device(rd->fbx_kybd_idev);
        rd->fbx_kybd_idev = NULL;
    }

    if (rd->kybd_connected) {
        rd->kybd_connected = 0;
        
        fbx_kybd_free_irq();
        fbx_kybd_flush_work();
    }
    
    fbx_kybd_release_gpio();

    kfree(rd);

    return 0;
}

static int fbx_kybd_suspend(struct platform_device *pdev, pm_message_t state)
{
    struct pm8058_gpio button_configuration = {
        .direction      = PM_GPIO_DIR_OUT,
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
        dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_up);
        return rc;
    } else {
        rc = gpio_direction_output(rd->sys_gpio_vol_up, 0);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_up);
            return rc;
        }
    }
    
    rc = pm8058_gpio_config(rd->pmic_gpio_vol_dn, &button_configuration);
    if (rc < 0) {
        dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_dn);
        return rc;
    } else {
        rc = gpio_direction_output(rd->sys_gpio_vol_dn, 0);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_dn);
            return rc;
        }
    }
 
    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        rc = pm8058_gpio_config(rd->pmic_gpio_cam_t, &button_configuration);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_t);
            return rc;
        } else {
            rc = gpio_direction_output(rd->sys_gpio_cam_t, 0);
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_t);
                return rc;
            }
        }
        
        rc = pm8058_gpio_config(rd->pmic_gpio_cam_f, &button_configuration);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_f);
            return rc;
        } else {
            rc = gpio_direction_output(rd->sys_gpio_cam_f, 0); 
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_f);
                return rc;
            }
        }
    }
 
    return 0;
}

static int fbx_kybd_resume(struct platform_device *pdev)
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
        dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_up);
        return rc;
    } else {
        rc = gpio_direction_input(rd->sys_gpio_vol_up);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_up);
            return rc;
        }
    }
    
    rc = pm8058_gpio_config(rd->pmic_gpio_vol_dn, &button_configuration);
    if (rc < 0) {
        dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_vol_dn);
        return rc;
    } else {
        rc = gpio_direction_input(rd->sys_gpio_vol_dn);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_vol_dn);
            return rc;
        }
    }

    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        rc = pm8058_gpio_config(rd->pmic_gpio_cam_t, &button_configuration);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_t);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_cam_t);
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_t);
                return rc;
            }
        }
        
        rc = pm8058_gpio_config(rd->pmic_gpio_cam_f, &button_configuration);
        if (rc < 0) {
            dev_err(rd->fbx_kybd_pdev, "%s: gpio %d configured failed\n", __func__, rd->pmic_gpio_cam_f);
            return rc;
        } else {
            rc = gpio_direction_input(rd->sys_gpio_cam_f); 
            if (rc < 0) {
                dev_err(rd->fbx_kybd_pdev, "%s: set gpio %d direction failed\n", __func__, rd->sys_gpio_cam_f);
                return rc;
            }
        }
    }
    
    return 0;
}

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

static int fbx_kybd_probe(struct platform_device *pdev)
{
    struct fbx_kybd_platform_data *setup_data = pdev->dev.platform_data;
    int rc = -ENOMEM;

    rd = kzalloc(sizeof(struct fbx_kybd_record), GFP_KERNEL);
    if (!rd) {
        dev_err(&pdev->dev, "record memory allocation failed!!\n");
        return rc;
    }
    
    rd->product_phase           = fih_get_product_phase();
    rd->fbx_kybd_pdev           = &pdev->dev;
    rd->pmic_gpio_vol_up        = setup_data->pmic_gpio_vol_up;
    rd->pmic_gpio_vol_dn        = setup_data->pmic_gpio_vol_dn;
    rd->sys_gpio_vol_up         = setup_data->sys_gpio_vol_up;
    rd->sys_gpio_vol_dn         = setup_data->sys_gpio_vol_dn;
    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        rd->pmic_gpio_cam_f = setup_data->pmic_gpio_cam_f;
        rd->pmic_gpio_cam_t = setup_data->pmic_gpio_cam_t;
        rd->sys_gpio_cam_f  = setup_data->sys_gpio_cam_f;
        rd->sys_gpio_cam_t  = setup_data->sys_gpio_cam_t;
    }
    /* Div1-FW3-BSP-AUDIO */
    rd->hook_sw_pin             = setup_data->hook_sw_pin;
    rd->hooksw_irq_enable = false;
    headset_hook_wq = create_singlethread_workqueue("headset_hook");
    if (!headset_hook_wq) {
        printk("%s: create workque failed \n", __func__);       
        return -EBUSY;
    }
    
    //Request GPIOs
    rc = fbx_kybd_config_gpio();
    if (rc) {
        goto failexit1;
    } else {
        dev_info(&pdev->dev,
               "%s: GPIOs configured SUCCESSFULLY!!\n", __func__);
    }

    //Initialize WORKs
    INIT_WORK(&rd->qkybd_volup, fbx_kybd_volup);
    INIT_WORK(&rd->qkybd_voldn, fbx_kybd_voldn);
    if (rd->product_phase == Product_PR1 || rd->product_phase == Product_EVB) {
        INIT_WORK(&rd->qkybd_camf, fbx_kybd_camf);
        INIT_WORK(&rd->qkybd_camt, fbx_kybd_camt);
    }
    /* Div1-FW3-BSP-AUDIO */
    INIT_WORK(&rd->hook_switchkey, fbx_kybd_hooksw);
    
    //Request IRQs
    rc = fbx_kybd_irqsetup();
    if (rc)  
        goto failexit2;

    //Register Input Device to Input Subsystem
    fbx_kybd_connect2inputsys();
    if (rd->fbx_kybd_idev == NULL)
        goto failexit2;
        
    rc = device_create_file(&pdev->dev, &dev_attr_gpio_status);
    if (rc) {
        dev_err(&pdev->dev,
               "%s: dev_attr_gpio_status failed\n", __func__);
    }
    
    return 0;

failexit2:
    fbx_kybd_free_irq();
    fbx_kybd_flush_work();
    
failexit1:
    fbx_kybd_release_gpio();
    kfree(rd);

    return rc;
}

static struct platform_driver fbx_kybd_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = fbx_kybd_name,
    },
    .probe    = fbx_kybd_probe,
    .remove   = __devexit_p(fbx_kybd_remove),
    .suspend  = fbx_kybd_suspend,
    .resume   = fbx_kybd_resume,
};

static int __init fbx_kybd_init(void)
{
    return platform_driver_register(&fbx_kybd_driver);
}

static void __exit fbx_kybd_exit(void)
{
    platform_driver_unregister(&fbx_kybd_driver);
}

module_init(fbx_kybd_init);
module_exit(fbx_kybd_exit);
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("fbx button driver");
MODULE_LICENSE("GPL v2");
