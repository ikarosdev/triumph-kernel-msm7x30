/*
 *  FIH headset device detection driver.
 *
 */

/*  

    Logically, the H2W driver is always present, and H2W state (hi->state)
    indicates what is currently plugged into the headset interface.

    The headset detection work involves GPIO. A headset will still
    pull this line low.

    Headset insertion/removal causes UEvent's to be sent, and
    /sys/class/switch/headset_sensor/state to be updated.

    Button presses are interpreted as input event (KEY_MEDIA). 

*/


#include <linux/module.h>
#include <linux/sysdev.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/switch.h>
#include <linux/debugfs.h>
#include <asm/gpio.h>
#include <asm/atomic.h>
#include <mach/mpp.h>//MM-RC-ChangeCodingStyle-00+
#include<linux/fih_hw_info.h>//MM-RC-ChangeCodingStyle-00+
#if 0
//SW2-6-MM-RC-SF5-2030-AUDIO-PORTING-01+{
#ifdef CONFIG_FIH_PROJECT_SF4V5
#include <mach/mpp.h>
#endif
#endif
//SW2-6-MM-RC-SF5-2030-AUDIO-PORTING-01+}
#include <asm/io.h>     
#include "../gpio_hw.h"   
//#include "../../../../drivers/serial/msm_serial.h"   //for gasko/dorian only, uart 83
#include <mach/pmic.h>
#include <linux/earlysuspend.h>  //Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00+
//Define the GPIOs
//SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*{
//SW2-6-MM-RC-Audio_Porting-00+{
//#ifdef CONFIG_FIH_PROJECT_SFX //MM-RC-ChangeCodingStyle-00-
//Assessory Insertion Detection (AID) Pin
#define AUD_PIN_HEADSET_DET 26  //for SXX
//Push To Talk (PTT) Pin
#define AUD_PIN_HOOK_BTN 44 	//for SXX
//MM-RC-ChangeCodingStyle-00+{
#define ASWISH_CTL 37 	//for SFH L:ext. mic H:TV
//MM-RC-ChangeCodingStyle-00+}
//SW2-6-MM-RC-Audio_Porting-00+}
//#endif //MM-RC-ChangeCodingStyle-00-
//SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*}

#define CONFIG_DEBUG_FIH_HEADSET 
#define AUD_HOOK_BTN 

#define KEY_RINGSWITCH                  184 

#ifdef CONFIG_DEBUG_FIH_HEADSET
#define H2W_DBG(fmt, arg...) printk(KERN_INFO "[AUD_HS] %s " fmt "\n", __FUNCTION__, ## arg)
#else
#define H2W_DBG(fmt, arg...) do {} while (0)
#endif

static struct workqueue_struct *g_detection_work_queue;
static void detection_work(struct work_struct *work);
static DECLARE_WORK(g_detection_work, detection_work);


int bn_irq_enable = 0;
bool mHeadphone = false;

bool mSuspend = false; //Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-

//Define the switch state
// /sys/class/switch/headset_sensor/state
//Need to be syncronized with HeadsetObserver.java
enum {
	NO_DEVICE	= 0,
	HEADSET	= 1,
};
//SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*{

//MM-RC-ChangeCodingStyle-00+{
int HS_PLUG_IN  = 0;
int IRQF_TRIGGER_HS_INSERTED = IRQF_TRIGGER_LOW;
//MM-RC-ChangeCodingStyle-00+}
//MM-RC-ChangeCodingStyle-00-{
#if 0
//SW2-6-MM-RC-Audio_Porting-00*{
enum {
#ifdef CONFIG_FIH_PROJECT_SF4Y6
       HS_PLUG_IN  = 1,
       HS_PLUG_OUT = 0,
       IRQF_TRIGGER_HS_INSERTED = IRQF_TRIGGER_HIGH,
#elif defined CONFIG_FIH_PROJECT_SF4V5
       HS_PLUG_IN  = 0,
       HS_PLUG_OUT = 1,
       IRQF_TRIGGER_HS_INSERTED = IRQF_TRIGGER_LOW,
// SW251-RL-PORTING_SF8_FLAG-00--{
#elif defined CONFIG_FIH_PROJECT_SF8
       HS_PLUG_IN  = 0,
       HS_PLUG_OUT = 1,
       IRQF_TRIGGER_HS_INSERTED = IRQF_TRIGGER_LOW,
#endif       
// SW251-RL-PORTING_SF8_FLAG-00--}
};
//SW2-6-MM-RC-Audio_Porting-00*{
#endif
//MM-RC-ChangeCodingStyle-00-}

