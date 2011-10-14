/*
 *  drivers/switch/switch_gpio.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>

/* Div1-FW3-BSP-AUDIO */
#ifdef CONFIG_FIH_FBX_AUDIO
#include <linux/delay.h> 
#include <linux/input.h>
#include <linux/irq.h>
#include <mach/fbx_kybd.h>
#include <mach/pmic.h>
#include <linux/earlysuspend.h>  //MM-SL-HS WAKE UP IN SUSPEND-00+
#endif

struct gpio_switch_data {
	struct switch_dev sdev;
	//MM-SL-HS WAKE UP IN SUSPEND-00+{
	#ifdef CONFIG_FIH_FBX_AUDIO
	struct input_dev *hs_input;
	#endif
	//MM-SL-HS WAKE UP IN SUSPEND-00+}
	unsigned gpio;
	const char *name_on;
	const char *name_off;
	const char *state_on;
	const char *state_off;
	int irq;
	struct work_struct work;
	//MM-SL-HS WAKE UP IN SUSPEND-00+{	
	#ifdef CONFIG_FIH_FBX_AUDIO
	#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend es;
	#endif
	#endif
	//MM-SL-HS WAKE UP IN SUSPEND-00+}	
};


/* Div1-FW3-BSP-AUDIO */
#ifdef CONFIG_FIH_FBX_AUDIO
struct workqueue_struct *headset_switch_wq;
bool headset_without_mic = 0;
bool mSuspend = false; //MM-SL-HS WAKE UP IN SUSPEND-00+
#define KEY_RINGSWITCH                  184 //MM-SL-HS WAKE UP IN SUSPEND-00+

void hook_switch_enable(bool bHookSwitchEnabled)
{
	if(bHookSwitchEnabled) {
		msleep(300);
		headset_without_mic = (gpio_get_value_cansleep(44) == 0)? true: false; 
		printk("Headset: Enable_Headset_Without_Mic == %d\n", headset_without_mic);
		
		if(headset_without_mic == true){
			fbx_hookswitch_irqsetup(false);
			pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_OFF);
		}else{
		    fbx_hookswitch_irqsetup(bHookSwitchEnabled);
		}
	}else {
        if( headset_without_mic == false ){
            fbx_hookswitch_irqsetup(bHookSwitchEnabled);
        }else{
            headset_without_mic = false;
			fbx_hookswitch_irqsetup(false);
        }
	}
	printk("Headset: hook_switch_enabled = %d\n", bHookSwitchEnabled);
}
#endif

static void gpio_switch_work(struct work_struct *work)
{
	int state;
	struct gpio_switch_data	*data =
		container_of(work, struct gpio_switch_data, work);

	state = gpio_get_value_cansleep(data->gpio);
    #ifdef CONFIG_FIH_FBX_AUDIO
	disable_irq(data->irq);

	if (state == 0){
		printk("Headset plug out \n");
		hook_switch_enable(false);
		pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_OFF);
		switch_set_state(&data->sdev, state);
		//MM-SL-HS WAKE UP IN SUSPEND-00+{
		if (mSuspend){
			input_report_key(data->hs_input, KEY_RINGSWITCH, 1);  
			printk("hs_in: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			

			input_report_key(data->hs_input, KEY_RINGSWITCH, 0);	 	
			printk("hs_in: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			
		}
		input_sync(data->hs_input);
		//MM-SL-HS WAKE UP IN SUSPEND-00+}
	}else{
		printk("Headset plug in \n");
		//MM-SL-HS WAKE UP IN SUSPEND-00+{
		if (mSuspend){
			input_report_key(data->hs_input, KEY_RINGSWITCH, 1);  
			printk("hs_out: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			

			input_report_key(data->hs_input, KEY_RINGSWITCH, 0);	 	
			printk("hs_out: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			
		}
		input_sync(data->hs_input);
		//MM-SL-HS WAKE UP IN SUSPEND-00+}		
		pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_PWM_TCXO);
		hook_switch_enable(true);
		if ( headset_without_mic == 0)
			state = 1;
		else
			state = 2;
		switch_set_state(&data->sdev, state);
	}

	enable_irq(data->irq);
	enable_irq_wake(data->irq);
	#else
	switch_set_state(&data->sdev, state);
    #endif
}


static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	struct gpio_switch_data *switch_data =
	    (struct gpio_switch_data *)dev_id;

	/* Div1-FW3-BSP-AUDIO */
	#ifdef CONFIG_FIH_FBX_AUDIO
	queue_work(headset_switch_wq, &switch_data->work);
	set_irq_type(gpio_to_irq(26), gpio_get_value_cansleep(26) ?
			IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
	#else
	schedule_work(&switch_data->work);
	#endif
	
	return IRQ_HANDLED;
}

#ifndef CONFIG_FIH_FBX_AUDIO
static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);
	const char *state;
	if (switch_get_state(sdev))
		state = switch_data->state_on;
	else
		state = switch_data->state_off;

	if (state)
		return sprintf(buf, "%s\n", state);
	return -1;
}
#endif

//MM-SL-HS WAKE UP IN SUSPEND-00+{
#ifdef CONFIG_FIH_FBX_AUDIO
#ifdef CONFIG_HAS_EARLYSUSPEND
void headset_early_suspend(struct early_suspend *h)
{
	printk(" Now is early suspend.\n");
	mSuspend = true;
}