//SXX GPIO 44, when hook button pressed, L -> H
enum {
       BTN_STATE_RELEASED = 0,
       BTN_STATE_PRESSED = 1,
       IRQF_TRIGGER_BTN_PRESSED = IRQF_TRIGGER_HIGH,
};
//SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*}
struct h2w_info {
	struct switch_dev sdev;
	struct input_dev *input;
	struct input_dev *hs_input; 

	atomic_t btn_state;
	atomic_t hs_state;  
	int ignore_hs;      
	int ignore_btn;

	unsigned int irq;
	unsigned int irq_btn;

	struct hrtimer timer;
	ktime_t debounce_time;

	struct hrtimer btn_timer;
	ktime_t btn_debounce_time;
//MM-SL-CurrentIsTooLargeInSuspend-00-{
#if 0
//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00+{	
#ifdef CONFIG_HAS_EARLYSUSPEND
    	struct early_suspend es;
#endif	
//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00+}
#endif
//MM-SL-CurrentIsTooLargeInSuspend-00}
};
static struct h2w_info *hi;

static ssize_t trout_h2w_print_name(struct switch_dev *sdev, char *buf)
{
       int state = 0;
       state = switch_get_state(&hi->sdev);
       //H2W_DBG("%s, state = %d", __FUNCTION__, state);
	switch (state) {
	case NO_DEVICE:
		return sprintf(buf, "No Device\n");
	case HEADSET:         
		return sprintf(buf, "Headset\n");
	}
	return -EINVAL;
}



void aud_hs_print_gpio(void)
{
	H2W_DBG("hs gpio(%d) = %d, ptt gpio(%d) = %d ", AUD_PIN_HEADSET_DET, gpio_get_value(AUD_PIN_HEADSET_DET), AUD_PIN_HOOK_BTN, gpio_get_value(AUD_PIN_HOOK_BTN));
}



void aud_hs_dump_reg(void)
{
#if 0
      H2W_DBG("");
      H2W_DBG("GPIO 94, mask = 0x0400-0000");
      //H2W_DBG("GPIO 87, mask = 0x0008-0000");
      H2W_DBG("GPIO_OUT_3 : 0x%08x", readl(GPIO_OUT_3));
      H2W_DBG("GPIO_OE_3 : 0x%08x", readl(GPIO_OE_3));	  
      H2W_DBG("GPIO_IN_3 : 0x%08x", readl(GPIO_IN_3));
      H2W_DBG("GPIO_INT_EDGE_3 : 0x%08x", readl(GPIO_INT_EDGE_3));
      H2W_DBG("GPIO_INT_POS_3 : 0x%08x", readl(GPIO_INT_POS_3));
      H2W_DBG("GPIO_INT_EN_3 : 0x%08x", readl(GPIO_INT_EN_3));  
      H2W_DBG("GPIO_INT_CLEAR_3 : 0x%08x", readl(GPIO_INT_CLEAR_3));
      H2W_DBG("GPIO_INT_STATUS_3 : 0x%08x", readl(GPIO_INT_STATUS_3));
      H2W_DBG("========================");
      H2W_DBG("GPIO 21, mask = 0x0000-0020");
      H2W_DBG("GPIO_OUT_1 : 0x%08x", readl(GPIO_OUT_1));
      H2W_DBG("GPIO_OE_1 : 0x%08x", readl(GPIO_OE_1));	  
      H2W_DBG("GPIO_IN_1 : 0x%08x", readl(GPIO_IN_1));
      H2W_DBG("GPIO_INT_EDGE_1 : 0x%08x", readl(GPIO_INT_EDGE_1));
      H2W_DBG("GPIO_INT_POS_1 : 0x%08x", readl(GPIO_INT_POS_1));
      H2W_DBG("GPIO_INT_EN_1 : 0x%08x", readl(GPIO_INT_EN_1));  
      H2W_DBG("GPIO_INT_CLEAR_1 : 0x%08x", readl(GPIO_INT_CLEAR_1));
      H2W_DBG("GPIO_INT_STATUS_1 : 0x%08x", readl(GPIO_INT_STATUS_1));
#endif

}


//KEY_MEDIA is defined in <Linux/include/input.h>
//You can modify the keypad layout to map this key for Android
//For ADQ project, it will be located on : Vendor/qcom/msm7225_adq/stmpe1601.kl
//check the Android.mk first.
#ifdef AUD_HOOK_BTN
static void button_pressed(void)
{
	H2W_DBG("");
	atomic_set(&hi->btn_state, 1); 
        input_report_key(hi->input, KEY_MEDIA, 1);
	//Div6-PT2-MM-SL-PTT DETECT-00-input_sync(hi->input);
}

static void button_released(void)
{
        H2W_DBG("");
        atomic_set(&hi->btn_state, 0);
        input_report_key(hi->input, KEY_MEDIA, 0); 
        //Div6-PT2-MM-SL-PTT DETECT-00-input_sync(hi->input);
}
#endif

#ifdef CONFIG_MSM_SERIAL_DEBUGGER
extern void msm_serial_debug_enable(int);
#endif



static void insert_headset(void)
{
	unsigned long irq_flags;

	H2W_DBG("");

	//MM-SL-CurrentIsTooLargeInSuspend-00-{
	#if 0
	//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00*{
	if (mSuspend)
	{
		input_report_key(hi->hs_input, KEY_RINGSWITCH, 1);  
 		H2W_DBG("aud_hs: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			

  		input_report_key(hi->hs_input, KEY_RINGSWITCH, 0);	 	
  		H2W_DBG("aud_hs: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			
	}
	//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00*}
	#endif
	//MM-SL-CurrentIsTooLargeInSuspend-00-}
  	input_sync(hi->hs_input);			
				
#ifdef CONFIG_MSM_SERIAL_DEBUGGER
	msm_serial_debug_enable(false);
#endif
		//SW2-6-MM-RC-Call should be pick up via the wird headset-00+{
		//SW2-6-MM-RC-Audio_Porting-00+{
		//In7x30, ext. mic bias must be enabled  here
		//#ifdef CONFIG_FIH_PROJECT_SFX //MM-RC-ChangeCodingStyle-00-
		//MM-RC-ChangeCodingStyle-00+{
		//ASWISH_CTL: for switching ext.mic and TV in SFH
		//SFH need to enable VREG_L12_V3 for ext. mic, and the power is always opened.
		if(fih_get_product_id()==Product_SFH){
			gpio_set_value(ASWISH_CTL, 0); 
			H2W_DBG("aud_hs:switch to ext. mic\n ");
		}
		//MM-RC-ChangeCodingStyle-00+}
		pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_PWM_TCXO);
		H2W_DBG("aud_hs:open mic bias\n ");
		//#endif //MM-RC-ChangeCodingStyle-00-
		//SW2-6-MM-RC-Audio_Porting-00+}
		msleep(100); //MM-RC-ChangeCodingStyle-00*
		//SW2-6-MM-RC-Call should be pick up via the wird headset-00+}		
#ifdef AUD_HOOK_BTN
	/* On some non-standard headset adapters (usually those without a
	 * button) the btn line is pulled down at the same time as the detect
	 * line. We can check here by sampling the button line, if it is
	 * low then it is probably a bad adapter so ignore the button.
	 * If the button is released then we stop ignoring the button, so that
	 * the user can recover from the situation where a headset is plugged
	 * in with button held down.
	 */
	hi->ignore_btn = gpio_get_value(AUD_PIN_HOOK_BTN);  
	if(bn_irq_enable==0)
	{
		/* Enable button irq */
		local_irq_save(irq_flags);
		enable_irq(hi->irq_btn);
		set_irq_wake(hi->irq_btn, 1); 
		local_irq_restore(irq_flags);
		bn_irq_enable=1;
		}
#endif
	//Div6-PT2-MM-SL-HeadsetDetect-00-hi->debounce_time = ktime_set(0, 20000000);  /* 20 ms */
	//H2W_DBG("[SEVEN][1]aud_hs:gpio_get_value(AUD_PIN_HOOK_BTN) =%d\n",gpio_get_value(AUD_PIN_HOOK_BTN) );
	if(gpio_get_value(AUD_PIN_HEADSET_DET)==HS_PLUG_IN)//SW2-6-MM-RC-Audio_Porting-00*//SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*
	{
	
        	if(gpio_get_value(AUD_PIN_HOOK_BTN)==1)
        	{
            		switch_set_state(&hi->sdev, HEADSET);
            		#ifdef AUD_HOOK_BTN 
            		set_irq_type(hi->irq_btn, IRQF_TRIGGER_LOW );
            		#endif 
			//H2W_DBG("[SEVEN][2]aud_hs:gpio_get_value(AUD_PIN_HOOK_BTN) =%d\n",gpio_get_value(AUD_PIN_HOOK_BTN) );
			msleep(500);
			if(gpio_get_value(AUD_PIN_HOOK_BTN)==1)
			{
       			//SW2-6-MM-RC-Audio_Porting-00+{
       			//In7x30 3 rings, ext. mic bias must be disabled  here for power saving purpose
       			//#ifdef CONFIG_FIH_PROJECT_SFX  //MM-RC-ChangeCodingStyle-00-
				pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_OFF);//SW2-6-MM-RC-Audio_Porting-00+
				H2W_DBG("aud_hs:3 rings close mic bias\n ");
       			//#endif //MM-RC-ChangeCodingStyle-00-
       			//SW2-6-MM-RC-Audio_Porting-00+}
				mHeadphone=true;
				H2W_DBG("aud_hs:HEADPHONE is plugging\n ");
        		}
			else
			{
				H2W_DBG("aud_hs:HEADSET is plugging\n ");
			}
        	}
        	else
        	{
            		switch_set_state(&hi->sdev, HEADSET);
            		#ifdef AUD_HOOK_BTN 
            		set_irq_type(hi->irq_btn, IRQF_TRIGGER_HIGH);
            		#endif 
			H2W_DBG("aud_hs:HEADSET is plugging\n ");
        	}
        	H2W_DBG("switch_get_state= %d ",switch_get_state(&hi->sdev));
	
	}
}