void headset_late_resume(struct early_suspend *h)
{
	printk("Now is late resume.\n");
	mSuspend = false;
}
#endif
#endif
//MM-SL-HS WAKE UP IN SUSPEND-00+}

static int gpio_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_data *switch_data;
	int ret = 0;	

	if (!pdata)
		return -EBUSY;

	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data)
		return -ENOMEM;

	switch_data->sdev.name = pdata->name;
	switch_data->gpio = pdata->gpio;
	switch_data->name_on = pdata->name_on;
	switch_data->name_off = pdata->name_off;
	switch_data->state_on = pdata->state_on;
	switch_data->state_off = pdata->state_off;
	#ifndef CONFIG_FIH_FBX_AUDIO
	switch_data->sdev.print_state = switch_gpio_print_state;
	#endif
    ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0)
		goto err_switch_dev_register;

	ret = gpio_request(switch_data->gpio, pdev->name);
	if (ret < 0)
		goto err_request_gpio;

	ret = gpio_direction_input(switch_data->gpio);
	if (ret < 0)
		goto err_set_gpio_input;

	INIT_WORK(&switch_data->work, gpio_switch_work);
	/* Div1-FW3-BSP-AUDIO */
	#ifdef CONFIG_FIH_FBX_AUDIO
	headset_switch_wq = create_singlethread_workqueue("switch_gpio");
	
	if (!headset_switch_wq) {
		printk("%s: create workque failed \n", __func__);		
		return -EBUSY;
	}
	//MM-SL-HS WAKE UP IN SUSPEND-00+{
	#ifdef CONFIG_HAS_EARLYSUSPEND
	switch_data->es.level = 0;
	switch_data->es.suspend = headset_early_suspend;
	switch_data->es.resume = headset_late_resume;
	register_early_suspend(&switch_data->es);

	switch_data->hs_input = input_allocate_device();
	if (!switch_data->hs_input) {
 	printk("%s: input_allocate_device failed \n", __func__);  
	return -EBUSY;
	goto err_register_hs_input_dev;
	}
	
	switch_data->hs_input->name = "fih_ringswitch";
	set_bit(EV_KEY, switch_data->hs_input->evbit);  	
	set_bit(KEY_RINGSWITCH, switch_data->hs_input->keybit);		  	  
	#endif

	ret= input_register_device(switch_data->hs_input);
	if (ret < 0){	
	printk("%s: input_register_device failed \n", __func__);  		
	goto err_register_hs_input_dev;	
	}	 	
	//MM-SL-HS WAKE UP IN SUSPEND-00+}	
	#endif

	switch_data->irq = gpio_to_irq(switch_data->gpio);
	if (switch_data->irq < 0) {
		ret = switch_data->irq;
		goto err_detect_irq_num_failed;
	}
	
	/* Div1-FW3-BSP-AUDIO */
	#ifdef CONFIG_FIH_FBX_AUDIO
		ret = request_irq(switch_data->irq, gpio_irq_handler,
	            IRQF_TRIGGER_HIGH, pdev->name, switch_data);
	#else
		ret = request_irq(switch_data->irq, gpio_irq_handler,
				IRQF_TRIGGER_LOW, pdev->name, switch_data);
	#endif

	if (ret < 0)
		goto err_request_irq;
	
	/* Div1-FW3-BSP-AUDIO */
    #ifdef CONFIG_FIH_FBX_AUDIO
	ret = enable_irq_wake(MSM_GPIO_TO_INT(switch_data->gpio));
	if (ret < 0){
		printk("Headset: gpio_switch_prob: enable_irq_wake failed for HS detect.\n");
		goto err_request_irq;
	}
    #endif

	/* Perform initial detection */
	gpio_switch_work(&switch_data->work);

	return 0;
//MM-SL-HS WAKE UP IN SUSPEND-00+{
#ifdef CONFIG_FIH_FBX_AUDIO
err_register_hs_input_dev:
	input_free_device(switch_data->hs_input);
#endif	
//MM-SL-HS WAKE UP IN SUSPEND-00+}	
err_request_irq:
err_detect_irq_num_failed:
err_set_gpio_input:
	gpio_free(switch_data->gpio);
err_request_gpio:
    switch_dev_unregister(&switch_data->sdev);
err_switch_dev_register:
	kfree(switch_data);

	return ret;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = platform_get_drvdata(pdev);

	cancel_work_sync(&switch_data->work);
	gpio_free(switch_data->gpio);
    switch_dev_unregister(&switch_data->sdev);
	kfree(switch_data);

	return 0;
}

static struct platform_driver gpio_switch_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
	/* Div1-FW3-BSP-AUDIO */
	#ifdef CONFIG_FIH_FBX_AUDIO
		.name	= "switch_gpio",
	#else
		.name	= "switch-gpio",
	#endif
		.owner	= THIS_MODULE,
	},
};

static int __init gpio_switch_init(void)
{
	return platform_driver_register(&gpio_switch_driver);
}

static void __exit gpio_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_driver);
}

module_init(gpio_switch_init);
module_exit(gpio_switch_exit);

MODULE_AUTHOR("Mike Lockwood <lockwood@android.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");