static void remove_headset(void)
{
	unsigned long irq_flags;

	H2W_DBG("");

	switch_set_state(&hi->sdev, NO_DEVICE);
	
	//MM-SL-CurrentIsTooLargeInSuspend-00-{
	#if 0
	if (mSuspend)
	{
		input_report_key(hi->hs_input, KEY_RINGSWITCH, 1);  
 		H2W_DBG("FIH: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			

  		input_report_key(hi->hs_input, KEY_RINGSWITCH, 0);	 	
  		H2W_DBG("FIH: keypress KEY_RINGSWITCH = %d\n",KEY_RINGSWITCH);			
	}
	//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00*}
	#endif
	//MM-SL-CurrentIsTooLargeInSuspend-00-}
	
  	input_sync(hi->hs_input);			

#ifdef AUD_HOOK_BTN

	mHeadphone=false;
	if(bn_irq_enable==1)
	{
		/* Disable button */
		local_irq_save(irq_flags);
		disable_irq(hi->irq_btn);
		set_irq_wake(hi->irq_btn, 0); 
		local_irq_restore(irq_flags);
		bn_irq_enable=0;
	}


	if (atomic_read(&hi->btn_state))
		button_released();
#endif
	//SW2-6-MM-RC-Call should be pick up via the wird headset-00+{
	//SW2-6-MM-RC-Audio_Porting-00+{
       //In7x30, ext. mic bias must be disabled  here
       //#ifdef CONFIG_FIH_PROJECT_SFX //MM-RC-ChangeCodingStyle-00-
	pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_OFF);//SW2-6-MM-RC-Audio_Porting-00+
	H2W_DBG("aud_hs:close mic bias\n ");
	//#endif //MM-RC-ChangeCodingStyle-00-
//SW2-6-MM-RC-Audio_Porting-00+{
//SW2-6-MM-RC-Call should be pick up via the wird headset-00+}
	//Div6-PT2-MM-SL-HeadsetDetect-00-hi->debounce_time = ktime_set(0, 100000000);  /* 100 ms */
}

static void detection_work(struct work_struct *work)
{
	//unsigned long irq_flags;
	int cable_in1;

	H2W_DBG("");

        //aud_hs_dump_reg();
        
	if (gpio_get_value(AUD_PIN_HEADSET_DET) != HS_PLUG_IN) {
		/* Headset not plugged in */
		if (switch_get_state(&hi->sdev) == HEADSET)
		{
			H2W_DBG("Headset is plugged out.\n");
			remove_headset();

		}
		return;
	}

	/* Something plugged in, lets make sure its a headset */
	cable_in1 = gpio_get_value(AUD_PIN_HEADSET_DET);
       
      if (cable_in1 == HS_PLUG_IN ) {
          	if (switch_get_state(&hi->sdev) == NO_DEVICE)
          	{
          		H2W_DBG("Headset is plugged in.\n");
          		insert_headset();
          	}
      } else {
          	H2W_DBG("WARN: AUD_PIN_HEADSET_DET was low, but not a headset ");
      }

}

#ifdef AUD_HOOK_BTN
static enum hrtimer_restart button_event_timer_func(struct hrtimer *data)
{
	H2W_DBG("");
	//aud_hs_dump_reg(); 
	
	if (switch_get_state(&hi->sdev) == HEADSET) {
		if (gpio_get_value(AUD_PIN_HOOK_BTN) == BTN_STATE_RELEASED) {  
			if (hi->ignore_btn)
				hi->ignore_btn = 0;
			else if (atomic_read(&hi->btn_state))
				button_released();
		} else {
			if (!hi->ignore_btn && !atomic_read(&hi->btn_state))
				button_pressed();
		}
	}

	return HRTIMER_NORESTART;
}
#endif

static enum hrtimer_restart detect_event_timer_func(struct hrtimer *data)
{
	H2W_DBG("");

	queue_work(g_detection_work_queue, &g_detection_work);
	return HRTIMER_NORESTART;
}

static irqreturn_t detect_irq_handler(int irq, void *dev_id)
{
	int value1, value2;
	int retry_limit = 10;

	H2W_DBG("");
	aud_hs_print_gpio(); 

      //debunce
	do {
		value1 = gpio_get_value(AUD_PIN_HEADSET_DET);         
		set_irq_type(hi->irq, value1 ?
				IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
		value2 = gpio_get_value(AUD_PIN_HEADSET_DET);         
             //H2W_DBG("VALUE1 = %d, value2 = %d\n", value1, value2);
	} while (value1 != value2 && retry_limit-- > 0);

	H2W_DBG("value2 = %d (%d retries)", value2, (10-retry_limit));	

      /*
    * If the sdev is NO_DEVICE, and we detect the headset has been plugged,
    * then we can do headset_insertion check.
	*/
	if ((switch_get_state(&hi->sdev) == NO_DEVICE) ^ (value2^HS_PLUG_IN)) {//SW2-6-MM-RC-Audio_Porting-00*
		if (switch_get_state(&hi->sdev) == HEADSET)          
			hi->ignore_btn = 1;
		/* Do the rest of the work in timer context */
		hrtimer_start(&hi->timer, hi->debounce_time, HRTIMER_MODE_REL);
	}

	return IRQ_HANDLED;
}

#ifdef AUD_HOOK_BTN 
static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
	int value1, value2;
	int retry_limit = 10;

	H2W_DBG("");
	aud_hs_print_gpio(); 
	do {
		value1 = gpio_get_value(AUD_PIN_HOOK_BTN);          
		set_irq_type(hi->irq_btn, value1 ?
				IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
		value2 = gpio_get_value(AUD_PIN_HOOK_BTN);
		//H2W_DBG("VALUE1 = %d, value2 = %d\n", value1, value2);
	} while (value1 != value2 && retry_limit-- > 0);

	H2W_DBG("Hook BTN :value2 = %d (%d retries)", value2, (10-retry_limit));

	 
	hrtimer_start(&hi->btn_timer, hi->btn_debounce_time, HRTIMER_MODE_REL);

	return IRQ_HANDLED;
}
#endif 

#if defined(CONFIG_DEBUG_FS)

static int __init h2w_debug_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("h2w", 0);
	if (IS_ERR(dent))
		return PTR_ERR(dent);

	return 0;
}

device_initcall(h2w_debug_init);
#endif

//MM-SL-CurrentIsTooLargeInSuspend-00-{
#if 0
//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00+{
#ifdef CONFIG_HAS_EARLYSUSPEND
void headset_early_suspend(struct early_suspend *h)
{
	H2W_DBG(" Now is early suspend.\n");
	mSuspend = true;
}

void headset_late_resume(struct early_suspend *h)
{
	H2W_DBG("Now is late resume.\n");
	mSuspend = false;
}
#endif
//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00+}
#endif
//MM-SL-CurrentIsTooLargeInSuspend-00-}

static int trout_h2w_probe(struct platform_device *pdev)
{
	int ret;

	printk(KERN_INFO "[AUD_HS]: Registering H2W (headset) driver\n");
	//MM-RC-ChangeCodingStyle-00+{

	//MM-RC-ChangeCodingStyle-00+}
    
      //aud_hs_dump_reg();
      //ret = gpio_tlmm_config(GPIO_CFG(94, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
      //if(ret < 0)
         //H2W_DBG("roger: gpio_tlmm_config(gpio 94) failed");
      //ret = gpio_tlmm_config(GPIO_CFG(21, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
      //if(ret < 0)
         //H2W_DBG("roger: gpio_tlmm_config(gpio 21) failed");
      //aud_hs_dump_reg();
//      GPIO_CFG(94, 0, BSP_GPIO_INPUT, BSP_GPIO_NO_PULL, BSP_GPIO_2MA, NO_RMT),
      

	hi = kzalloc(sizeof(struct h2w_info), GFP_KERNEL);
	if (!hi)
		return -ENOMEM;

	atomic_set(&hi->btn_state, 0);
	hi->ignore_btn = 0;

    // Headset insertion/removal causes UEvent's to be sent, and
    // /sys/class/switch/headset_sensor/state to be updated.
    
	hi->debounce_time = ktime_set(0, 500000000);  /* 500 ms */
	hi->btn_debounce_time = ktime_set(0, 80000000); /* 80 ms */   //debounce time too short will affect the behavior of headset plugin/out in phone call 
	hi->sdev.name = "headset_sensor";
	hi->sdev.print_name = trout_h2w_print_name;
	//MM-SL-CurrentIsTooLargeInSuspend-00-{
	#if 0
	//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00+{
	#ifdef CONFIG_HAS_EARLYSUSPEND
	hi->es.level = 0;
	hi->es.suspend = headset_early_suspend;
	hi->es.resume = headset_late_resume;
	register_early_suspend(&hi->es);
	#endif
	//Div6-PT2-MM-SL-HS WAKE UP IN SUSPEND-00+}
	#endif
	//MM-SL-CurrentIsTooLargeInSuspend-00-}
	
	hi->hs_input = input_allocate_device();
	if (!hi->hs_input) {
		        ret = -ENOMEM;
		  goto err_request_input_dev;
	}
	atomic_set(&hi->hs_state, 0);
 	hi->ignore_hs = 0;
  	hi->hs_input->name = "fih_ringswitch";
 	set_bit(EV_KEY, hi->hs_input->evbit);  	  
  	set_bit(KEY_RINGSWITCH, hi->hs_input->keybit);		  	  

	ret = input_register_device(hi->hs_input);
	    if (ret < 0)
		     goto err_register_hs_input_dev;	  


	ret = switch_dev_register(&hi->sdev);
	if (ret < 0)
		goto err_switch_dev_register;

	g_detection_work_queue = create_workqueue("detection");
	
	if (g_detection_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_create_work_queue;
	}

	ret = gpio_request(AUD_PIN_HEADSET_DET, "h2w_detect");  
	if (ret < 0)
		goto err_request_detect_gpio;

#ifdef AUD_HOOK_BTN
	ret = gpio_request(AUD_PIN_HOOK_BTN, "h2w_button");   
	if (ret < 0)
		goto err_request_button_gpio;
#endif 

	ret = gpio_direction_input(AUD_PIN_HEADSET_DET);    
	if (ret < 0)
		goto err_set_detect_gpio;
      else
         H2W_DBG(" set aid gpio(%d) as input pin : success.\r\n", AUD_PIN_HEADSET_DET);

#ifdef AUD_HOOK_BTN
	ret = gpio_direction_input(AUD_PIN_HOOK_BTN);   
	if (ret < 0)
		goto err_set_button_gpio;
      else
         H2W_DBG(" set ptt gpio(%d) as input pin : success.\r\n", AUD_PIN_HOOK_BTN);
#endif

	
	
	hi->irq = gpio_to_irq(AUD_PIN_HEADSET_DET);   
	if (hi->irq < 0) {
		ret = hi->irq;
		goto err_get_h2w_detect_irq_num_failed;
	}
       else
           H2W_DBG(" hs_det gpio_to_irq(%d): success.\r\n", AUD_PIN_HEADSET_DET);

#ifdef AUD_HOOK_BTN
	hi->irq_btn = gpio_to_irq(AUD_PIN_HOOK_BTN);   
	if (hi->irq_btn < 0) {
		ret = hi->irq_btn;
		goto err_get_button_irq_num_failed;
	}
      else
           H2W_DBG(" hook_btn gpio_to_irq(%d): success.\r\n", AUD_PIN_HOOK_BTN);
#endif

	hrtimer_init(&hi->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hi->timer.function = detect_event_timer_func;
#ifdef AUD_HOOK_BTN
	hrtimer_init(&hi->btn_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hi->btn_timer.function = button_event_timer_func;
#endif
	aud_hs_print_gpio(); 
      //aud_hs_dump_reg();
      //printk(KERN_INFO "[AUD_HS] roger: gpio 94 should not be LOW without insertion\r\n");

       //When 	headset inserted, gpio H->L, so we detect LOW level.
       //SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*{
       //SW2-6-MM-RC-Audio_Porting-00*{
	ret = request_irq(hi->irq, detect_irq_handler,
	         IRQF_TRIGGER_HS_INSERTED, "h2w_detect", NULL);
	  //SW2-6-MM-RC-Audio_Porting-00*}
	  //SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*}
	if (ret < 0)
		goto err_request_detect_irq;
      else
          H2W_DBG(" request_irq (gpio 94, IRQF_TRIGGER_LOW) success\n");

#ifdef AUD_HOOK_BTN
	/* Disable button until plugged in */
	set_irq_flags(hi->irq_btn, IRQF_VALID | IRQF_NOAUTOEN);
//SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*{
	ret = request_irq(hi->irq_btn, button_irq_handler,
			  IRQF_TRIGGER_BTN_PRESSED, "h2w_button", NULL);         
//SW2-6-MM-RC-Modify the HS Detect function for SF5 and SF6-00*}
	if (ret < 0)
		goto err_request_h2w_headset_button_irq;
      else
          H2W_DBG("request_irq (gpio 21, IRQF_TRIGGER_HIGH) success\n");
 #endif
 
       // Set headset_detect pin as wake up pin
	ret = set_irq_wake(hi->irq, 1);
	if (ret < 0)
		goto err_request_input_dev;

        if(gpio_get_value(AUD_PIN_HEADSET_DET) == HS_PLUG_IN)
       {
       	  printk(KERN_INFO "aud:  gpio 94 is %d, enable gpio21 wake up pin\n", gpio_get_value(AUD_PIN_HEADSET_DET));
#ifdef AUD_HOOK_BTN
               ret = set_irq_wake(hi->irq_btn, 1);
               if (ret < 0)
		     goto err_request_input_dev;
#endif
        }
        else{
               printk(KERN_INFO "aud:  gpio 94 is %d, disable gpio21 wake up pin\n", gpio_get_value(AUD_PIN_HEADSET_DET));
#ifdef AUD_HOOK_BTN
                 //Div6-PT2-MM-SL-HeadsetDetect-00-ret = set_irq_wake(hi->irq_btn, 0);
                 if (ret < 0)
			goto err_request_input_dev;
#endif
	}
	 
	
#ifdef AUD_HOOK_BTN
	hi->input = input_allocate_device();
	if (!hi->input) {
		ret = -ENOMEM;
		goto err_request_input_dev;
	}

	hi->input->name = "fih_headsethook";
	hi->input->evbit[0] = BIT_MASK(EV_KEY); 
      hi->input->keybit[BIT_WORD(KEY_MEDIA)] = BIT_MASK(KEY_MEDIA);   

	ret = input_register_device(hi->input);
	if (ret < 0)
		goto err_register_input_dev;
#endif
      //aud_hs_dump_reg(); //seven+
	return 0;

err_register_input_dev:
#ifdef AUD_HOOK_BTN
      printk(KERN_ERR "aud_hs: err_register_input_dev\n");
	input_free_device(hi->input);
#endif


err_register_hs_input_dev:
      printk(KERN_ERR "aud_hs: err_register_hs_input_dev\n");
	input_free_device(hi->hs_input);


err_request_input_dev:
#ifdef AUD_HOOK_BTN
      printk(KERN_ERR "aud_hs: err_request_input_dev\n");
	free_irq(hi->irq_btn, 0);
#endif    
err_request_h2w_headset_button_irq:
      printk(KERN_ERR "aud_hs: request_h2w_headset_button_irq\n");
	free_irq(hi->irq, 0);
err_request_detect_irq:
err_get_button_irq_num_failed:
err_get_h2w_detect_irq_num_failed:
err_set_button_gpio:
err_set_detect_gpio:
      printk(KERN_ERR "aud_hs: AUD_PIN_HOOK_BTN, gpio/irq error\n");
#ifdef AUD_HOOK_BTN
	gpio_free(AUD_PIN_HOOK_BTN);       
#endif
err_request_button_gpio:
       printk(KERN_ERR "aud_hs: err_request_button_gpio\n");
	gpio_free(AUD_PIN_HEADSET_DET);  
err_request_detect_gpio:
      printk(KERN_ERR "aud_hs: err_request_detect_gpio\n");
	destroy_workqueue(g_detection_work_queue);
err_create_work_queue:
      printk(KERN_ERR "aud_hs: err_create_work_queue\n");    
	switch_dev_unregister(&hi->sdev);
err_switch_dev_register:
	printk(KERN_ERR "aud_hs: Failed to register driver\n");

	return ret;
}

static int trout_h2w_remove(struct platform_device *pdev)
{
	H2W_DBG("");
	if (switch_get_state(&hi->sdev))
		remove_headset();
#ifdef AUD_HOOK_BTN
	input_unregister_device(hi->input);
	gpio_free(AUD_PIN_HOOK_BTN);       
#endif    
	gpio_free(AUD_PIN_HEADSET_DET);      
#ifdef AUD_HOOK_BTN
	free_irq(hi->irq_btn, 0);
#endif
	free_irq(hi->irq, 0);
	destroy_workqueue(g_detection_work_queue);
	switch_dev_unregister(&hi->sdev);

	return 0;
}

static struct platform_device trout_h2w_device = {
	.name		= "headset_sensor",
};

static struct platform_driver trout_h2w_driver = {
	.probe		= trout_h2w_probe,
	.remove		= trout_h2w_remove,
	.driver		= {
		.name		= "headset_sensor",
		.owner		= THIS_MODULE,
	},
};


static int __init trout_h2w_init(void)
{

	int ret;
//MM-RC-ChangeCodingStyle-00*{
	H2W_DBG("trout_h2w_init:set interrupt trigger level\n ");
	if(fih_get_product_id()==Product_SF6){
		HS_PLUG_IN  = 1;
       	IRQF_TRIGGER_HS_INSERTED = IRQF_TRIGGER_HIGH;
	}
	else
	{
		HS_PLUG_IN  = 0;
       	IRQF_TRIGGER_HS_INSERTED = IRQF_TRIGGER_LOW;
	}
//SW2-6-MM-RC-SF5-2030-AUDIO-PORTING-01+{
	if(fih_get_product_id()==Product_SF5){
		H2W_DBG("trout_h2w_init:PM_MPP_3 config\n ");
		pm8058_mpp_config_digital_in(PM_MPP_3,
			PM8058_MPP_DIG_LEVEL_L3, PM_MPP_DIN_TO_DBUS1);
	}
//SW2-6-MM-RC-SF5-2030-AUDIO-PORTING-01+}
//MM-RC-ChangeCodingStyle-00*}
	H2W_DBG("");

	ret = platform_driver_register(&trout_h2w_driver);
	if (ret)
		return ret;
	return platform_device_register(&trout_h2w_device);
}

static void __exit trout_h2w_exit(void)
{
	platform_device_unregister(&trout_h2w_device);
	platform_driver_unregister(&trout_h2w_driver);
}

module_init(trout_h2w_init);
module_exit(trout_h2w_exit);

MODULE_AUTHOR("Seven Lin <sevenlin@fihspec.com>");
MODULE_DESCRIPTION("FIH headset detection driver");
MODULE_LICENSE("Proprietary");
