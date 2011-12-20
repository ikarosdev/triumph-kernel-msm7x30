/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/io.h>
#ifdef CONFIG_SPI_QSD
#include <linux/spi/spi.h>
#endif
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/marimba.h>
#include <linux/i2c.h>
#include <linux/input.h>
#ifdef CONFIG_SMSC911X
#include <linux/smsc911x.h>
#endif
#include <linux/ofn_atlab.h>
#include <linux/power_supply.h>
/* Div2-SW2-BSP-FBX-KEYS { */
#ifdef CONFIG_KEYBOARD_PMIC8058
#include <linux/input/pmic8058-keypad.h>
#endif
/* } Div2-SW2-BSP-FBX-KEYS */
#include <linux/i2c/isa1200.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/i2c/tsc2007.h>
#include <linux/input/kp_flip_switch.h>
/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_PMIC8058
#include <linux/leds-pmic8058.h>
#endif
/* } Div2-SW2-BSP-FBX-LEDS */
#include <linux/input/cy8c_ts.h>
#include <linux/msm_adc.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/mpp.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/memory.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/msm_spi.h>
#include <mach/qdsp5v2/msm_lpa.h>
#include <mach/dma.h>
#include <linux/android_pmem.h>
#include <linux/input/msm_ts.h>
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
/* Div2-SW2-BSP-FBX-BATT { */
#ifdef CONFIG_BATTERY_MSM
#include <mach/msm_battery.h>
#endif
/* } Div2-SW2-BSP-FBX-BATT */
#include <mach/rpc_server_handset.h>
#include <mach/msm_tsif.h>
#include <linux/cyttsp.h>

#include <asm/mach/mmc.h>
#include <asm/mach/flash.h>
#include <mach/vreg.h>
#include "devices.h"
#include "timer.h"
#include "socinfo.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif
#include "pm.h"
#include "spm.h"
#include <linux/msm_kgsl.h>
#include <mach/dal_axi.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_reqs.h>
#include "smd_private.h"    //Div2-SW2-BSP,JOE HSU
#include "proc_comm.h"      //Div6-D1-JL-UsbPorting-00+
#include <linux/ctype.h>    //Div6D1-JL-FixWrongID
#include<linux/fih_hw_info.h> //MM-RC-ChangeFMPath-00+

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
#undef HDMI_RESET
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */

/* Div2-SW2-BSP-FBX-LEDS, Div2-SW2-BSP-FBX-KEYS, Div2-SW2-BSP-FBX-VIB, Div-SW2-BSP-FBX-OW { */
#ifdef CONFIG_LEDS_FIH_FBX_PWM
#include <mach/leds-fbx-pwm.h>
#endif
#ifdef CONFIG_KEYBOARD_FBX
#include <mach/fbx_kybd.h>
#endif
#ifdef CONFIG_FIH_PROJECT_SF4Y6
#include <mach/sf6_kybd.h>
#endif
#ifdef CONFIG_KEYBOARD_SF4H8
#include <mach/sf8_kybd.h>
#endif
#ifdef CONFIG_PMIC8058_VIBRATOR
#include <linux/pmic8058-vibrator.h>
#endif
#ifdef CONFIG_DS2482
#include <mach/ds2482.h>
#endif
#ifdef CONFIG_BATTERY_FIH_DS2784
#include <mach/ds2784.h>
#endif
#ifdef CONFIG_BATTERY_FIH_MSM
#include <mach/fih_msm_battery.h>
#endif
#ifdef CONFIG_BATTERY_BQ275X0
#include <mach/bq275x0_battery.h>
#endif
/* } Div2-SW2-BSP-FBX-LEDS, Div2-SW2-BSP-FBX-KEYS, Div2-SW2-BSP-FBX-VIB, Div2-SW2-BSP-FBX-OW, Div2-SW2-BSP-FBX-BATT */

/* FIHTDC, Div2-SW2-BSP Godfrey, FB0.B-396 */
#include <media/tavarua.h>
#ifdef CONFIG_FIH_FBX_AUDIO
#include <linux/switch.h> 
#endif

//DIV5-BSP-CH-SF6-SENSOR-PORTING00++[
//Div2D1-OH-eCompass-AKM8975C_Porting-00+{
#ifdef CONFIG_SENSORS_AKM8975C
#include <linux/akm8975.h> 
#endif
//Div2D1-OH-eCompass-AKM8975C_Porting-00+}
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++]

/* FIHTDC, Div2-SW2-BSP, Ming, PMEM { */
/* Enlarge PMEM_SF to 30 MB for WVGA */
#define MSM_PMEM_SF_SIZE    0x1E00000   //0x1700000
/* } FIHTDC, Div2-SW2-BSP, Ming, PMEM */
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#define MSM_FB_SIZE		0xA00000       ///0x500000
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
#define MSM_GPU_PHYS_SIZE       SZ_2M
#define MSM_PMEM_ADSP_SIZE      0x2000000  //SW2-5-CL-Camera-720P-00*
#define MSM_FLUID_PMEM_ADSP_SIZE	0x2800000
#define PMEM_KERNEL_EBI1_SIZE   0x600000
#define MSM_PMEM_AUDIO_SIZE     0x200000

#define PMIC_GPIO_INT		27
#define PMIC_VREG_WLAN_LEVEL	2900
#define PMIC_GPIO_SD_DET	36
#define PMIC_GPIO_SDC4_EN	17  /* PMIC GPIO Number 18 */
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL
#define PMIC_GPIO_HDMI_5V_EN	39  /* PMIC GPIO Number 40 */
#endif

//DIV5-CONN-MW-POWER SAVING MODE-01*[
#if  defined(CONFIG_FIH_PROJECT_SF4Y6) && defined(CONFIG_FIH_WIMAX_GCT_SDIO)
#define PMIC_HOST_WAKEUP_WiMAX_N_PR3    15 /* PMIC GPIO Number 16--> PR3 */
#endif
//DIV5-CONN-MW-POWER SAVING MODE-01*]

#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
#define PMIC_GPIO_HDMI_18V_EN   32  /* PMIC GPIO Number 33 */
#define GPIO_HDMI_5V_EN \
	GPIO_CFG(34, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA)
#define HDMI_INT                180	
static bool hdmi_init_done = false;
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
/* FIHTDC, Div2-SW2-BSP, Ming, LCM { */
#define PMIC_GPIO_LCM_V1P8_EN   35  /* PMIC GPIO Number 36 */
/* } FIHTDC, Div2-SW2-BSP, Ming, LCM */
//Div2-SW6-Conn-JC-WiFi-Init-00+{
#define PMIC_GPIO_WLAN_SHUTDOWN	22	/* PMIC GPIO Number 23 */
//Div2-SW6-Conn-JC-WiFi-Init-00+}
#define FPGA_SDCC_STATUS       0x8E0001A8

#define FPGA_OPTNAV_GPIO_ADDR	0x8E000026
#define OPTNAV_I2C_SLAVE_ADDR	(0xB0 >> 1)
#define OPTNAV_IRQ		20
#define OPTNAV_CHIP_SELECT	19

//DIV5-PHONE-JH-WiMAX_GPIO-01+[
#if defined(CONFIG_FIH_PROJECT_SF4Y6) && defined(CONFIG_FIH_WIMAX_GCT_SDIO)
#define PMIC_GPIO_WiMAX_WAKEUP_HOST  5  /* PMIC GPIO Number 6 */
#endif
//DIV5-PHONE-JH-WiMAX_GPIO-01+]

/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)    (sys_gpio - NR_GPIO_IRQS)

#define PMIC_GPIO_HAP_ENABLE   16  /* PMIC GPIO Number 17 */

#define HAP_LVL_SHFT_MSM_GPIO 24

#define	PM_FLIP_MPP 5 /* PMIC MPP 06 */

/* FIHTDC, Div2-SW2-BSP Godfrey */
//SQ01.FC-73: Change QTR8200 WCN clock source(32M) from PMIC-D0 to PMIC-A1
//Div2-SW2-CONN-JC-SF5_BT_Clock-00+{
#if defined(CONFIG_FIH_PROJECT_SF4V5) || defined(CONFIG_FIH_PROJECT_SF4Y6)
#define QTR8x00_WCN_CLK         PMAPP_CLOCK_ID_DO
#else
#define QTR8x00_WCN_CLK         PMAPP_CLOCK_ID_A1
#endif
//Div2-SW2-CONN-JC-SF5_BT_Clock-00+}

//MM-SL-OPControl-00+{
bool m_HsAmpOn=false;
bool m_SpkAmpOn=false; 
//MM-SL-OPControl-00+}

int sd_detect_pin = 0;
int sd_enable_pin = 0;

static int pm8058_gpios_init(void)
{
	int rc;
#if 0
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	struct pm8058_gpio sdcc_det = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
#endif
	struct pm8058_gpio sdc4_en = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
#endif
    /* FIHTDC, Div2-SW2-BSP, Ming, LCM { */
    struct pm8058_gpio lcm_v1p8_en ={
         .direction        = PM_GPIO_DIR_OUT,         //Let the pin be an output one.
         .output_buffer    = PM_GPIO_OUT_BUF_CMOS,    //HW suggestion
         .output_value     = 1,                       //You also can set 0, but it seems useless.
         .out_strength     = PM_GPIO_STRENGTH_HIGH,   //There are three options for this, LOW, MED, and HIGH, but I don!|t know the actual effect.
         .function         = PM_GPIO_FUNC_NORMAL,     //Let the pin be a general GPIO.
    };
    /* } FIHTDC, Div2-SW2-BSP, Ming, LCM */
	struct pm8058_gpio haptics_enable = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		.vin_sel        = 2,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
	};

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL
	struct pm8058_gpio hdmi_5V_en = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_VPH,
		.function       = PM_GPIO_FUNC_NORMAL,
	};
#endif
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
  struct pm8058_gpio hdmi_18V_en = {
        .direction      = PM_GPIO_DIR_OUT,
        .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
        .output_value   = 1,
        .pull           = PM_GPIO_PULL_NO,
        .out_strength   = PM_GPIO_STRENGTH_HIGH,
        .function       = PM_GPIO_FUNC_NORMAL,
    };
#endif    
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */

//Div2-SW6-Conn-JC-WiFi-Init-00+{
    struct pm8058_gpio wlan_shutdown = {
	  .direction		= PM_GPIO_DIR_OUT,
	  .output_buffer	= PM_GPIO_OUT_BUF_CMOS,
	  .output_value		= 0,
	  .pull			= PM_GPIO_PULL_NO,
	  .vin_sel		= 2, 
	  .out_strength		= PM_GPIO_STRENGTH_LOW,
	  .function		= PM_GPIO_FUNC_NORMAL,
	  .inv_int_pol		= 0,
    };
//Div2-SW6-Conn-JC-WiFi-Init-00+}

//DIV5-PHONE-JH-WiMAX_GPIO-01+[
#if defined(CONFIG_FIH_PROJECT_SF4Y6) && defined(CONFIG_FIH_WIMAX_GCT_SDIO)
	struct pm8058_gpio wimax_wakeup_host = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_UP_31P5,
		.vin_sel        = PM_GPIO_VIN_L6,  //SW2-CONN-EC-WiMAX_GPIO-04*
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
#endif
//DIV5-PHONE-JH-WiMAX_GPIO-01+]

//DIV5-CONN-MW-POWER SAVING MODE-01*[
#if  defined(CONFIG_FIH_PROJECT_SF4Y6) && defined(CONFIG_FIH_WIMAX_GCT_SDIO)
	struct pm8058_gpio pmic_wimax_wakeup_host_pr3 = {
        .direction      = PM_GPIO_DIR_OUT,
        .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
        .output_value   = 1,
        .pull           = PM_GPIO_PULL_DN,
        .vin_sel	= PM_GPIO_VIN_VPH,         
        .out_strength   = PM_GPIO_STRENGTH_HIGH,
        .function       = PM_GPIO_FUNC_NORMAL,
	};
#endif
//DIV5-CONN-MW-POWER SAVING MODE-01*]

	if (machine_is_msm7x30_fluid()) {
		rc = pm8058_gpio_config(PMIC_GPIO_HAP_ENABLE, &haptics_enable);
		if (rc) {
			pr_err("%s: PMIC GPIO %d write failed\n", __func__,
				(PMIC_GPIO_HAP_ENABLE + 1));
			return rc;
		}
	}
#if 0
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	if (machine_is_msm7x30_fluid())
		sdcc_det.inv_int_pol = 1;

	rc = pm8058_gpio_config(PMIC_GPIO_SD_DET - 1, &sdcc_det);
	if (rc) {
		pr_err("%s PMIC_GPIO_SD_DET config failed\n", __func__);
		return rc;
	}
#endif
#endif
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL
	rc = pm8058_gpio_config(PMIC_GPIO_HDMI_5V_EN, &hdmi_5V_en);
	if (rc) {
		pr_err("%s PMIC_GPIO_HDMI_5V_EN config failed\n", __func__);
		return rc;
	}
	rc = gpio_request(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HDMI_5V_EN),
		"hdmi_5V_en");
	if (rc) {
		pr_err("%s PMIC_GPIO_HDMI_5V_EN gpio_request failed\n",
			__func__);
		return rc;
	}
#endif
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
	rc = pm8058_gpio_config(PMIC_GPIO_HDMI_18V_EN, &hdmi_18V_en);
	if (rc) {
		pr_err("%s PMIC_GPIO_HDMI_1.8V_EN config failed\n", __func__);
		return rc;
	}
	rc = gpio_request(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HDMI_18V_EN),
		"hdmi_18V_en");
	if (rc) {
		pr_err("%s PMIC_GPIO_HDMI_1.8V_EN gpio_request failed\n",
			__func__);
		return rc;
	}

		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HDMI_18V_EN), 1);    
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
#if 0
	if (machine_is_msm7x30_fluid()) {
		rc = pm8058_gpio_config(PMIC_GPIO_SDC4_EN, &sdc4_en);
		if (rc) {
			pr_err("%s PMIC_GPIO_SDC4_EN config failed\n",
								 __func__);
			return rc;
		}
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC4_EN), 1);
	}
#endif
    /* FIHTDC, Div2-SW2-BSP, Ming, LCM { */
    rc = pm8058_gpio_config(PMIC_GPIO_LCM_V1P8_EN, &lcm_v1p8_en);
    if (rc) {
        pr_err("%s PMIC_GPIO_LCM_V1P8_EN config failed\n",
                             __func__);
        return rc;
    }
    /* } FIHTDC, Div2-SW2-BSP, Ming, LCM */

//Div2-SW6-Conn-JC-WiFi-Init-00+{
    rc = pm8058_gpio_config(PMIC_GPIO_WLAN_SHUTDOWN, &wlan_shutdown);
    if (rc) {
        pr_err("%s PMIC_GPIO_WLAN_SHUTDOWN config failed\n",
                             __func__);
         return rc;
       }
    gpio_set_value(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_WLAN_SHUTDOWN), 0);
//Div2-SW6-Conn-JC-WiFi-Init-00+}

//DIV5-CONN-MW-POWER SAVING MODE-01*[
#if defined(CONFIG_FIH_PROJECT_SF4Y6) && defined(CONFIG_FIH_WIMAX_GCT_SDIO)
    if (fih_get_product_id() == Product_SF6) 
    {
        if (fih_get_product_phase() <= Product_PR2)
        {
	rc = pm8058_gpio_config(PMIC_GPIO_WiMAX_WAKEUP_HOST, &wimax_wakeup_host);
	if (rc) {
        printk(KERN_INFO "%s: PMIC_GPIO_WiMAX_WAKEUP_HOST config fail\n", __func__);
		return rc;
	}
        }
        else
        {
	    rc = pm8058_gpio_config(PMIC_HOST_WAKEUP_WiMAX_N_PR3, &pmic_wimax_wakeup_host_pr3);
	    if (rc) {
	    	      pr_err("%s PMIC_HOST_WAKEUP_WiMAX_N_PR3 config failed\n", __func__);
		      return rc;
	    }
	    rc = gpio_request(PM8058_GPIO_PM_TO_SYS(PMIC_HOST_WAKEUP_WiMAX_N_PR3),
	  	    "pmic_wimax_wakeup_host");
	    if (rc) {
		    pr_err("%s PMIC_HOST_WAKEUP_WiMAX_N_PR3 gpio_request failed\n",
			__func__);
		    return rc;
	     }
	     gpio_set_value(PM8058_GPIO_PM_TO_SYS(PMIC_HOST_WAKEUP_WiMAX_N_PR3), 0);
        }

    }


	
#endif
//DIV5-CONN-MW-POWER SAVING MODE-01*]
	return 0;
}

/*virtual key support */
static ssize_t tma300_vkeys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":80:904:160:210"
	":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":240:904:160:210"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":400:904:160:210"
	"\n");
}

static struct kobj_attribute tma300_vkeys_attr = {
	.attr = {
		.mode = S_IRUGO,
	},
	.show = &tma300_vkeys_show,
};

static struct attribute *tma300_properties_attrs[] = {
	&tma300_vkeys_attr.attr,
	NULL
};

static struct attribute_group tma300_properties_attr_group = {
	.attrs = tma300_properties_attrs,
};

static struct kobject *properties_kobj;

#define CYTTSP_TS_GPIO_IRQ	150
static int cyttsp_platform_init(struct i2c_client *client)
{
	int rc = -EINVAL;
	struct vreg *vreg_ldo8, *vreg_ldo15;

	vreg_ldo8 = vreg_get(NULL, "gp7");

	if (!vreg_ldo8) {
		pr_err("%s: VREG L8 get failed\n", __func__);
		return rc;
	}

	rc = vreg_set_level(vreg_ldo8, 1800);
	if (rc) {
		pr_err("%s: VREG L8 set failed\n", __func__);
		goto l8_put;
	}

	rc = vreg_enable(vreg_ldo8);
	if (rc) {
		pr_err("%s: VREG L8 enable failed\n", __func__);
		goto l8_put;
	}

	vreg_ldo15 = vreg_get(NULL, "gp6");

	if (!vreg_ldo15) {
		pr_err("%s: VREG L15 get failed\n", __func__);
		goto l8_disable;
	}

	rc = vreg_set_level(vreg_ldo15, 3050);
	if (rc) {
		pr_err("%s: VREG L15 set failed\n", __func__);
		goto l8_disable;
	}

	rc = vreg_enable(vreg_ldo15);
	if (rc) {
		pr_err("%s: VREG L15 enable failed\n", __func__);
		goto l8_disable;
	}

	/* check this device active by reading first byte/register */
	rc = i2c_smbus_read_byte_data(client, 0x01);
	if (rc < 0) {
		pr_err("%s: i2c sanity check failed\n", __func__);
		goto l8_disable;
	}

	rc = gpio_tlmm_config(GPIO_CFG(CYTTSP_TS_GPIO_IRQ, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: Could not configure gpio %d\n",
					 __func__, CYTTSP_TS_GPIO_IRQ);
		goto l8_disable;
	}

	rc = gpio_request(CYTTSP_TS_GPIO_IRQ, "ts_irq");
	if (rc) {
		pr_err("%s: unable to request gpio %d (%d)\n",
			__func__, CYTTSP_TS_GPIO_IRQ, rc);
		goto l8_disable;
	}

	/* virtual keys */
	tma300_vkeys_attr.attr.name = "virtualkeys.cyttsp-i2c";
	properties_kobj = kobject_create_and_add("board_properties",
				NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
			&tma300_properties_attr_group);
	if (!properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n",
				__func__);

	return CY_OK;

l8_disable:
	vreg_disable(vreg_ldo8);
l8_put:
	vreg_put(vreg_ldo8);
	return rc;
}

static int cyttsp_platform_resume(struct i2c_client *client)
{
	/* add any special code to strobe a wakeup pin or chip reset */
	mdelay(10);

	return CY_OK;
}

static struct cyttsp_platform_data cyttsp_data = {
	.maxx = 479,
	.maxy = 799,
	.flags = 0,
	.gen = CY_GEN3,	/* or */
	.use_st = CY_USE_ST,
	.use_mt = CY_USE_MT,
	.use_hndshk = CY_SEND_HNDSHK,
	.use_trk_id = CY_USE_TRACKING_ID,
	.use_sleep = CY_USE_SLEEP,
	.use_gestures = CY_USE_GESTURES,
	/* activate up to 4 groups
	 * and set active distance
	 */
	.gest_set = CY_GEST_GRP1 | CY_GEST_GRP2 |
				CY_GEST_GRP3 | CY_GEST_GRP4 |
				CY_ACT_DIST,
	/* change act_intrvl to customize the Active power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.act_intrvl = CY_ACT_INTRVL_DFLT,
	/* change tch_tmout to customize the touch timeout for the
	 * Active power state for Operating mode
	 */
	.tch_tmout = CY_TCH_TMOUT_DFLT,
	/* change lp_intrvl to customize the Low Power power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.lp_intrvl = CY_LP_INTRVL_DFLT,
	.resume = cyttsp_platform_resume,
	.init = cyttsp_platform_init,
};

static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm8058_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_S3,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};
	int	rc = -EINVAL;
	int	id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8058_gpio_config(id - 1, &pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8058_gpio_config(%d): rc=%d\n",
				       __func__, id, rc);
		}
		break;

	case 3:
		id = PM_PWM_LED_KPD;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;

	case 4:
		id = PM_PWM_LED_0;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 40;
		break;
/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_FIH_FBX_PWM
    case 5:
        id = PM_PWM_LED_1;
        mode = PM_PWM_CONF_PWM2;
        max_mA = 40;
        break;

    case 6:
        id = PM_PWM_LED_2;
        mode = PM_PWM_CONF_PWM3;
        max_mA = 40;
        break;
#else
#ifdef CONFIG_FIH_PROJECT_SF4Y6	//OwenHung Add+
    case 5:
        id = PM_PWM_LED_1;
        mode = PM_PWM_CONF_PWM2;
        max_mA = 10;
        break;

    case 6:
        id = PM_PWM_LED_2;
        mode = PM_PWM_CONF_PWM3;
        max_mA = 10;
        break;		
#else
	case 5:
		id = PM_PWM_LED_2;
		mode = PM_PWM_CONF_PWM2;
		max_mA = 40;
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;
#endif
#endif
/* } Div2-SW2-BSP-FBX-LEDS */
	default:
		break;
	}

	if (ch >= 3 && ch <= 6) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}

	return rc;
}

static int pm8058_pwm_enable(struct pwm_device *pwm, int ch, int on)
{
	int	rc;

	switch (ch) {
	case 7:
		rc = pm8058_pwm_set_dtest(pwm, on);
		if (rc)
			pr_err("%s: pwm_set_dtest(%d): rc=%d\n",
			       __func__, on, rc);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

/* Div2-SW2-BSP-FBX-KEYS { */
#ifdef CONFIG_KEYBOARD_PMIC8058
static const unsigned int fluid_keymap[] = {
	KEY(0, 0, KEY_7),
	KEY(0, 1, KEY_DOWN),
	KEY(0, 2, KEY_UP),
	KEY(0, 3, KEY_RIGHT),
	KEY(0, 4, KEY_ENTER),

	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_SEND),
	KEY(1, 2, KEY_1),
	KEY(1, 3, KEY_4),
	KEY(1, 4, KEY_CLEAR),

	KEY(2, 0, KEY_6),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_8),
	KEY(2, 3, KEY_3),
	KEY(2, 4, KEY_NUMERIC_STAR),

	KEY(3, 0, KEY_9),
	KEY(3, 1, KEY_NUMERIC_POUND),
	KEY(3, 2, KEY_0),
	KEY(3, 3, KEY_2),
	KEY(3, 4, KEY_END),

	KEY(4, 0, KEY_BACK),
	KEY(4, 1, KEY_HOME),
	KEY(4, 2, KEY_MENU),
	KEY(4, 3, KEY_VOLUMEUP),
	KEY(4, 4, KEY_VOLUMEDOWN),
};

//Div2D5-AriesHuang-Keypad FTM/Driver porting +{
#ifdef CONFIG_FIH_PROJECT_SF4V5
static const unsigned int surf_keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(0, 2, KEY_SILENT),
	KEY(0, 3, KEY_RIGHT),
	KEY(0, 4, KEY_ENTER),
	
	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_SEND),
	KEY(1, 2, KEY_1),
	KEY(1, 3, KEY_4),
	KEY(1, 4, KEY_CLEAR),

	KEY(2, 0, KEY_6),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_8),
	KEY(2, 3, KEY_3),
	KEY(2, 4, KEY_NUMERIC_STAR),

	KEY(3, 0, KEY_9),
	KEY(3, 1, KEY_NUMERIC_POUND),
	KEY(3, 2, KEY_0),
	KEY(3, 3, KEY_2),
	KEY(3, 4, KEY_END),

	KEY(4, 0, KEY_BACK),
	KEY(4, 1, KEY_HOME),
	KEY(4, 2, KEY_MENU),
	KEY(4, 3, KEY_VOLUMEUP),
	KEY(4, 4, KEY_VOLUMEDOWN),		
};
#else
//Div2D5-OwenHung-Keypad FTM/Driver porting+
static const unsigned int surf_keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(0, 2, KEY_MUTE),
	KEY(0, 3, KEY_RIGHT),
	KEY(0, 4, KEY_ENTER),
	
	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_SEND),
	KEY(1, 2, KEY_1),
	KEY(1, 3, KEY_4),
	KEY(1, 4, KEY_CLEAR),

	KEY(2, 0, KEY_6),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_8),
	KEY(2, 3, KEY_3),
	KEY(2, 4, KEY_NUMERIC_STAR),

	KEY(3, 0, KEY_9),
	KEY(3, 1, KEY_NUMERIC_POUND),
	KEY(3, 2, KEY_0),
	KEY(3, 3, KEY_2),
	KEY(3, 4, KEY_END),

	KEY(4, 0, KEY_BACK),
	KEY(4, 1, KEY_HOME),
	KEY(4, 2, KEY_MENU),
	KEY(4, 3, KEY_VOLUMEUP),
	KEY(4, 4, KEY_VOLUMEDOWN),		
};
//Div2D5-OwenHung-Keypad FTM/Driver porting-
#endif
#if 0	//Div2D5-OwenHung-Keypad FTM/Driver porting+
static const unsigned int surf_keymap[] = {
	KEY(0, 0, KEY_7),
	KEY(0, 1, KEY_DOWN),
	KEY(0, 2, KEY_UP),
	KEY(0, 3, KEY_RIGHT),
	KEY(0, 4, KEY_ENTER),
	KEY(0, 5, KEY_L),
	KEY(0, 6, KEY_BACK),
	KEY(0, 7, KEY_M),

	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_SEND),
	KEY(1, 2, KEY_1),
	KEY(1, 3, KEY_4),
	KEY(1, 4, KEY_CLEAR),
	KEY(1, 5, KEY_MSDOS),
	KEY(1, 6, KEY_SPACE),
	KEY(1, 7, KEY_COMMA),

	KEY(2, 0, KEY_6),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_8),
	KEY(2, 3, KEY_3),
	KEY(2, 4, KEY_NUMERIC_STAR),
	KEY(2, 5, KEY_UP),
	KEY(2, 6, KEY_DOWN), /* SYN */
	KEY(2, 7, KEY_LEFTSHIFT),

	KEY(3, 0, KEY_9),
	KEY(3, 1, KEY_NUMERIC_POUND),
	KEY(3, 2, KEY_0),
	KEY(3, 3, KEY_2),
	KEY(3, 4, KEY_SLEEP),
	KEY(3, 5, KEY_F1),
	KEY(3, 6, KEY_F2),
	KEY(3, 7, KEY_F3),

	KEY(4, 0, KEY_BACK),
	KEY(4, 1, KEY_HOME),
	KEY(4, 2, KEY_MENU),
	KEY(4, 3, KEY_VOLUMEUP),
	KEY(4, 4, KEY_VOLUMEDOWN),
	KEY(4, 5, KEY_F4),
	KEY(4, 6, KEY_F5),
	KEY(4, 7, KEY_F6),

	KEY(5, 0, KEY_R),
	KEY(5, 1, KEY_T),
	KEY(5, 2, KEY_Y),
	KEY(5, 3, KEY_LEFTALT),
	KEY(5, 4, KEY_KPENTER),
	KEY(5, 5, KEY_Q),
	KEY(5, 6, KEY_W),
	KEY(5, 7, KEY_E),

	KEY(6, 0, KEY_F),
	KEY(6, 1, KEY_G),
	KEY(6, 2, KEY_H),
	KEY(6, 3, KEY_CAPSLOCK),
	KEY(6, 4, KEY_PAGEUP),
	KEY(6, 5, KEY_A),
	KEY(6, 6, KEY_S),
	KEY(6, 7, KEY_D),

	KEY(7, 0, KEY_V),
	KEY(7, 1, KEY_B),
	KEY(7, 2, KEY_N),
	KEY(7, 3, KEY_MENU), /* REVISIT - SYM */
	KEY(7, 4, KEY_PAGEDOWN),
	KEY(7, 5, KEY_Z),
	KEY(7, 6, KEY_X),
	KEY(7, 7, KEY_C),

	KEY(8, 0, KEY_P),
	KEY(8, 1, KEY_J),
	KEY(8, 2, KEY_K),
	KEY(8, 3, KEY_INSERT),
	KEY(8, 4, KEY_LINEFEED),
	KEY(8, 5, KEY_U),
	KEY(8, 6, KEY_I),
	KEY(8, 7, KEY_O),

	KEY(9, 0, KEY_4),
	KEY(9, 1, KEY_5),
	KEY(9, 2, KEY_6),
	KEY(9, 3, KEY_7),
	KEY(9, 4, KEY_8),
	KEY(9, 5, KEY_1),
	KEY(9, 6, KEY_2),
	KEY(9, 7, KEY_3),

	KEY(10, 0, KEY_F7),
	KEY(10, 1, KEY_F8),
	KEY(10, 2, KEY_F9),
	KEY(10, 3, KEY_F10),
	KEY(10, 4, KEY_FN),
	KEY(10, 5, KEY_9),
	KEY(10, 6, KEY_0),
	KEY(10, 7, KEY_DOT),

	KEY(11, 0, KEY_LEFTCTRL),
	KEY(11, 1, KEY_F11),  /* START */
	KEY(11, 2, KEY_ENTER),
	KEY(11, 3, KEY_SEARCH),
	KEY(11, 4, KEY_DELETE),
	KEY(11, 5, KEY_RIGHT),
	KEY(11, 6, KEY_LEFT),
	KEY(11, 7, KEY_RIGHTSHIFT),
};
#endif 
//Div2D5-AriesHuang-Keypad FTM/Driver porting +}

static struct resource resources_keypad[] = {
	{
		.start	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct matrix_keymap_data surf_keymap_data = {
	.keymap_size	= ARRAY_SIZE(surf_keymap),
	.keymap		= surf_keymap,
};

//Div2D5-AriesHuang-Keypad FTM/Driver porting +{
#ifdef CONFIG_FIH_PROJECT_SF4V5
static struct pmic8058_keypad_data surf_keypad_data = {
	.input_name		= "pm8058-keypad",
	.input_phys_device	= "surf_keypad/input0",
	.num_rows		= 5,
	.num_cols		= 5,
 	.rows_gpio_start    = 8,
	.cols_gpio_start     = 0,
	.debounce_ms       = {8, 10},
	.scan_delay_ms     = 32,
	.row_hold_ns         = 91500,
	.wakeup         	= 1,
	.keymap_data		= &surf_keymap_data,	
};
#else
static struct pmic8058_keypad_data surf_keypad_data = {
	.input_name		= "surf_keypad",
	.input_phys_device	= "surf_keypad/input0",
	.num_rows		= 12,
	.num_cols		= 8,
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.debounce_ms		= {8, 10},
	.scan_delay_ms	= 32,
	.row_hold_ns		= 91500,
	.wakeup			= 1,
	.keymap_data		= &surf_keymap_data,
};
#endif 
//Div2D5-AriesHuang-Keypad FTM/Driver porting +}

static struct matrix_keymap_data fluid_keymap_data = {
	.keymap_size	= ARRAY_SIZE(fluid_keymap),
	.keymap		= fluid_keymap,
};

static struct pmic8058_keypad_data fluid_keypad_data = {
	.input_name		= "fluid-keypad",
	.input_phys_device	= "fluid-keypad/input0",
	.num_rows		= 5,
	.num_cols		= 5,
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.debounce_ms		= {8, 10},
	.scan_delay_ms		= 32,
	.row_hold_ns		= 91500,
	.wakeup			= 1,
	.keymap_data		= &fluid_keymap_data,
};
#endif
/* } Div2-SW2-BSP-FBX-KEYS */

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
	.enable		= pm8058_pwm_enable,
};

/* Put sub devices with fixed location first in sub_devices array */
#define	PM8058_SUBDEV_KPD	0
#define	PM8058_SUBDEV_LED	1

static struct pm8058_gpio_platform_data pm8058_gpio_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, 0),
	.init		= pm8058_gpios_init,
};

static struct pm8058_gpio_platform_data pm8058_mpp_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS),
	.irq_base	= PM8058_MPP_IRQ(PMIC8058_IRQ_BASE, 0),
};

/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_PMIC8058
static struct pmic8058_led pmic8058_ffa_leds[] = {
	[0] = {
		.name		= "keyboard-backlight",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},
};

static struct pmic8058_leds_platform_data pm8058_ffa_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_ffa_leds),
	.leds	= pmic8058_ffa_leds,
};

static struct pmic8058_led pmic8058_surf_leds[] = {
	[0] = {
		.name		= "keyboard-backlight",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},
	[1] = {
		.name		= "voice:red",
		.max_brightness = 20,
		.id		= PMIC8058_ID_LED_0,
	},
	[2] = {
		.name		= "wlan:green",
		.max_brightness = 20,
		.id		= PMIC8058_ID_LED_2,
	},
};
#endif
/* Div2-SW2-BSP-FBX-LEDS { */

/* Div2-SW2-BSP-FBX-VIB { */
#ifdef CONFIG_PMIC8058_VIBRATOR
static struct pmic8058_vibrator_pdata pmic_vib_pdata = {
    .initial_vibrate_ms  = 0,
    .level_mV = 3000,
    .max_timeout_ms = 15000,
};
#endif
/* } Div2-SW2-BSP-FBX-VIB */

static struct mfd_cell pm8058_subdevs[] = {
/* Div2-SW2-BSP-FBX-KEYS, Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_KEYBOARD_PMIC8058
	{	.name = "pm8058-keypad",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(resources_keypad),
		.resources	= resources_keypad,
	},
#endif
#ifdef CONFIG_LEDS_PMIC8058
	{	.name = "pm8058-led",
		.id		= -1,
	},
#endif
/* } Div2-SW2-BSP-FBX-KEYS, Div2-SW2-BSP-FBX-LEDS  */
	{	.name = "pm8058-gpio",
		.id		= -1,
		.platform_data	= &pm8058_gpio_data,
		.data_size	= sizeof(pm8058_gpio_data),
	},
	{	.name = "pm8058-mpp",
		.id		= -1,
		.platform_data	= &pm8058_mpp_data,
		.data_size	= sizeof(pm8058_mpp_data),
	},
	{	.name = "pm8058-pwm",
		.id		= -1,
		.platform_data	= &pm8058_pwm_data,
		.data_size	= sizeof(pm8058_pwm_data),
	},
	{	.name = "pm8058-nfc",
		.id		= -1,
	},
	{	.name = "pm8058-upl",
		.id		= -1,
	},
/* Div2-SW2-BSP-FBX-VIB { */
#ifdef CONFIG_PMIC8058_VIBRATOR
    {   .name = "pm8058-vib",
        .id     = -1,
        .platform_data = &pmic_vib_pdata,
        .data_size     = sizeof(pmic_vib_pdata),
    },
#endif
/* } Div2-SW2-BSP-FBX-VIB */
};

/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_PMIC8058
static struct pmic8058_leds_platform_data pm8058_surf_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_surf_leds),
	.leds	= pmic8058_surf_leds,
};
#endif
/* } Div2-SW2-BSP-FBX-LEDS */

/* Div1-FW3-BSP-AUDIO */
#ifdef CONFIG_FIH_FBX_AUDIO
static struct gpio_switch_platform_data headset_sensor_device_data = {
    .name = "headset_sensor",
    .gpio = 26,
    .name_on = "",
    .name_off = "",
    .state_on = "",
    .state_off = "",
};
    
static struct platform_device headset_sensor_device = {
    .name = "switch_gpio",
    .id = -1,
    .dev = { .platform_data = &headset_sensor_device_data },
};
#endif

static struct pm8058_platform_data pm8058_7x30_data = {
	.irq_base = PMIC8058_IRQ_BASE,

	.num_subdevs = ARRAY_SIZE(pm8058_subdevs),
	.sub_devices = pm8058_subdevs,
};

static struct i2c_board_info pm8058_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8058-core", 0),
		.irq = MSM_GPIO_TO_INT(PMIC_GPIO_INT),
		.platform_data = &pm8058_7x30_data,
	},
};

static struct i2c_board_info cy8info[] __initdata = {
	{
		I2C_BOARD_INFO(CY_I2C_NAME, 0x24),
		.platform_data = &cyttsp_data,
#ifndef CY_USE_TIMER
		.irq = MSM_GPIO_TO_INT(CYTTSP_TS_GPIO_IRQ),
#endif /* CY_USE_TIMER */
	},
};

static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
//Div2-SW6-MM-HL-Camera-BringUp-00+{    
#ifdef CONFIG_FIH_MT9P111
        {
            I2C_BOARD_INFO("mt9p111", 0x78 >> 1),
        },
#endif
#ifdef CONFIG_FIH_HM0356
        {
            I2C_BOARD_INFO("hm0356", 0x68 >> 1),
        },
#endif
//Div2-SW6-MM-HL-Camera-BringUp-00+} 
#ifdef CONFIG_FIH_HM0357
        {
            I2C_BOARD_INFO("hm0357", 0x60 >> 1),
        },
#endif

#ifdef CONFIG_FIH_TCM9001MD
    {
        I2C_BOARD_INFO("tcm9001md", 0x7C >> 1),
    },
#endif


#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x78 >> 1),
	},
#endif
#ifdef CONFIG_S5K3E2FX
	{
		I2C_BOARD_INFO("s5k3e2fx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9P012
	{
		I2C_BOARD_INFO("mt9p012", 0x6C >> 1),
	},
#endif
#ifdef CONFIG_VX6953
	{
		I2C_BOARD_INFO("vx6953", 0x20),
	},
#endif
#ifdef CONFIG_SN12M0PZ
	{
		I2C_BOARD_INFO("sn12m0pz", 0x34 >> 1),
	},
#endif
#if defined(CONFIG_MT9T013) || defined(CONFIG_SENSORS_MT9T013)
	{
		I2C_BOARD_INFO("mt9t013", 0x6C),
	},
#endif
};

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_vcm_gpio_table[] = {
//GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VCM */
};

static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
    //GPIO_CFG(0,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FC PWRDWN *///Div2-SW6-MM-MC-PortingCameraFor2030FTM-00-
    //GPIO_CFG(3,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FLASH *///Div2-SW6-MM-MC-PortingCameraFor2030FTM-00-
    GPIO_CFG(4,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
    GPIO_CFG(5,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
    GPIO_CFG(6,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
    GPIO_CFG(7,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
    GPIO_CFG(8,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
    GPIO_CFG(9,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
    GPIO_CFG(10, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
    GPIO_CFG(11, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
    GPIO_CFG(12, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
    GPIO_CFG(13, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
    GPIO_CFG(14, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
    GPIO_CFG(15, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* MCLK */
#ifdef CONFIG_FIH_AAT1272
    #ifdef CONFIG_FIH_PROJECT_SF4Y6
    	GPIO_CFG(30, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FLASHLED_DRV_EN PIN*/
    #else
        GPIO_CFG(39, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FLASHLED_DRV_EN PIN*/ 
	#endif
#endif

#ifdef CONFIG_FIH_PROJECT_FB400
    GPIO_CFG(50, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* SWITCH PIN*/
#endif
//Div2-SW6-MM-MC-ImplementCameraStandbyMode-00-{
//#ifdef CONFIG_FIH_TCM9001MD
//GPIO_CFG(19,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FC RESET */
//GPIO_CFG(98,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* Power Enable */
//#endif
//Div2-SW6-MM-MC-ImplementCameraStandbyMode-00-}

};

static uint32_t camera_on_vcm_gpio_table[] = {
//GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* VCM */
};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
    //GPIO_CFG(0,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FC PWRDWN *///Div2-SW6-MM-MC-PortingCameraFor2030FTM-00-
    //GPIO_CFG(3,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* FLASH *///Div2-SW6-MM-MC-PortingCameraFor2030FTM-00-
    GPIO_CFG(4,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
    GPIO_CFG(5,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
    GPIO_CFG(6,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
    GPIO_CFG(7,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
    GPIO_CFG(8,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
    GPIO_CFG(9,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
    GPIO_CFG(10, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
    GPIO_CFG(11, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
    GPIO_CFG(12, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
    GPIO_CFG(13, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
    GPIO_CFG(14, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
    GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* MCLK */
#ifdef CONFIG_FIH_AAT1272
	//SW5-Multimedia-TH-MT9P111forV6-00+{
	#ifdef CONFIG_FIH_PROJECT_SF4Y6
    	GPIO_CFG(30, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FLASHLED_DRV_EN PIN*/
    #else
        GPIO_CFG(39, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* FLASHLED_DRV_EN PIN*/ 
	#endif
	//SW5-Multimedia-TH-MT9P111forV6-00+} 
#endif
#ifdef CONFIG_FIH_PROJECT_FB400
    GPIO_CFG(50, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* SWITCH PIN*/
#endif
//Div2-SW6-MM-MC-ImplementCameraStandbyMode-00-{
//#ifdef CONFIG_FIH_TCM9001MD
//GPIO_CFG(19,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* FC RESET */
//GPIO_CFG(98,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* Power Enable */
//#endif
//Div2-SW6-MM-MC-ImplementCameraStandbyMode-00-}

};

static uint32_t camera_off_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* FLUID: Disable CAMIF_STANDBY */
	GPIO_CFG(143, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
};

static uint32_t camera_on_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* FLUID: Disable CAMIF_STANDBY */
	GPIO_CFG(143, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA)
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}
static void config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));

	if (adie_get_detected_codec_type() != TIMPANI_ID)
		/* GPIO1 is shared also used in Timpani RF card so
		only configure it for non-Timpani RF card */
		config_gpio_table(camera_on_vcm_gpio_table,
			ARRAY_SIZE(camera_on_vcm_gpio_table));

	if (machine_is_msm7x30_fluid()) {
		config_gpio_table(camera_on_gpio_fluid_table,
			ARRAY_SIZE(camera_on_gpio_fluid_table));
	}
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));

	if (adie_get_detected_codec_type() != TIMPANI_ID)
		/* GPIO1 is shared also used in Timpani RF card so
		only configure it for non-Timpani RF card */
		config_gpio_table(camera_off_vcm_gpio_table,
			ARRAY_SIZE(camera_off_vcm_gpio_table));

	if (machine_is_msm7x30_fluid()) {
		config_gpio_table(camera_off_gpio_fluid_table,
			ARRAY_SIZE(camera_off_gpio_fluid_table));
	}
}

struct resource msm_camera_resources[] = {
	{
		.start	= 0xA6000000,
		.end	= 0xA6000000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
};

struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PWM,
	._fsrc.pwm_src.freq  = 1000,
	._fsrc.pwm_src.max_load = 300,
	._fsrc.pwm_src.low_load = 30,
	._fsrc.pwm_src.high_load = 100,
	._fsrc.pwm_src.channel = 7,
};
//Div2-SW6-MM-HL-Camera-BringUp-00+{
#ifdef CONFIG_FIH_MT9P111
static struct msm_camera_sensor_flash_data flash_mt9p111 = {
    .flash_type = MSM_CAMERA_FLASH_LED,
    .flash_src  = &msm_flash_src
};
//Div6D1-CL-Camera-SensorInfo-01+{
static struct msm_parameters_data parameters_mt9p111 = {

};
//Div6D1-CL-Camera-SensorInfo-01+}
static struct msm_camera_sensor_info msm_camera_sensor_mt9p111_data = {
    .sensor_name    = "mt9p111",
    .sensor_reset   = 1,
    .sensor_pwd     = 2,
    .sensor_Orientation = MSM_CAMERA_SENSOR_ORIENTATION_90,//Div6D1-CL-Camera-SensorInfo-00+
    .vcm_pwd        = 0,
    .pdata          = &msm_camera_device_data,
    .resource       = msm_camera_resources,
    .num_resources  = ARRAY_SIZE(msm_camera_resources),
    .flash_data     = &flash_mt9p111,
    .parameters_data     = &parameters_mt9p111,//Div6D1-CL-Camera-SensorInfo-01+
    .csi_if         = 0,

        /* Declare for camea pins */
    .MCLK_PIN =15 ,
    .mclk_sw_pin = 0xffff,
    .pwdn_pin = 0xffff,
    .rst_pin = 0xffff,
    .vga_pwdn_pin = 0xffff,
    .vga_rst_pin = 0xffff,
    .vga_power_en_pin = 0xffff,
    .standby_pin = 0xffff,
    .GPIO_FLASHLED = 0xffff,
    .GPIO_FLASHLED_DRV_EN = 0xffff,

    /* Declare for camera power */
    .AF_pmic_en_pin= 0xffff,
    .cam_v2p8_en_pin = 0xffff,
    .cam_vreg_vddio_id = "NoVreg",
    .cam_vreg_acore_id = "NoVreg",

    //SW5-Multimedia-TH-FlashModeSetting-01+{
    /* Flash LED setting */
    .flash_target_addr = 0xffff,//Div2-SW6-MM-MC-FixCameraCurrentLeakage-02+
    .flash_target = 0xffff,
    .flash_bright = 0xffff,
    .flash_main_waittime = 0,
    .flash_main_starttime = 0,
    .flash_second_waittime = 0,
    //SW5-Multimedia-TH-FlashModeSetting-01+}

    //SW5-Multimedia-TH-MT9P111ReAFTest-00+{
    .fast_af_retest_target = 0xffff
    //SW5-Multimedia-TH-MT9P111ReAFTest-00+}
};

static struct platform_device msm_camera_sensor_mt9p111 = {
    .name      = "msm_camera_mt9p111",
    .dev       = {
        .platform_data = &msm_camera_sensor_mt9p111_data,
    },
};
#endif

#ifdef CONFIG_FIH_HM0356
static struct msm_camera_sensor_flash_data flash_hm0356 = {
    .flash_type = MSM_CAMERA_FLASH_LED,
    .flash_src  = &msm_flash_src
};
//Div6D1-CL-Camera-SensorInfo-01+{
static struct msm_parameters_data parameters_hm0356 = {

};
//Div6D1-CL-Camera-SensorInfo-01+}
static struct msm_camera_sensor_info msm_camera_sensor_hm0356_data = {
    .sensor_name    = "hm0356",
    .sensor_pwd     = 0,
    .sensor_Orientation = MSM_CAMERA_SENSOR_ORIENTATION_0,//Div6D1-CL-Camera-SensorInfo-00+
    .pdata          = &msm_camera_device_data,
    .resource       = msm_camera_resources,
    .num_resources  = ARRAY_SIZE(msm_camera_resources),
    .flash_data     = &flash_hm0356,
    .parameters_data     = &parameters_hm0356,//Div6D1-CL-Camera-SensorInfo-01+
    .csi_if         = 0,
            /* Declare for camea pins */
    .MCLK_PIN =15 ,
    .mclk_sw_pin = 0xffff,
    .pwdn_pin = 0xffff,
    .rst_pin = 0xffff,
    .vga_pwdn_pin = 0xffff,
    .vga_rst_pin = 0xffff,
    .vga_power_en_pin = 0xffff,
    .standby_pin = 0xffff,
    .GPIO_FLASHLED = 0xffff,
    .GPIO_FLASHLED_DRV_EN = 0xffff,

    /* Declare for camera power */
    .AF_pmic_en_pin= 0xffff,
    .cam_v2p8_en_pin = 0xffff,
    .cam_vreg_vddio_id = "NoVreg",
    .cam_vreg_acore_id = "NoVreg"
};

static struct platform_device msm_camera_sensor_hm0356 = {
    .name      = "msm_camera_hm0356",
    .dev       = {
        .platform_data = &msm_camera_sensor_hm0356_data,
    },
};
#endif
//Div2-SW6-MM-HL-Camera-BringUp-00+}
#ifdef CONFIG_FIH_HM0357
static struct msm_camera_sensor_flash_data flash_hm0357 = {
    .flash_type = MSM_CAMERA_FLASH_LED,
    .flash_src  = &msm_flash_src
};

static struct msm_parameters_data parameters_hm0357 = {

};

static struct msm_camera_sensor_info msm_camera_sensor_hm0357_data = {
    .sensor_name    = "hm0357",
    .sensor_pwd     = 0,
    .sensor_Orientation = MSM_CAMERA_SENSOR_ORIENTATION_90,
    .pdata          = &msm_camera_device_data,
    .resource       = msm_camera_resources,
    .num_resources  = ARRAY_SIZE(msm_camera_resources),
    .flash_data     = &flash_hm0357,
    .parameters_data     = &parameters_hm0357,
    .csi_if         = 0,
            /* Declare for camea pins */
    .MCLK_PIN =15 ,
    .mclk_sw_pin = 0xffff,
    .pwdn_pin = 0xffff,
    .rst_pin = 0xffff,
    .vga_pwdn_pin = 0xffff,
    .vga_rst_pin = 0xffff,
    .vga_power_en_pin = 0xffff,
    .standby_pin = 0xffff,
    .GPIO_FLASHLED = 0xffff,
    .GPIO_FLASHLED_DRV_EN = 0xffff,

    /* Declare for camera power */
    .AF_pmic_en_pin= 0xffff,
    .cam_v2p8_en_pin = 0xffff,
    .cam_vreg_vddio_id = "NoVreg",
    .cam_vreg_acore_id = "NoVreg"
};

static struct platform_device msm_camera_sensor_hm0357 = {
    .name      = "msm_camera_hm0357",
    .dev       = {
        .platform_data = &msm_camera_sensor_hm0357_data,
    },
};
#endif

//Div2-SW6-MM-MC-Camera-BringUpForSF5-00+{
#ifdef CONFIG_FIH_TCM9001MD
static struct msm_camera_sensor_flash_data flash_tcm9001md = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

//Div2-SW6-MM-MC-BringUpFrontCameraForOS-00+{
static struct msm_parameters_data parameters_tcm9001md = {

};
//Div2-SW6-MM-MC-BringUpFrontCameraForOS-00+}
static struct msm_camera_sensor_info msm_camera_sensor_tcm9001md_data = {
	.sensor_name    = "tcm9001md",
	.sensor_reset   = 19,
	.sensor_pwd     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_tcm9001md,	
    .parameters_data     = &parameters_tcm9001md,//Div2-SW6-MM-MC-BringUpFrontCameraForOS-00+
	.csi_if         = 0,
	            /* Declare for camea pins */
    .MCLK_PIN =15 ,
    .mclk_sw_pin = 0xffff,
    .pwdn_pin = 0xffff,
    .rst_pin = 0xffff,
    .vga_pwdn_pin = 0xffff,
    .vga_rst_pin = 0xffff,
    .vga_power_en_pin = 0xffff,
    .standby_pin = 0xffff,
    .GPIO_FLASHLED = 0xffff,
    .GPIO_FLASHLED_DRV_EN = 0xffff,

    /* Declare for camera power */
    .AF_pmic_en_pin= 0xffff,
    .cam_v2p8_en_pin = 0xffff,
    .cam_vreg_vddio_id = "NoVreg",
    .cam_vreg_acore_id = "NoVreg"
};

static struct platform_device msm_camera_sensor_tcm9001md = {
	.name      = "msm_camera_tcm9001md",
	.dev       = {
		.platform_data = &msm_camera_sensor_tcm9001md_data,
	},
};
#endif
//Div2-SW6-MM-MC-Camera-BringUpForSF5-00+}

#ifdef CONFIG_MT9D112
static struct msm_camera_sensor_flash_data flash_mt9d112 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9d112_data = {
	.sensor_name    = "mt9d112",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9d112,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_mt9d112 = {
	.name      = "msm_camera_mt9d112",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d112_data,
	},
};
#endif

#ifdef CONFIG_S5K3E2FX
static struct msm_camera_sensor_flash_data flash_s5k3e2fx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3e2fx_data = {
	.sensor_name    = "s5k3e2fx",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_s5k3e2fx,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_s5k3e2fx = {
	.name      = "msm_camera_s5k3e2fx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k3e2fx_data,
	},
};
#endif

#ifdef CONFIG_MT9P012
static struct msm_camera_sensor_flash_data flash_mt9p012 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_data = {
	.sensor_name    = "mt9p012",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9p012,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_mt9p012 = {
	.name      = "msm_camera_mt9p012",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_data,
	},
};
#endif
#ifdef CONFIG_VX6953
static struct msm_camera_sensor_flash_data flash_vx6953 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};
static struct msm_camera_sensor_info msm_camera_sensor_vx6953_data = {
	.sensor_name    = "vx6953",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable		= 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_vx6953,
	.csi_if         = 1
};
static struct platform_device msm_camera_sensor_vx6953 = {
	.name  	= "msm_camera_vx6953",
	.dev   	= {
		.platform_data = &msm_camera_sensor_vx6953_data,
	},
};
#endif

#ifdef CONFIG_SN12M0PZ
static struct msm_camera_sensor_flash_data flash_sn12m0pz = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};
static struct msm_camera_sensor_info msm_camera_sensor_sn12m0pz_data = {
	.sensor_name    = "sn12m0pz",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_sn12m0pz,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_sn12m0pz = {
	.name      = "msm_camera_sn12m0pz",
	.dev       = {
		.platform_data = &msm_camera_sensor_sn12m0pz_data,
	},
};
#endif

#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9t013,
	.csi_if         = 1
};

static struct platform_device msm_camera_sensor_mt9t013 = {
	.name      = "msm_camera_mt9t013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t013_data,
	},
};
#endif

#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start  = 0xA3A00000,
		.end    = 0xA3A00000 + 0x0150 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = INT_JPEG,
		.end    = INT_JPEG,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name           = "msm_gemini",
	.resource       = msm_gemini_resources,
	.num_resources  = ARRAY_SIZE(msm_gemini_resources),
};
#endif

#ifdef CONFIG_MSM_VPE
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0xAD200000,
		.end	= 0xAD200000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_vpe_device = {
       .name = "msm_vpe",
       .id   = 0,
       .num_resources = ARRAY_SIZE(msm_vpe_resources),
       .resource = msm_vpe_resources,
};
#endif

//Div2-SW6-MM-MC-FixCameraCurrentLeakage-02*{
void camera_sensor_hwpin_init(void)
{
    int pid  = 0;
    int phid = 0;
    
    pid = fih_get_product_id();
    phid = fih_get_product_phase();

#ifdef CONFIG_FIH_MT9P111
    if (pid == Product_SF5) 
    {
        /* Declare for camea pins */
        if (phid <= Product_PR2)
        {
            msm_camera_sensor_mt9p111_data.rst_pin = 1;
        }
        else
        {
            msm_camera_sensor_mt9p111_data.rst_pin = 120;
        }

        msm_camera_sensor_mt9p111_data.pwdn_pin = 2;
        msm_camera_sensor_mt9p111_data.vga_pwdn_pin = 0;
        msm_camera_sensor_mt9p111_data.standby_pin=33;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED = 3;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED_DRV_EN = 39;
        
        /* Declare for camera power */
        msm_camera_sensor_mt9p111_data.cam_vreg_vddio_id = "gp7";
        msm_camera_sensor_mt9p111_data.cam_vreg_acore_id = "gp10";
        
        /* Flash LED setting */
        msm_camera_sensor_mt9p111_data.flash_target_addr = 0xB80C;
        msm_camera_sensor_mt9p111_data.flash_target = 0x07BE;
        msm_camera_sensor_mt9p111_data.flash_bright = 0x20;
        msm_camera_sensor_mt9p111_data.flash_main_waittime = 150;
        msm_camera_sensor_mt9p111_data.flash_main_starttime = 150;
        msm_camera_sensor_mt9p111_data.flash_second_waittime = 200;
        /*Fast AF*/
        msm_camera_sensor_mt9p111_data.fast_af_retest_target = 0x2000;
    }
    else if (pid == Product_SF6) 
    {
        msm_camera_sensor_mt9p111_data.sensor_Orientation=MSM_CAMERA_SENSOR_ORIENTATION_270;
        /* Declare for camea pins */
        msm_camera_sensor_mt9p111_data.rst_pin = 3;
        msm_camera_sensor_mt9p111_data.pwdn_pin = 2;
        msm_camera_sensor_mt9p111_data.vga_pwdn_pin = 0;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED = 164;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED_DRV_EN = 30;
        
        /* Declare for camera power */
        msm_camera_sensor_mt9p111_data.cam_vreg_vddio_id = "gp7";
        msm_camera_sensor_mt9p111_data.cam_vreg_acore_id = "gp10";
        
        /* Flash LED setting */
        msm_camera_sensor_mt9p111_data.flash_target_addr = 0xB80C;
        msm_camera_sensor_mt9p111_data.flash_target = 0x2E00;
        msm_camera_sensor_mt9p111_data.flash_bright = 0x20;
        msm_camera_sensor_mt9p111_data.flash_main_waittime = 100;
        msm_camera_sensor_mt9p111_data.flash_main_starttime = 200;
        msm_camera_sensor_mt9p111_data.flash_second_waittime = 200;
        /*Fast AF*/
        msm_camera_sensor_mt9p111_data.fast_af_retest_target = 0x3800;
    }
    else if (IS_SF8_SERIES_PRJ())//Div2-SW6-MM-MC-ImplementCameraFTMforSF8Serials-00*
    {
        /* Declare for camea pins */
        if (pid == Product_SF8 && phid < Product_PR2)
        {
            msm_camera_sensor_mt9p111_data.rst_pin = 1;
            msm_camera_sensor_mt9p111_data.vga_pwdn_pin = 0;
        }
        else
        {
            msm_camera_sensor_mt9p111_data.rst_pin = 173;
            msm_camera_sensor_mt9p111_data.vga_pwdn_pin = 174;
        }

        msm_camera_sensor_mt9p111_data.pwdn_pin = 2;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED = 177;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED_DRV_EN = 3;

        /* Declare for camera power */
        msm_camera_sensor_mt9p111_data.cam_v2p8_en_pin = 120;
        msm_camera_sensor_mt9p111_data.cam_vreg_vddio_id = "gp10";
        msm_camera_sensor_mt9p111_data.cam_vreg_acore_id = "NoVreg";
        
        /* Flash LED setting */
        msm_camera_sensor_mt9p111_data.flash_target_addr = 0x3012;
        msm_camera_sensor_mt9p111_data.flash_target = 0x9C4;
        msm_camera_sensor_mt9p111_data.flash_bright = 0x30;
        msm_camera_sensor_mt9p111_data.flash_main_waittime = 100;
        msm_camera_sensor_mt9p111_data.flash_main_starttime = 200;
        msm_camera_sensor_mt9p111_data.flash_second_waittime = 200;
        /*Fast AF*/
        msm_camera_sensor_mt9p111_data.fast_af_retest_target = 0x2000;
    }
    else// For FBx and FDx series project
    {
        /* Declare for camea pins */
        msm_camera_sensor_mt9p111_data.rst_pin = 1;
        msm_camera_sensor_mt9p111_data.pwdn_pin = 2;
        msm_camera_sensor_mt9p111_data.vga_pwdn_pin = 0;
        msm_camera_sensor_mt9p111_data.standby_pin=33;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED = 3;
        msm_camera_sensor_mt9p111_data.GPIO_FLASHLED_DRV_EN = 39;
        msm_camera_sensor_mt9p111_data.AF_pmic_en_pin=16;
        msm_camera_sensor_mt9p111_data.mclk_sw_pin = 50;
            
        /* Declare for camera power */
        msm_camera_sensor_mt9p111_data.cam_vreg_vddio_id = "gp7";
        msm_camera_sensor_mt9p111_data.cam_vreg_acore_id = "gp10";

        /* Flash LED setting */
        msm_camera_sensor_mt9p111_data.flash_target_addr = 0x3012;
        msm_camera_sensor_mt9p111_data.flash_target = 0x7AA;
        msm_camera_sensor_mt9p111_data.flash_bright = 0x14;
        msm_camera_sensor_mt9p111_data.flash_main_waittime = 300;
        msm_camera_sensor_mt9p111_data.flash_main_starttime = 90;
        msm_camera_sensor_mt9p111_data.flash_second_waittime = 200;
        /*Fast AF*/
        msm_camera_sensor_mt9p111_data.fast_af_retest_target = 0x2000;
        if(pid ==Product_FD1 )
        {
            msm_camera_sensor_mt9p111_data.flash_target = 0x7D0;
            msm_camera_sensor_mt9p111_data.flash_main_starttime = 90;
            msm_camera_sensor_mt9p111_data.flash_main_waittime = 700;
        }
    }

#endif

#ifdef CONFIG_FIH_HM0356
    /*Setting camera pins */
    if (IS_SF8_SERIES_PRJ())
    {
        if (pid == Product_SF8 && phid < Product_PR2)
        {
            msm_camera_sensor_hm0356_data.rst_pin = 1;
            msm_camera_sensor_hm0356_data.vga_pwdn_pin = 0;
        }
        else
        {
            msm_camera_sensor_hm0356_data.rst_pin = 173;
            msm_camera_sensor_hm0356_data.vga_pwdn_pin = 174;
        }

        msm_camera_sensor_hm0356_data.pwdn_pin = 2;
        msm_camera_sensor_hm0356_data.mclk_sw_pin = 0xffff;

        /* Declare for camera power */
        msm_camera_sensor_hm0356_data.cam_v2p8_en_pin = 120;
        msm_camera_sensor_hm0356_data.cam_vreg_vddio_id="gp10";
        msm_camera_sensor_hm0356_data.cam_vreg_acore_id="NoVreg";
    }
    else if (pid == Product_SF6) 
    {
        msm_camera_sensor_hm0356_data.sensor_Orientation = MSM_CAMERA_SENSOR_ORIENTATION_180,
        msm_camera_sensor_hm0356_data.rst_pin = 3;
        msm_camera_sensor_hm0356_data.pwdn_pin = 2;
        msm_camera_sensor_hm0356_data.vga_pwdn_pin = 0;
        msm_camera_sensor_hm0356_data.cam_vreg_vddio_id = "gp7";
        msm_camera_sensor_hm0356_data.cam_vreg_acore_id = "gp10";
    }
    else// For FBx and FDx series project
    {
        msm_camera_sensor_hm0356_data.rst_pin = 1;
        msm_camera_sensor_hm0356_data.pwdn_pin = 2;
        msm_camera_sensor_hm0356_data.vga_pwdn_pin = 0;
        msm_camera_sensor_hm0356_data.mclk_sw_pin = 50;
        msm_camera_sensor_hm0356_data.cam_vreg_vddio_id = "gp7";
        msm_camera_sensor_hm0356_data.cam_vreg_acore_id = "gp10";
    }
#endif
#ifdef CONFIG_FIH_HM0357
    /*Setting camera pins */
    if (IS_SF8_SERIES_PRJ())
    {
        if (pid == Product_SF8 && phid < Product_PR2)
        {
             msm_camera_sensor_hm0357_data.rst_pin = 1;
            msm_camera_sensor_hm0357_data.vga_pwdn_pin = 0;
        }
        else
        {
            msm_camera_sensor_hm0357_data.rst_pin = 173;
            msm_camera_sensor_hm0357_data.vga_pwdn_pin = 174;
        }
        
        msm_camera_sensor_hm0357_data.pwdn_pin = 2;
        msm_camera_sensor_hm0357_data.mclk_sw_pin = 0xffff;

        /* Declare for camera power */
        msm_camera_sensor_hm0357_data.cam_v2p8_en_pin = 120;
        msm_camera_sensor_hm0357_data.cam_vreg_vddio_id="gp10";
        msm_camera_sensor_hm0357_data.cam_vreg_acore_id="NoVreg";
    }
    else if (pid == Product_SF6) 
    {
        msm_camera_sensor_hm0357_data.sensor_Orientation = MSM_CAMERA_SENSOR_ORIENTATION_180,
        msm_camera_sensor_hm0357_data.rst_pin = 3;
        msm_camera_sensor_hm0357_data.pwdn_pin = 2;
        msm_camera_sensor_hm0357_data.vga_pwdn_pin = 0;
        msm_camera_sensor_hm0357_data.cam_vreg_vddio_id = "gp7";
        msm_camera_sensor_hm0357_data.cam_vreg_acore_id = "gp10";
    }
    else// For FBx and FDx series project
    {
        msm_camera_sensor_hm0357_data.rst_pin = 1;
        msm_camera_sensor_hm0357_data.pwdn_pin = 2;
        msm_camera_sensor_hm0357_data.vga_pwdn_pin = 0;
        msm_camera_sensor_hm0357_data.mclk_sw_pin = 50;
        msm_camera_sensor_hm0357_data.cam_vreg_vddio_id = "gp7";
        msm_camera_sensor_hm0357_data.cam_vreg_acore_id = "gp10";
    }
#endif

#ifdef CONFIG_FIH_TCM9001MD
    if (pid  == Product_SF5)
    {
        /*5M sensor pins*/
        if (phid <= Product_PR2)
        {
            msm_camera_sensor_tcm9001md_data.rst_pin = 1;
        }
        else
        {
            msm_camera_sensor_tcm9001md_data.rst_pin = 120;
        }
        msm_camera_sensor_tcm9001md_data.pwdn_pin = 2;

        /*VGA sensor pins*/
        msm_camera_sensor_tcm9001md_data.vga_rst_pin = 19;
        msm_camera_sensor_tcm9001md_data.vga_pwdn_pin = 0;
        msm_camera_sensor_tcm9001md_data.vga_power_en_pin = 98;
    }
    else
    {
        /*5M sensor pins*/
        if (phid <= Product_PR2)
        {
            msm_camera_sensor_tcm9001md_data.rst_pin = 1;
        }
        else
        {
            msm_camera_sensor_tcm9001md_data.rst_pin = 120;
        }
        msm_camera_sensor_tcm9001md_data.pwdn_pin = 2;

        /*VGA sensor pins*/
        msm_camera_sensor_tcm9001md_data.vga_rst_pin = 19;
        msm_camera_sensor_tcm9001md_data.vga_pwdn_pin = 0;
        msm_camera_sensor_tcm9001md_data.vga_power_en_pin = 98;
    }
#endif

}
//Div2-SW6-MM-MC-FixCameraCurrentLeakage-02*}

#endif /*CONFIG_MSM_CAMERA*/
//MM-RC-ChangeCodingStyle-00*{
#ifdef CONFIG_MSM7KV2_AUDIO
int SPK1_AMP = 36;
int SPK2_AMP = 37;
int HS_AMP = 55;


static uint32_t audio_fluid_icodec_tx_config =
  GPIO_CFG(85, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
static uint32_t audio_pamp_spk1_amp_gpio_config = 0;
#ifdef CONFIG_FIH_FBX_AUDIO
static uint32_t audio_pamp_spk2_amp_gpio_config = 0;
#endif
static uint32_t audio_pamp_hs_amp_gpio_config = 0;
//MM-RC-ChangeCodingStyle-00*}
  
static int __init snddev_poweramp_gpio_init(void)
{
    int rc;
    
    //MM-RC-ChangeCodingStyle-00+{
    int pid =0;
    pid = fih_get_product_id();
	//MM-RC-ChangeCodingStyle-01*{
    if((pid==Product_SF5)||(pid==Product_SF8)||(pid==Product_SFH))
    {
    	SPK1_AMP = 36;
    }
	else if((pid==Product_SF6)||(pid==Product_SH8))
    {
    	SPK1_AMP = 37;
    }
	else
    {
    	SPK1_AMP = 36;
	SPK2_AMP = 37;
    }
	//MM-RC-ChangeCodingStyle-01*}
audio_pamp_spk1_amp_gpio_config =
    GPIO_CFG(SPK1_AMP, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
#ifdef CONFIG_FIH_FBX_AUDIO
audio_pamp_spk2_amp_gpio_config =
    GPIO_CFG(SPK2_AMP, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
#endif
audio_pamp_hs_amp_gpio_config =
    GPIO_CFG(HS_AMP, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
    //MM-RC-ChangeCodingStyle-00+}

    pr_info("snddev_poweramp_gpio_init \n");
    rc = gpio_tlmm_config(audio_pamp_spk1_amp_gpio_config, GPIO_CFG_ENABLE);
    if (rc) {
        printk(KERN_ERR
            "%s: gpio_tlmm_config(%#x)=%d\n",
            __func__, audio_pamp_spk1_amp_gpio_config, rc);
    }
#ifdef CONFIG_FIH_FBX_AUDIO
    rc = gpio_tlmm_config(audio_pamp_spk2_amp_gpio_config, GPIO_CFG_ENABLE);
    if (rc) {
        printk(KERN_ERR
            "%s: gpio_tlmm_config(%#x)=%d\n",
            __func__, audio_pamp_spk2_amp_gpio_config, rc);
    }
#endif
    rc = gpio_tlmm_config(audio_pamp_hs_amp_gpio_config, GPIO_CFG_ENABLE);  
    if (rc) {
        printk(KERN_ERR
            "%s: gpio_tlmm_config(%#x)=%d\n",
            __func__, audio_pamp_hs_amp_gpio_config, rc);
    }
    return rc;
}

void msm_snddev_tx_route_config(void)
{
	int rc;

	pr_debug("%s()\n", __func__);

	if (machine_is_msm7x30_fluid()) {
		rc = gpio_tlmm_config(audio_fluid_icodec_tx_config,
		GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, audio_fluid_icodec_tx_config, rc);
		} else
			gpio_set_value(85, 0);
	}
}

void msm_snddev_tx_route_deconfig(void)
{
	int rc;

	pr_debug("%s()\n", __func__);

	if (machine_is_msm7x30_fluid()) {
		rc = gpio_tlmm_config(audio_fluid_icodec_tx_config,
		GPIO_CFG_DISABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, audio_fluid_icodec_tx_config, rc);
		}
	}
}

void msm_snddev_poweramp_on(void)
{
    //MM-SL-OPControl-00*{
    printk(KERN_ERR "Have let SPK OP been opened after switching path in snddev_icodec_open_rx\n");
    m_SpkAmpOn=true;
 //MM-RC-ChangeCodingStyle-00-{
#if 0
    //gpio_set_value(SPK1_AMP, 1);    /* enable spkr poweramp 1*/
    pr_info("%s: power on amplifier\n", __func__);
    printk(KERN_ERR "GPIO%d:%d \n",SPK1_AMP,gpio_get_value(SPK1_AMP) );
#ifdef CONFIG_FIH_FBX_AUDIO
    //gpio_set_value(SPK2_AMP, 1);    /* enable spkr poweramp 2*/
    printk(KERN_ERR "GPIO%d:%d \n",SPK2_AMP,gpio_get_value(SPK2_AMP) );
#endif
#endif
 //MM-RC-ChangeCodingStyle-00-}
    //MM-SL-OPControl-00*}
}

void msm_snddev_poweramp_off(void)
{
//MM-RC-OPControl-01*{
    printk(KERN_ERR "SPK OP has been closed before switching path\n");

    m_SpkAmpOn=false; //MM-SL-OPControl-00+
 //MM-RC-ChangeCodingStyle-00-{
 #if 0
//    gpio_set_value(SPK1_AMP, 0);    /* disable spkr poweramp 1*/
    pr_info("%s: power off amplifier\n", __func__);
    printk(KERN_ERR "GPIO%d:%d \n",SPK1_AMP,gpio_get_value(SPK1_AMP) );
#ifdef CONFIG_FIH_FBX_AUDIO
 //   gpio_set_value(SPK2_AMP, 0);    /* disable spkr poweramp 2*/
    printk(KERN_ERR "GPIO%d:%d \n",SPK2_AMP,gpio_get_value(SPK2_AMP) );
#endif
#endif
 //MM-RC-ChangeCodingStyle-00-}
//MM-RC-OPControl-01*}

}

/* Div1-FW3-BSP-AUDIO */
#ifndef CONFIG_FIH_FBX_AUDIO
static struct vreg *snddev_vreg_ncp;
#endif

void msm_snddev_hsed_voltage_on(void)
{
#ifndef CONFIG_FIH_FBX_AUDIO
    int rc;
#endif
    //MM-SL-OPControl-00*{
    printk(KERN_ERR "Have let HS OP been opened after switching path in snddev_icodec_open_rx.\n");	
    m_HsAmpOn=true;
 //MM-RC-ChangeCodingStyle-00-{
	#if 0
    pr_info("%s: power on amplifier\n", __func__);
    //gpio_set_value(55, 1);  /* enable hs poweramp GPIO_55*/
    //MM-SL-OPControl-00*}
    printk(KERN_ERR "GPIO55:%d \n",gpio_get_value(55) );
	#endif
 //MM-RC-ChangeCodingStyle-00-}
#ifndef CONFIG_FIH_FBX_AUDIO
    snddev_vreg_ncp = vreg_get(NULL, "ncp");

    if (IS_ERR(snddev_vreg_ncp)) {
        pr_err("%s: vreg_get(%s) failed (%ld)\n",
            __func__, "ncp", PTR_ERR(snddev_vreg_ncp));
        return;
        }
    rc = vreg_enable(snddev_vreg_ncp);
    if (rc)
        pr_err("%s: vreg_enable(ncp) failed (%d)\n", __func__, rc);
#endif
}

void msm_snddev_hsed_voltage_off(void)
{
#ifndef CONFIG_FIH_FBX_AUDIO
    int rc;
#endif
    
    pr_info("%s: power on amplifier\n", __func__);
    //MM-SL-OPControl-00*{
    printk(KERN_ERR "HS OP has been closed before switching path\n");
    //gpio_set_value(55, 0);  /* disable hs poweramp GPIO_55*/
    m_HsAmpOn=false;
    //MM-SL-OPControl-00*}	
    //printk(KERN_ERR "GPIO55:%d \n",gpio_get_value(55) );  //MM-RC-ChangeCodingStyle-00-

#ifndef CONFIG_FIH_FBX_AUDIO
    if (IS_ERR(snddev_vreg_ncp)) {
        pr_err("%s: vreg_get(%s) failed (%ld)\n",
            __func__, "ncp", PTR_ERR(snddev_vreg_ncp));
        return;
        }
    rc = vreg_disable(snddev_vreg_ncp);
    if (rc)
        pr_err("%s: vreg_disable(ncp) failed (%d)\n", __func__, rc);
    vreg_put(snddev_vreg_ncp);
#endif
}

static unsigned aux_pcm_gpio_on[] = {
	GPIO_CFG(138, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DOUT */
	GPIO_CFG(139, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DIN  */
	GPIO_CFG(140, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_SYNC */
	GPIO_CFG(141, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_CLK  */
};

static int __init aux_pcm_gpio_init(void)
{
	int pin, rc;

	pr_info("aux_pcm_gpio_init \n");
	for (pin = 0; pin < ARRAY_SIZE(aux_pcm_gpio_on); pin++) {
		rc = gpio_tlmm_config(aux_pcm_gpio_on[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, aux_pcm_gpio_on[pin], rc);
		}
	}
	return rc;
}

static struct msm_gpio mi2s_clk_gpios[] = {
	{ GPIO_CFG(145, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_SCLK"},
	{ GPIO_CFG(144, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_WS"},
	{ GPIO_CFG(120, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_MCLK_A"},
};

static struct msm_gpio mi2s_rx_data_lines_gpios[] = {
	{ GPIO_CFG(121, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD0_A"},
	{ GPIO_CFG(122, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD1_A"},
	{ GPIO_CFG(123, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD2_A"},
	{ GPIO_CFG(146, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

static struct msm_gpio mi2s_tx_data_lines_gpios[] = {
	{ GPIO_CFG(146, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

int mi2s_config_clk_gpio(void)
{
 //MM-RC-ChangeFMPath-00*{
 if(!IS_SF8_SERIES_PRJ()){
	int rc = 0;

	rc = msm_gpios_request_enable(mi2s_clk_gpios,
			ARRAY_SIZE(mi2s_clk_gpios));
	if (rc) {
		pr_err("%s: enable mi2s clk gpios  failed\n",
					__func__);
		return rc;
	}
}
//MM-RC-ChangeFMPath-00*}
	return 0;
}

int  mi2s_unconfig_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i, rc = 0;
	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		msm_gpios_disable_free(mi2s_tx_data_lines_gpios, 1);
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask) {
			if (sd_line_mask & 0x1)
				msm_gpios_disable_free(
					mi2s_rx_data_lines_gpios + i , 1);
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
						__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_config_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i , rc = 0;
	u8 sd_config_done_mask = 0;

	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		if ((sd_line_mask & MI2S_SD_0) || (sd_line_mask & MI2S_SD_1) ||
		   (sd_line_mask & MI2S_SD_2) || !(sd_line_mask & MI2S_SD_3)) {
			pr_err("%s: can not use SD0 or SD1 or SD2 for TX"
				".only can use SD3. sd_line_mask = 0x%x\n",
				__func__ , sd_line_mask);
			rc = -EINVAL;
		} else {
			rc = msm_gpios_request_enable(mi2s_tx_data_lines_gpios,
							 1);
			if (rc)
				pr_err("%s: enable mi2s gpios for TX failed\n",
					   __func__);
		}
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask && (rc == 0)) {
			if (sd_line_mask & 0x1) {
				rc = msm_gpios_request_enable(
					mi2s_rx_data_lines_gpios + i , 1);
				if (rc) {
					pr_err("%s: enable mi2s gpios for"
					 "RX failed.  SD line = %s\n",
					 __func__,
					 (mi2s_rx_data_lines_gpios + i)->label);
					mi2s_unconfig_data_gpio(DIR_RX,
						sd_config_done_mask);
				} else
					sd_config_done_mask |= (1 << i);
			}
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
			__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_unconfig_clk_gpio(void)
{
 //MM-RC-ChangeFMPath-00*{
 if(!IS_SF8_SERIES_PRJ()){
	msm_gpios_disable_free(mi2s_clk_gpios, ARRAY_SIZE(mi2s_clk_gpios));
 }
  //MM-RC-ChangeFMPath-00*}
	return 0;
}

#endif /* CONFIG_MSM7KV2_AUDIO */

static int __init buses_init(void)
{
	if (gpio_tlmm_config(GPIO_CFG(PMIC_GPIO_INT, 1, GPIO_CFG_INPUT,
				  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config (gpio=%d) failed\n",
		       __func__, PMIC_GPIO_INT);

/* Div2-SW2-BSP-FBX-KEYS { */
#ifdef CONFIG_KEYBOARD_PMIC8058
	if (machine_is_msm7x30_fluid()) {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].platform_data
			= &fluid_keypad_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].data_size
			= sizeof(fluid_keypad_data);
	} else {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].platform_data
			= &surf_keypad_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].data_size
			= sizeof(surf_keypad_data);
	}
#endif
/* } Div2-SW2-BSP-FBX-KEYS */

	i2c_register_board_info(6 /* I2C_SSBI ID */, pm8058_boardinfo,
				ARRAY_SIZE(pm8058_boardinfo));

	return 0;
}

#define TIMPANI_RESET_GPIO	1

static struct vreg *vreg_marimba_1;
static struct vreg *vreg_marimba_2;

static struct msm_gpio timpani_reset_gpio_cfg[] = {
{ GPIO_CFG(TIMPANI_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "timpani_reset"} };

static int config_timpani_reset(void)
{
	int rc;

	rc = msm_gpios_request_enable(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
	if (rc < 0) {
		printk(KERN_ERR
			"%s: msm_gpios_request_enable failed (%d)\n",
				__func__, rc);
	}
	return rc;
}

static unsigned int msm_timpani_setup_power(void)
{
	int rc;

	rc = config_timpani_reset();
	if (rc < 0)
		goto out;

	rc = vreg_enable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
					__func__, rc);
		goto out;
	}
	rc = vreg_enable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
					__func__, rc);
		goto fail_disable_vreg_marimba_1;
	}

	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 1);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
		msm_gpios_free(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
		vreg_disable(vreg_marimba_2);
	} else
		goto out;


fail_disable_vreg_marimba_1:
	vreg_disable(vreg_marimba_1);

out:
	return rc;
};

static void msm_timpani_shutdown_power(void)
{
	int rc;

	rc = vreg_disable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}
	rc = vreg_disable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}

	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 0);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
	}

	msm_gpios_free(timpani_reset_gpio_cfg,
				   ARRAY_SIZE(timpani_reset_gpio_cfg));
};

static unsigned int msm_marimba_setup_power(void)
{
	int rc;

	rc = vreg_enable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}
	rc = vreg_enable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}

out:
	return rc;
};

static void msm_marimba_shutdown_power(void)
{
	int rc;

	rc = vreg_disable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}
	rc = vreg_disable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
};


/* FIHTDC, Div2-SW2-BSP Godfrey, FM { */
#ifdef CONFIG_FIH_FM_LNA
//FB0 add LNA in FM, so we need to control FM_LNA_EN and GPS_FM_LNA_2V8_EN to turn on/off FM LNA.
    #define FM_LNA_EN           78
    //#define GPS_FM_LNA_2V8_EN   79
    static struct msm_gpio fm_gpio_table[] = {
    //{ GPIO_CFG(GPS_FM_LNA_2V8_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
    //    "gps_fm_lna_2v8_en" },
    { GPIO_CFG(FM_LNA_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
        "fm_lna_en" },
    };

#endif
/* } FIHTDC, Div2-SW2-BSP Godfrey, FM  */

static int fm_radio_setup(struct marimba_fm_platform_data *pdata)
{
	int rc;
	uint32_t irqcfg;
	const char *id = "FMPW";

	/* FIHTDC, Div2-SW2-BSP Godfrey, FM { */
	pdata->vreg_gp16 = vreg_get(NULL, "gp16");
    if (IS_ERR(pdata->vreg_gp16)) {
        printk(KERN_ERR "%s: vreg get failed (%ld)\n",
            __func__, PTR_ERR(pdata->vreg_gp16));
        return -1;
    }

	rc = vreg_enable(pdata->vreg_gp16);
    if (rc) {
        printk(KERN_ERR "%s: vreg_enable() = %d \n",
                    __func__, rc);
        return rc;
    }
	/* } FIHTDC, Div2-SW2-BSP Godfrey, FM  */

	pdata->vreg_s2 = vreg_get(NULL, "s2");
	if (IS_ERR(pdata->vreg_s2)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(pdata->vreg_s2));
		return -1;
	}

	rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 1300);
	if (rc < 0) {
		printk(KERN_ERR "%s: voltage level vote failed (%d)\n",
			__func__, rc);
		return rc;
	}

	rc = vreg_enable(pdata->vreg_s2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		return rc;
	}

    /* FIHTDC, Div2-SW2-BSP Godfrey */
	if( ( fih_get_product_id() == Product_FD1 ) && ( ( fih_get_product_phase() != Product_PR1 ) &&
													( fih_get_product_phase() != Product_PR2p5 ) &&
													( fih_get_product_phase() != Product_PR230 ) &&
													( fih_get_product_phase() != Product_PR232 ) &&
													( fih_get_product_phase() != Product_PR3 ) &&
													( fih_get_product_phase() != Product_PR4 ) ) ){
	    rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
						  PMAPP_CLOCK_VOTE_ON);
	}
	else{
	    rc = pmapp_clock_vote(id, QTR8x00_WCN_CLK,
						  PMAPP_CLOCK_VOTE_ON);
	}
	if (rc < 0) {
		printk(KERN_ERR "%s: clock vote failed (%d)\n",
			__func__, rc);
		goto fm_clock_vote_fail;
	}
	irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA);
	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
		rc = -EIO;
		goto fm_gpio_config_fail;

	}

	/* FIHTDC, Div2-SW2-BSP Godfrey, FM { */
    #ifdef CONFIG_FIH_FM_LNA
    //FB0 add LNA in FM, so we need to control FM_LNA_EN and GPS_FM_LNA_2V8_EN to turn LNA.
    printk(KERN_INFO "%s : config and enable FM LNA control.\n", __func__);
    rc = msm_gpios_enable(fm_gpio_table,ARRAY_SIZE(fm_gpio_table));
    if (rc < 0)
            goto fm_gpio_config_fail;

	/* FIHTDC, Div2-SW2-BSP Godfrey, FB0.B-396 */
	rc = enableGPS_FM_LNA(true);
	if (rc < 0)
            goto fm_gpio_config_fail;

    gpio_set_value(FM_LNA_EN,1);
    #endif
    /* } FIHTDC, Div2-SW2-BSP Godfrey, FM  */

	return 0;
fm_gpio_config_fail:
    /* FIHTDC, Div2-SW2-BSP Godfrey */
	if( ( fih_get_product_id() == Product_FD1 ) && ( ( fih_get_product_phase() != Product_PR1 ) &&
													( fih_get_product_phase() != Product_PR2p5 ) &&
													( fih_get_product_phase() != Product_PR230 ) &&
													( fih_get_product_phase() != Product_PR232 ) &&
													( fih_get_product_phase() != Product_PR3 ) &&
													( fih_get_product_phase() != Product_PR4 ) ) ){
	    pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_OFF);
	}
	else{
	    pmapp_clock_vote(id, QTR8x00_WCN_CLK,
					  PMAPP_CLOCK_VOTE_OFF);
	}
fm_clock_vote_fail:
	vreg_disable(pdata->vreg_s2);
	vreg_disable(pdata->vreg_gp16);
	return rc;

};

static void fm_radio_shutdown(struct marimba_fm_platform_data *pdata)
{
	int rc;
	const char *id = "FMPW";
	uint32_t irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
					GPIO_CFG_2MA);

    /* FIHTDC, Div2-SW2-BSP Godfrey, FM { */
    #ifdef CONFIG_FIH_FM_LNA
    //FB0 add LNA in FM, so we need to control FM_LNA_EN and GPS_FM_LNA_2V8_EN to turn LNA.
    /* FIHTDC, Div2-SW2-BSP Godfrey, FB0.B-396 */
	enableGPS_FM_LNA(false);
    gpio_set_value(FM_LNA_EN,0);
    #endif
    /* } FIHTDC, Div2-SW2-BSP Godfrey, FM  */

	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
	}
	rc = vreg_disable(pdata->vreg_s2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}

	/* } FIHTDC, Div2-SW2-BSP Godfrey, FM  */
	rc = vreg_disable(pdata->vreg_gp16);
    if (rc) {
        printk(KERN_ERR "%s: return val: %d \n",
                    __func__, rc);
    }
	if( ( fih_get_product_id() == Product_FD1 ) && ( ( fih_get_product_phase() != Product_PR1 ) &&
													( fih_get_product_phase() != Product_PR2p5 ) &&
													( fih_get_product_phase() != Product_PR230 ) &&
													( fih_get_product_phase() != Product_PR232 ) &&
													( fih_get_product_phase() != Product_PR3 ) &&
													( fih_get_product_phase() != Product_PR4 ) ) ){
	    rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
						  PMAPP_CLOCK_VOTE_OFF);
	}
	else{
	    rc = pmapp_clock_vote(id, QTR8x00_WCN_CLK,
						  PMAPP_CLOCK_VOTE_OFF);
	}
	if (rc < 0)
		printk(KERN_ERR "%s: clock_vote return val: %d \n",
						__func__, rc);
	rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 0);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg level vote return val: %d \n",
						__func__, rc);
}

static struct marimba_fm_platform_data marimba_fm_pdata = {
	.fm_setup =  fm_radio_setup,
	.fm_shutdown = fm_radio_shutdown,
	.irq = MSM_GPIO_TO_INT(147),
	.vreg_s2 = NULL,
	.vreg_xo_out = NULL,
};


/* Slave id address for FM/CDC/QMEMBIST
 * Values can be programmed using Marimba slave id 0
 * should there be a conflict with other I2C devices
 * */
#define MARIMBA_SLAVE_ID_FM_ADDR	0x2A
#define MARIMBA_SLAVE_ID_CDC_ADDR	0x77
#define MARIMBA_SLAVE_ID_QMEMBIST_ADDR	0X66

static const char *tsadc_id = "MADC";
static const char *vregs_tsadc_name[] = {
	"gp12",
	"s2",
};
static struct vreg *vregs_tsadc[ARRAY_SIZE(vregs_tsadc_name)];

static int marimba_tsadc_power(int vreg_on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
		if (!vregs_tsadc[i]) {
			printk(KERN_ERR "%s: vreg_get %s failed (%d)\n",
				__func__, vregs_tsadc_name[i], rc);
			goto vreg_fail;
		}

		rc = vreg_on ? vreg_enable(vregs_tsadc[i]) :
			  vreg_disable(vregs_tsadc[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
				__func__, vregs_tsadc_name[i],
			       vreg_on ? "enable" : "disable", rc);
			goto vreg_fail;
		}
	}
	/* vote for D0 buffer */
    /* FIHTDC, Div2-SW2-BSP Godfrey */
	if( ( fih_get_product_id() == Product_FD1 ) && ( ( fih_get_product_phase() != Product_PR1 ) &&
													( fih_get_product_phase() != Product_PR2p5 ) &&
													( fih_get_product_phase() != Product_PR230 ) &&
													( fih_get_product_phase() != Product_PR232 ) &&
													( fih_get_product_phase() != Product_PR3 ) &&
													( fih_get_product_phase() != Product_PR4 ) ) ){
	    rc = pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_DO,
			vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
	}
	else{
	    rc = pmapp_clock_vote(tsadc_id, QTR8x00_WCN_CLK,
			vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
	}
	if (rc)	{
		printk(KERN_ERR "%s: unable to %svote for d0 clk\n",
			__func__, vreg_on ? "" : "de-");
		goto do_vote_fail;
	}

	msleep(5); /* ensure power is stable */

	return 0;

do_vote_fail:
vreg_fail:
	while (i)
		vreg_disable(vregs_tsadc[--i]);
	return rc;
}

static int marimba_tsadc_vote(int vote_on)
{
	int rc, level;

	level = vote_on ? 1300 : 0;

	rc = pmapp_vreg_level_vote(tsadc_id, PMAPP_VREG_S2, level);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg level %s failed (%d)\n",
			__func__, vote_on ? "on" : "off", rc);

	return rc;
}

static int marimba_tsadc_init(void)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
		vregs_tsadc[i] = vreg_get(NULL, vregs_tsadc_name[i]);
		if (IS_ERR(vregs_tsadc[i])) {
			printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
			       __func__, vregs_tsadc_name[i],
			       PTR_ERR(vregs_tsadc[i]));
			rc = PTR_ERR(vregs_tsadc[i]);
			goto vreg_get_fail;
		}
	}

	return rc;

vreg_get_fail:
	while (i)
		vreg_put(vregs_tsadc[--i]);
	return rc;
}

static int marimba_tsadc_exit(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
		if (vregs_tsadc[i])
			vreg_put(vregs_tsadc[i]);
	}

	rc = pmapp_vreg_level_vote(tsadc_id, PMAPP_VREG_S2, 0);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg level off failed (%d)\n",
			__func__, rc);

	return rc;
}


static struct msm_ts_platform_data msm_ts_data = {
	.min_x          = 0,
	.max_x          = 4096,
	.min_y          = 0,
	.max_y          = 4096,
	.min_press      = 0,
	.max_press      = 255,
	.inv_x          = 4096,
	.inv_y          = 4096,
	.can_wakeup	= false,
};

static struct marimba_tsadc_platform_data marimba_tsadc_pdata = {
	.marimba_tsadc_power =  marimba_tsadc_power,
	.init		     =  marimba_tsadc_init,
	.exit		     =  marimba_tsadc_exit,
	.level_vote	     =  marimba_tsadc_vote,
	.tsadc_prechg_en = true,
	.can_wakeup	= false,
	.setup = {
		.pen_irq_en	=	true,
		.tsadc_en	=	true,
	},
	.params2 = {
		.input_clk_khz		=	2400,
		.sample_prd		=	TSADC_CLK_3,
	},
	.params3 = {
		.prechg_time_nsecs	=	6400,
		.stable_time_nsecs	=	6400,
		.tsadc_test_mode	=	0,
	},
	.tssc_data = &msm_ts_data,
};

static struct vreg *vreg_codec_s4;
static int msm_marimba_codec_power(int vreg_on)
{
	int rc = 0;

	if (!vreg_codec_s4) {

		vreg_codec_s4 = vreg_get(NULL, "s4");

		if (IS_ERR(vreg_codec_s4)) {
			printk(KERN_ERR "%s: vreg_get() failed (%ld)\n",
				__func__, PTR_ERR(vreg_codec_s4));
			rc = PTR_ERR(vreg_codec_s4);
			goto  vreg_codec_s4_fail;
		}
	}

	if (vreg_on) {
		rc = vreg_enable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	} else {
		rc = vreg_disable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	}

vreg_codec_s4_fail:
	return rc;
}

static struct marimba_codec_platform_data mariba_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_MARIMBA_CODEC
	.snddev_profile_init = msm_snddev_init,
#endif
};

static struct marimba_platform_data marimba_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_FM]       = MARIMBA_SLAVE_ID_FM_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_CDC]	     = MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_marimba_setup_power,
	.marimba_shutdown = msm_marimba_shutdown_power,
	.fm = &marimba_fm_pdata,
	.codec = &mariba_codec_pdata,
};

static void __init msm7x30_init_marimba(void)
{
	vreg_marimba_1 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}

	vreg_marimba_2 = vreg_get(NULL, "gp16");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}
}

static struct marimba_codec_platform_data timpani_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_TIMPANI_CODEC
	.snddev_profile_init = msm_snddev_init_timpani,
#endif
};

static struct marimba_platform_data timpani_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_CDC]	= MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_timpani_setup_power,
	.marimba_shutdown = msm_timpani_shutdown_power,
	.codec = &timpani_codec_pdata,
	.tsadc = &marimba_tsadc_pdata,
};

#define TIMPANI_I2C_SLAVE_ADDR	0xD

static struct i2c_board_info msm_i2c_gsbi7_timpani_info[] = {
	{
		I2C_BOARD_INFO("timpani", TIMPANI_I2C_SLAVE_ADDR),
		.platform_data = &timpani_pdata,
	},
};

#ifdef CONFIG_MSM7KV2_AUDIO
static struct resource msm_aictl_resources[] = {
	{
		.name = "aictl",
		.start = 0xa5000100,
		.end = 0xa5000100,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_mi2s_resources[] = {
	{
		.name = "hdmi",
		.start = 0xac900000,
		.end = 0xac900038,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_rx",
		.start = 0xac940040,
		.end = 0xac940078,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_tx",
		.start = 0xac980080,
		.end = 0xac9800B8,
		.flags = IORESOURCE_MEM,
	}

};

static struct msm_lpa_platform_data lpa_pdata = {
	.obuf_hlb_size = 0x2BFF8,
	.dsp_proc_id = 0,
	.app_proc_id = 2,
	.nosb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x3ff8,
		.sb_min_addr = 0,
		.sb_max_addr = 0,
	},
	.sb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x37f8,
		.sb_min_addr = 0x3800,
		.sb_max_addr = 0x3ff8,
	}
};

static struct resource msm_lpa_resources[] = {
	{
		.name = "lpa",
		.start = 0xa5000000,
		.end = 0xa50000a0,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_aux_pcm_resources[] = {

	{
		.name = "aux_codec_reg_addr",
		.start = 0xac9c00c0,
		.end = 0xac9c00c8,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "aux_pcm_dout",
		.start  = 138,
		.end    = 138,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 139,
		.end    = 139,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 140,
		.end    = 140,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 141,
		.end    = 141,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_aux_pcm_device = {
	.name   = "msm_aux_pcm",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_aux_pcm_resources),
	.resource       = msm_aux_pcm_resources,
};

struct platform_device msm_aictl_device = {
	.name = "audio_interct",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_aictl_resources),
	.resource = msm_aictl_resources,
};

struct platform_device msm_mi2s_device = {
	.name = "mi2s",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_mi2s_resources),
	.resource = msm_mi2s_resources,
};

struct platform_device msm_lpa_device = {
	.name = "lpa",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_lpa_resources),
	.resource = msm_lpa_resources,
	.dev		= {
		.platform_data = &lpa_pdata,
	},
};
#endif /* CONFIG_MSM7KV2_AUDIO */

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	0,
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_MODE_LP)|
	(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 1 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	 /* Concurrency 2 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 3 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 4 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 5 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 6 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

#define DEC_INSTANCE(max_instance_same, max_instance_diff) { \
	.max_instances_same_dec = max_instance_same, \
	.max_instances_diff_dec = max_instance_diff}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
};

static struct dec_instance_table dec_instance_list[][MSM_MAX_DEC_CNT] = {
	/* Non Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 2), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 2), /* WMA */
		DEC_INSTANCE(3, 2), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(1, 1), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 1), /* WMAPRO */
	},
	/* Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 3), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 3), /* WMA */
		DEC_INSTANCE(4, 3), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(2, 3), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 2), /* WMAPRO */
	},
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
	.dec_instance_list = &dec_instance_list[0][0],
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

/* Div2-SW2-BSP-FBX-OW { */
#ifdef CONFIG_SMC91X
static struct resource smc91x_resources[] = {
	[0] = {
		.start = 0x8A000300,
		.end = 0x8A0003ff,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start = MSM_GPIO_TO_INT(156),
		.end = MSM_GPIO_TO_INT(156),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device smc91x_device = {
	.name           = "smc91x",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(smc91x_resources),
	.resource       = smc91x_resources,
};
#endif

#ifdef CONFIG_SMSC911X
static struct smsc911x_platform_config smsc911x_config = {
	.phy_interface	= PHY_INTERFACE_MODE_MII,
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_32BIT,
};

static struct resource smsc911x_resources[] = {
	[0] = {
		.start		= 0x8D000000,
		.end		= 0x8D000100,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= MSM_GPIO_TO_INT(88),
		.end		= MSM_GPIO_TO_INT(88),
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

static struct msm_gpio smsc911x_gpios[] = {
    { GPIO_CFG(172, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr6" },
    { GPIO_CFG(173, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr5" },
    { GPIO_CFG(174, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr4" },
    { GPIO_CFG(175, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr3" },
    { GPIO_CFG(176, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr2" },
    { GPIO_CFG(177, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr1" },
    { GPIO_CFG(178, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ebi2_addr0" },
    { GPIO_CFG(88, 2, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "smsc911x_irq"  },
};

static void msm7x30_cfg_smsc911x(void)
{
	int rc;

	rc = msm_gpios_request_enable(smsc911x_gpios,
			ARRAY_SIZE(smsc911x_gpios));
	if (rc)
		pr_err("%s: unable to enable gpios\n", __func__);
}
#endif
/* } Div2-SW2-BSP-FBX-OW */

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x02,
	.buf_size       = 16384,
	.vendor         = "GOOGLE",
	.product        = "Mass storage",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID

//Div6-D1-JL-UsbPidVid-02+{   
//Original products's function array.
#if 0
static char *usb_functions_default[] = {
	"diag",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_default_adb[] = {
	"diag",
	"adb",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_rndis_diag[] = {
	"rndis",
	"diag",
};

static char *usb_functions_rndis_adb_diag[] = {
	"rndis",
	"adb",
	"diag",
};
#endif
//Div6-D1-JL-UsbPidVid-02+}
//Div2-5-3-Peripheral-LL-UsbPorting-00+{
//C000
static char *usb_functions_c000[] = {
    "nmea",
    "modem",
    "adb",
    "diag",
    "usb_mass_storage",
};

//BEGIN FXPCAYM-206: Motorola-VMU WX435-Caymus #26408:
// Removed modem composition as c001 is used for UMS/ADB
//C001
static char *usb_functions_c001[] = {
    "adb",
    "usb_mass_storage",
};

//END FXPCAYM-206: Motorola-VMU WX435-Caymus #26408

//C002
static char *usb_functions_c002[] = {
    "diag",
};

//C003
static char *usb_functions_c003[] = {
    "modem",
};

//C004
static char *usb_functions_c004[] = {
    "usb_mass_storage",
};

//C007
static char *usb_functions_c007[] = {
    "rndis",
    "adb",
    "diag",
};

//C008
static char *usb_functions_c008[] = {
    "rndis",
    "adb",
};
//Div2-5-3-Peripheral-LL-UsbPorting-00+}

#if 0
//ori
static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
	"adb",
#ifdef CONFIG_USB_F_SERIAL
	"modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	"rmnet",
#endif
	"usb_mass_storage",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};
#else

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
        "rndis",
#endif
    "usb_mass_storage",
#ifdef CONFIG_USB_ANDROID_DIAG
    "diag",
#endif        
    "adb",
#ifdef CONFIG_USB_F_SERIAL
    "modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	"rmnet",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};

#endif
//Div6-D1-JL-UsbPidVid-03+}

//Div6-D1-JL-UsbPidVid-02+{  
#if 0
static struct android_usb_product usb_products[] = {
    //Div6-D1-JL-UsbPidVid-01+{   
    {
        .product_id	= 0xC000,
		.num_functions	= ARRAY_SIZE(fih_usb_functions_all),
		.functions	= fih_usb_functions_all,
	},
	{
        .product_id	= 0xC004,
		.num_functions	= ARRAY_SIZE(fih_usb_functions_user),
		.functions	= fih_usb_functions_user,
	},
	//Div6-D1-JL-UsbPidVid-01+}
	{
		.product_id	= 0x9026,
		.num_functions	= ARRAY_SIZE(usb_functions_default),
		.functions	= usb_functions_default,
	},
	{
		.product_id	= 0x9025,
		.num_functions	= ARRAY_SIZE(usb_functions_default_adb),
		.functions	= usb_functions_default_adb,
	},
	{
		.product_id	= 0x902C,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_diag),
		.functions	= usb_functions_rndis_diag,
	},
	{
		.product_id	= 0x902D,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb_diag),
		.functions	= usb_functions_rndis_adb_diag,
	},

};
#else
//Div2-5-3-Peripheral-LL-UsbCustomized-01+{
/*Huawei*/
static struct android_usb_product usb_products_12d1[] = {
    {
        .product_id = 0x1028,
        .num_functions = ARRAY_SIZE(usb_functions_c008),
        .functions = usb_functions_c008,
    },

    {
        .product_id = 0x1027,
        .num_functions = ARRAY_SIZE(usb_functions_c007),
        .functions = usb_functions_c007,
    },

    {
        .product_id = 0x1024,
        .num_functions = ARRAY_SIZE(usb_functions_c004),
        .functions = usb_functions_c004,
    },

    {
        .product_id = 0x1023,
        .num_functions = ARRAY_SIZE(usb_functions_c003),
        .functions = usb_functions_c003,
    },

    {
        .product_id = 0x1022,
        .num_functions = ARRAY_SIZE(usb_functions_c002),
        .functions = usb_functions_c002,
    },

    {
        .product_id = 0x103c,
        .num_functions = ARRAY_SIZE(usb_functions_c001),
        .functions = usb_functions_c001,
    },

    {
        .product_id = 0x1021,
        .num_functions = ARRAY_SIZE(usb_functions_c000),
        .functions = usb_functions_c000,
    },
};
//Div2-5-3-Peripheral-LL-UsbCustomized-01+}
/*FIH*/
static struct android_usb_product usb_products[] = {
    //Div2-5-3-Peripheral-LL-UsbPorting-00+{
    {
        .product_id = 0xC008,
        .num_functions = ARRAY_SIZE(usb_functions_c008),
        .functions = usb_functions_c008,
    },

    {
        .product_id = 0xC007,
        .num_functions = ARRAY_SIZE(usb_functions_c007),
        .functions = usb_functions_c007,
    },

    {
        .product_id = 0xC004,
        .num_functions = ARRAY_SIZE(usb_functions_c004),
        .functions = usb_functions_c004,
    },

    {
        .product_id = 0xC003,
        .num_functions = ARRAY_SIZE(usb_functions_c003),
        .functions = usb_functions_c003,
    },

    {
        .product_id = 0xC002,
        .num_functions = ARRAY_SIZE(usb_functions_c002),
        .functions = usb_functions_c002,
    },

    {
        .product_id = 0xC001,
        .num_functions = ARRAY_SIZE(usb_functions_c001),
        .functions = usb_functions_c001,
    },

    {
        .product_id = 0xC000,
        .num_functions = ARRAY_SIZE(usb_functions_c000),
        .functions = usb_functions_c000,
    },

    //Div2-5-3-Peripheral-LL-UsbPorting-00+}
};

#endif
//Div6-D1-JL-UsbPidVid-02+}


static struct usb_mass_storage_platform_data mass_storage_pdata = {
//Div2D5-LC-BSP-Implement_Dual_SD_Card-00 *[
#ifdef CONFIG_FIH_SF5_DUAL_SD_CARD
	.nluns		= 3,
#else
	.nluns		= 2,    //Div6-D1-JL-UsbPorting-00+ PCtool
#endif
//Div2D5-LC-BSP-Implement_Dual_SD_Card-00 *]
	.vendor		= "Qualcomm Incorporated",
	.product        = "Mass Storage",
	.release	= 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x05C6,
	.vendorDescr	= "Qualcomm Incorporated",
};

static struct platform_device rndis_device = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x0489,   //Div6-D1-JL-UsbPidVid-00+
//BEGIN FXPCAYM-206: Motorola-VMU WX435-Caymus #26408:
	.product_id	= 0xC001,   //Div6-D1-JL-UsbPidVid-03+
//END FXPCAYM-206: Motorola-VMU WX435-Caymus #26408:
	.version	= 0x0100,
	.product_name		= "Android HSUSB Device",
	.manufacturer_name	= "FIH",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.serial_number = "1234567890ABCDEF",
	
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

static int __init board_serialno_setup(char *serialno)
{
	int i;
	char *src = serialno;

	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	android_usb_pdata.serial_number = serialno;
    
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);
#endif

static struct msm_gpio optnav_config_data[] = {
	{ GPIO_CFG(OPTNAV_CHIP_SELECT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
	"optnav_chip_select" },
};

static void __iomem *virtual_optnav;

static int optnav_gpio_setup(void)
{
	int rc = -ENODEV;
	rc = msm_gpios_request_enable(optnav_config_data,
			ARRAY_SIZE(optnav_config_data));

	/* Configure the FPGA for GPIOs */
	virtual_optnav = ioremap(FPGA_OPTNAV_GPIO_ADDR, 0x4);
	if (!virtual_optnav) {
		pr_err("%s:Could not ioremap region\n", __func__);
		return -ENOMEM;
	}
	/*
	 * Configure the FPGA to set GPIO 19 as
	 * normal, active(enabled), output(MSM to SURF)
	 */
	writew(0x311E, virtual_optnav);
	return rc;
}

static void optnav_gpio_release(void)
{
	msm_gpios_disable_free(optnav_config_data,
		ARRAY_SIZE(optnav_config_data));
	iounmap(virtual_optnav);
}

static struct vreg *vreg_gp7;
static struct vreg *vreg_gp4;
static struct vreg *vreg_gp9;
static struct vreg *vreg_usb3_3;

static int optnav_enable(void)
{
	int rc;
	/*
	 * Enable the VREGs L8(gp7), L10(gp4), L12(gp9), L6(usb)
	 * for I2C communication with keyboard.
	 */
	vreg_gp7 = vreg_get(NULL, "gp7");
	rc = vreg_set_level(vreg_gp7, 1800);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_gp7;
	}

	rc = vreg_enable(vreg_gp7);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_gp7;
	}

	vreg_gp4 = vreg_get(NULL, "gp4");
	rc = vreg_set_level(vreg_gp4, 2600);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_gp4;
	}

	rc = vreg_enable(vreg_gp4);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_gp4;
	}

	vreg_gp9 = vreg_get(NULL, "gp9");
	rc = vreg_set_level(vreg_gp9, 1800);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_gp9;
	}

	rc = vreg_enable(vreg_gp9);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_gp9;
	}

	vreg_usb3_3 = vreg_get(NULL, "usb");
	rc = vreg_set_level(vreg_usb3_3, 3300);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_3_3;
	}

	rc = vreg_enable(vreg_usb3_3);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_3_3;
	}

	/* Enable the chip select GPIO */
	gpio_set_value(OPTNAV_CHIP_SELECT, 1);
	gpio_set_value(OPTNAV_CHIP_SELECT, 0);

	return 0;

fail_vreg_3_3:
	vreg_disable(vreg_gp9);
fail_vreg_gp9:
	vreg_disable(vreg_gp4);
fail_vreg_gp4:
	vreg_disable(vreg_gp7);
fail_vreg_gp7:
	return rc;
}

static void optnav_disable(void)
{
	vreg_disable(vreg_usb3_3);
	vreg_disable(vreg_gp9);
	vreg_disable(vreg_gp4);
	vreg_disable(vreg_gp7);

	gpio_set_value(OPTNAV_CHIP_SELECT, 1);
}

static struct ofn_atlab_platform_data optnav_data = {
	.gpio_setup    = optnav_gpio_setup,
	.gpio_release  = optnav_gpio_release,
	.optnav_on     = optnav_enable,
	.optnav_off    = optnav_disable,
	.rotate_xy     = 0,
	.function1 = {
		.no_motion1_en		= true,
		.touch_sensor_en	= true,
		.ofn_en			= true,
		.clock_select_khz	= 1500,
		.cpi_selection		= 1200,
	},
	.function2 =  {
		.invert_y		= false,
		.invert_x		= true,
		.swap_x_y		= false,
		.hold_a_b_en		= true,
		.motion_filter_en       = true,
	},
};

#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL
static struct msm_hdmi_platform_data adv7520_hdmi_data = {
		.irq = MSM_GPIO_TO_INT(18),
};
#endif

#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
static void hdmi_setup_int_power(int on)
{
	gpio_tlmm_config(GPIO_HDMI_5V_EN, GPIO_CFG_ENABLE);			
	
    if (on)
		gpio_set_value(GPIO_PIN(GPIO_HDMI_5V_EN), 1);       
    else 	
        gpio_set_value(GPIO_PIN(GPIO_HDMI_5V_EN), 0);  		  
}

static int hdmi_interrupt_detect(void)
{
	return gpio_get_value(HDMI_INT);
}
static struct msm_hdmi_platform_data adv7525_hdmi_data = {
		.irq = MSM_GPIO_TO_INT(HDMI_INT),
		.intr_detect = hdmi_interrupt_detect,
		.setup_int_power = hdmi_setup_int_power,		
};
#endif

/* Div2-SW2-BSP-FBX-BATT { */
#ifdef CONFIG_DS2482
static struct ds2482_platform_data ds2482_pdata = {
    .pmic_gpio_ds2482_SLPZ      = 31,
    .sys_gpio_ds2482_SLPZ       = PM8058_GPIO_PM_TO_SYS(31) - 1,
    .sys_gpio_gauge_ls_en       = 88,
};
#endif

#ifdef CONFIG_BATTERY_BQ275X0
struct bq275x0_platform_data bq275x0_pdata = {
#if defined(CONFIG_FIH_PROJECT_FB0) || defined(CONFIG_FIH_PROJECT_SF4Y6)
        .pmic_BATLOW = 12,
#else
        .pmic_BATLOW = 16,
#endif
        .pmic_BATGD = 19,
};
#endif
/* } Div2-SW2-BSP-FBX-BATT */

//DIV5-BSP-CH-SF6-SENSOR-PORTING00++[
//Div2D1-OH-eCompass-AKM8975C_Porting-00+{
#ifdef CONFIG_SENSORS_AKM8975C
static struct akm8975_platform_data akm8975_chip_data = {
	.project_name = "SF4Y6",
	.gpio_DRDY = AKM8975C_DRDY_PIN,
};
#endif
//Div2D1-OH-eCompass-AKM8975C_Porting-00+}
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++]

static struct i2c_board_info msm_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("m33c01", OPTNAV_I2C_SLAVE_ADDR),
		.irq		= MSM_GPIO_TO_INT(OPTNAV_IRQ),
		.platform_data = &optnav_data,
	},
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL
	{
		I2C_BOARD_INFO("adv7520", 0x72 >> 1),
		.platform_data = &adv7520_hdmi_data,
	},
#endif
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
	{
		I2C_BOARD_INFO("adv7525", 0x72 >> 1),
		.platform_data = &adv7525_hdmi_data,
	},
#endif	
//Div2-SW2-BSP-Sensors, Chihchia 2010.4.30 +
#ifdef CONFIG_SENSORS_YAS529
    {
            I2C_BOARD_INFO("YAS529", 0x2e),
    },
    {
            I2C_BOARD_INFO("BMA150", 0x38),
            .irq = MSM_GPIO_TO_INT(40),
    },
#endif

//Div2D1-OH-eCompass-AKM8975C_Porting-00+{
#ifdef CONFIG_SENSORS_AKM8975C
    {
    	I2C_BOARD_INFO("akm8975", 0x0C),
		.irq = MSM_GPIO_TO_INT(AKM8975C_DRDY_PIN),
		.platform_data = &akm8975_chip_data,
	},
#endif
//Div2D1-OH-eCompass-AKM8975C_Porting-00+}

//Div2D5-OwenHuang-FTM-YAS529_Self-Test-00+{
#ifdef CONFIG_SENSORS_NK_YAS529
	{
			I2C_BOARD_INFO("YAS529", 0x2e),
	},
#endif

#ifdef CONFIG_SENSORS_BMA150
	{
			I2C_BOARD_INFO("BMA150", 0x38),
		//Div2D5-OwenHuang-SF6_GSensor-GPIO_Setting-00+{
		#ifdef CONFIG_FIH_PROJECT_SF4Y6
			.irq = PM8058_GPIO_PM_TO_SYS(8 - 1), //PMIC GPIO_08
		#else
			.irq = MSM_GPIO_TO_INT(40),
		#endif
		//Div2D5-OwenHuang-SF6_GSensor-GPIO_Setting-00+}
	}, 
#endif
//Div2D5-OwenHuang-FTM-YAS529_Self-Test-00+}

//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00+{ 
//for new yamaha sensor architecture
#ifdef CONFIG_INPUT_YAS529
	{
		I2C_BOARD_INFO("yas529", 0x2E),	//yamaha yas529
	},
#endif

#ifdef CONFIG_INPUT_BMA150
	{
		I2C_BOARD_INFO("bma150", 0x38), //bosch bma150/bosch bma023	
	},
#endif
//Div2D5-OwenHuang-FB0_Sensors-Porting_New_Sensors_Architecture-00+}

#ifdef CONFIG_SENSORS_LTR502ALS
    {
    	    //Div2D5-OwenHuang-ALSPS-I2C_Address-00*{
    		#ifdef CONFIG_FIH_PROJECT_SF4V5
			I2C_BOARD_INFO("ltr502als", 0x1c),
			#else
            I2C_BOARD_INFO("ltr502als", 0x1d),
            #endif
			//Div2D5-OwenHuang-ALSPS-I2C_Address-00*}
     
            //Div2D5-OH-Sensors-GPIO_Settings-00+{
            #ifdef CONFIG_FIH_PROJECT_SF4V5
            .irq = MSM_GPIO_TO_INT(20),
            #else
            .irq = MSM_GPIO_TO_INT(49),
            #endif
            //Div2D5-OH-Sensors-GPIO_Settings-00+}
    },
#endif  
//Div2-SW2-BSP-Sensors, Chihchia -  

//Div2D5-OwenHuang-SF8_Sensor_Porting-00+{
#ifdef CONFIG_SENSORS_CM3623
	{
	#ifdef CONFIG_SENSORS_CM3623_IS_AD
		I2C_BOARD_INFO("cm3623", 0x49),
	#else
		I2C_BOARD_INFO("cm3623", 0x11),
	#endif
	},
#endif
//Div2D5-OwenHuang-SF8_Sensor_Porting-00+}

//Div2-SW2-BSP-Touch, Vincent +
#ifdef CONFIG_FIH_TOUCHSCREEN_BU21018MWV
    {
        I2C_BOARD_INFO("bu21018mwv", 0x5C),
    },
#endif
#ifdef CONFIG_FIH_TOUCHSCREEN_BI041P
    {
        I2C_BOARD_INFO("bi041p", 0x08),
        .irq = MSM_GPIO_TO_INT(42),
    },
#endif
//Div2-SW2-BSP-Touch, Vincent -
//Div2-D5-Peripheral-FG-4H8TouchPorting-00+[
#ifdef CONFIG_FIH_TOUCHSCREEN_ATMEL_MXT165
    {
        I2C_BOARD_INFO("atmel_mxt165", 0x4A),
    },
#endif
//Div2-D5-Peripheral-FG-4H8TouchPorting-00+]
//Div2-D5-Peripheral-FG-TouchPorting-00+[
#ifdef CONFIG_FIH_TOUCHSCREEN_ATMEL_QT602240
    {
        I2C_BOARD_INFO("qt602240", 0x4B),
    },
#endif
//Div2-D5-Peripheral-FG-TouchPorting-00+]

//Div2-D5-OwenHung-Synaptics T1320 touch driver porting+
#ifdef CONFIG_FIH_TOUCHSCREEN_SYNAPTICS_T1320
    {
        I2C_BOARD_INFO("synaptics-t1320-ts", 0x20),
    },
#endif
//Div2-D5-OwenHung-Synaptics T1320 touch driver porting-

//Div2-SW6-MM-HL-Camera-Flash-02+{
#ifdef CONFIG_FIH_AAT1272
    {
        I2C_BOARD_INFO("aat1272", 0x37),
    },
#endif
//Div2-SW6-MM-HL-Camera-Flash-02+}

};

static struct i2c_board_info msm_marimba_board_info[] = {
	{
		I2C_BOARD_INFO("marimba", 0xc),
		.platform_data = &marimba_pdata,
	}
};

//  FTM phone function, Henry.Wang 2010.4.30+
#ifdef CONFIG_FIH_FTM_PHONE
static struct platform_device ftm_phone_device = {
    .name       = "ftm_phone",
    .id     = -1,
};
#endif
// FTM phone function, Henry.Wang, 2010.4.30-

#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
};

static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = 0x9018,
		.functions	    = 0x1F, /* 011111 */
	},

};
static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.version	= 0x0100,
	.phy_info	= USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM,
	.vendor_id	= 0x5c6,
	.product_name	= "Qualcomm HSUSB Device",
	.serial_number	= "1234567890ABCDEF",
	.manufacturer_name
			= "Qualcomm Incorporated",
	.compositions	= usb_func_composition,
	.num_compositions
			= ARRAY_SIZE(usb_func_composition),
	.function_map	= usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.core_clk	= 1,
};
#endif

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
//SW2-5-1-MP-Force_Ramdump-00*[
//	.pwr_key_delay_ms = 500, /* 0 will disable end key */
	.pwr_key_delay_ms = 0, /* 0 will disable end key */
//SW2-5-1-MP-Force_Ramdump-00*]
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 8594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 4594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].suspend_enabled = 0,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].latency = 500,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].residency = 6000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 0,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 443,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 1098,

	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency = 2,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].residency = 0,
};

//SW2-6-MM-JH-SPI-00+
#ifdef CONFIG_SPI_QSD
static struct resource qsd_spi_resources[] = {
	{
		.name   = "spi_irq_in",
		.start	= INT_SPI_INPUT,
		.end	= INT_SPI_INPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_out",
		.start	= INT_SPI_OUTPUT,
		.end	= INT_SPI_OUTPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_err",
		.start	= INT_SPI_ERROR,
		.end	= INT_SPI_ERROR,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_base",
		.start	= 0xA8000000,
		.end	= 0xA8000000 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "spidm_channels",
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "spidm_crci",
		.flags  = IORESOURCE_DMA,
	},
};

#define AMDH0_BASE_PHYS		0xAC200000
#define ADMH0_GP_CTL		(ct_adm_base + 0x3D8)
static int msm_qsd_spi_dma_config(void)
{
	void __iomem *ct_adm_base = 0;
	u32 spi_mux = 0;
	int ret = 0;

	ct_adm_base = ioremap(AMDH0_BASE_PHYS, PAGE_SIZE);
	if (!ct_adm_base) {
		pr_err("%s: Could not remap %x\n", __func__, AMDH0_BASE_PHYS);
		return -ENOMEM;
	}

	spi_mux = (ioread32(ADMH0_GP_CTL) & (0x3 << 12)) >> 12;

	qsd_spi_resources[4].start  = DMOV_USB_CHAN;
	qsd_spi_resources[4].end    = DMOV_TSIF_CHAN;

	switch (spi_mux) {
	case (1):
		qsd_spi_resources[5].start  = DMOV_HSUART1_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART1_TX_CRCI;
		break;
	case (2):
		qsd_spi_resources[5].start  = DMOV_HSUART2_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART2_TX_CRCI;
		break;
	case (3):
		qsd_spi_resources[5].start  = DMOV_CE_OUT_CRCI;
		qsd_spi_resources[5].end    = DMOV_CE_IN_CRCI;
		break;
	default:
		ret = -ENOENT;
	}

	iounmap(ct_adm_base);

	return ret;
}

static struct platform_device qsd_device_spi = {
	.name		= "spi_qsd",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qsd_spi_resources),
	.resource	= qsd_spi_resources,
};

static struct spi_board_info lcdc_sharp_spi_board_info[] __initdata = {
	{
		.modalias	= "lcdc_sharp_ls038y7dx01",
		.mode		= SPI_MODE_1,
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 26331429,
	}
};
static struct spi_board_info lcdc_toshiba_spi_board_info[] __initdata = {
	{
		.modalias       = "lcdc_toshiba_ltm030dd40",
		.mode           = SPI_MODE_3|SPI_CS_HIGH,
		.bus_num        = 0,
		.chip_select    = 0,
		.max_speed_hz   = 9963243,
	}
};

static struct msm_gpio qsd_spi_gpio_config_data[] = {
	{ GPIO_CFG(45, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "spi_mosi" },
	{ GPIO_CFG(48, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
};

static int msm_qsd_spi_gpio_config(void)
{
	return msm_gpios_request_enable(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static void msm_qsd_spi_gpio_release(void)
{
	msm_gpios_disable_free(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static struct msm_spi_platform_data qsd_spi_pdata = {
	.max_clock_speed = 26331429,
	.clk_name = "spi_clk",
	.pclk_name = "spi_pclk",
	.gpio_config  = msm_qsd_spi_gpio_config,
	.gpio_release = msm_qsd_spi_gpio_release,
	.dma_config = msm_qsd_spi_dma_config,
};

static void __init msm_qsd_spi_init(void)
{
	qsd_device_spi.dev.platform_data = &qsd_spi_pdata;
}
#endif // CONFIG_SPI_QSD
//SW2-6-MM-JH-SPI-00-

#ifdef CONFIG_USB_EHCI_MSM
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	int rc;
	static int vbus_is_on;
	struct pm8058_gpio usb_vbus = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.vin_sel        = 2,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};

	/* If VBUS is already on (or off), do nothing. */
	if (unlikely(on == vbus_is_on))
		return;

	if (on) {
		rc = pm8058_gpio_config(36, &usb_vbus);
		if (rc) {
			pr_err("%s PMIC GPIO 36 write failed\n", __func__);
			return;
		}
	} else
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(36), 0);

	vbus_is_on = on;
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info   = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
	.power_budget   = 180,
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static struct vreg *vreg_3p3;
static int msm_hsusb_ldo_init(int init)
{
	uint32_t version = 0;
	int def_vol = 3400;

	version = socinfo_get_version();

	if (SOCINFO_VERSION_MAJOR(version) >= 2 &&
			SOCINFO_VERSION_MINOR(version) >= 1) {
		def_vol = 3075;
		pr_debug("%s: default voltage:%d\n", __func__, def_vol);
	}

	if (init) {
		vreg_3p3 = vreg_get(NULL, "usb");
		if (IS_ERR(vreg_3p3))
			return PTR_ERR(vreg_3p3);
		vreg_set_level(vreg_3p3, def_vol);
	} else
		vreg_put(vreg_3p3);

	return 0;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	if (enable)
		return vreg_enable(vreg_3p3);

	return vreg_disable(vreg_3p3);
}

static int msm_hsusb_ldo_set_voltage(int mV)
{
	static int cur_voltage = 3400;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (cur_voltage == mV)
		return 0;

	cur_voltage = mV;

	pr_debug("%s: (%d)\n", __func__, mV);

	return vreg_set_level(vreg_3p3, mV);
}
#endif

#ifndef CONFIG_USB_EHCI_MSM
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init);
#endif
static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,

#ifndef CONFIG_USB_EHCI_MSM
	.pmic_notif_init         = msm_hsusb_pmic_notif_init,
#else
	.vbus_power = msm_hsusb_vbus_power,
#endif
	.core_clk		 = 1,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
	.chg_connected		 = hsusb_chg_connected,
	.chg_init		 = hsusb_chg_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_set_voltage	 = msm_hsusb_ldo_set_voltage,
};

#ifdef CONFIG_USB_GADGET
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;
#endif
#ifndef CONFIG_USB_EHCI_MSM
typedef void (*notify_vbus_state) (int);
notify_vbus_state notify_vbus_state_func_ptr;
int vbus_on_irq;
static irqreturn_t pmic_vbus_on_irq(int irq, void *data)
{
	pr_info("%s: vbus notification from pmic\n", __func__);

	(*notify_vbus_state_func_ptr) (1);

	return IRQ_HANDLED;
}
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret;

	if (init) {
		if (!callback)
			return -ENODEV;

		notify_vbus_state_func_ptr = callback;
		vbus_on_irq = platform_get_irq_byname(&msm_device_otg,
			"vbus_on");
		if (vbus_on_irq <= 0) {
			pr_err("%s: unable to get vbus on irq\n", __func__);
			return -ENODEV;
		}

		ret = request_irq(vbus_on_irq, pmic_vbus_on_irq,
			IRQF_TRIGGER_RISING, "msm_otg_vbus_on", NULL);
		if (ret) {
			pr_info("%s: request_irq for vbus_on"
				"interrupt failed\n", __func__);
			return ret;
		}
		msm_otg_pdata.pmic_vbus_irq = vbus_on_irq;
		return 0;
	} else {
		free_irq(vbus_on_irq, 0);
		notify_vbus_state_func_ptr = NULL;
		return 0;
	}
}
#endif

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
#ifndef CONFIG_SPI_QSD
static int lcdc_gpio_array_num[] = {
				45, /* spi_clk */
				46, /* spi_cs  */
				47, /* spi_mosi */
				48, /* spi_miso */
				};

static struct msm_gpio lcdc_gpio_config_data[] = {
	{ GPIO_CFG(45, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
};

static void lcdc_config_gpios(int enable)
{
	if (enable) {
		msm_gpios_request_enable(lcdc_gpio_config_data,
					      ARRAY_SIZE(
						      lcdc_gpio_config_data));
	} else
		msm_gpios_disable_free(lcdc_gpio_config_data,
					    ARRAY_SIZE(
						    lcdc_gpio_config_data));
}
#endif

static struct msm_panel_common_pdata lcdc_sharp_panel_data = {
#ifndef CONFIG_SPI_QSD
	.panel_config_gpio = lcdc_config_gpios,
	.gpio_num          = lcdc_gpio_array_num,
#endif
	.gpio = 2, 	/* LPG PMIC_GPIO26 channel number */
};

static struct platform_device lcdc_sharp_panel_device = {
	.name   = "lcdc_sharp_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_sharp_panel_data,
	}
};
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-

#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL 	
static struct msm_gpio hdmi_panel_gpios[] = {
    { GPIO_CFG(34, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), "hdmi_5v_en" },   
	{ GPIO_CFG(HDMI_INT, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "hdmi_int" },	       
};
#endif	

#if defined(CONFIG_FB_MSM_HDMI_ADV7525_PANEL) || defined(CONFIG_FB_MSM_HDMI_ADV7520_PANEL) 
static struct msm_gpio dtv_panel_gpios[] = {
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL
	{ GPIO_CFG(18, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "hdmi_int" },
#endif
	{ GPIO_CFG(120, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_mclk" },
	{ GPIO_CFG(121, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_sd0" },
	{ GPIO_CFG(122, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_sd1" },
	{ GPIO_CFG(123, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_sd2" },
	{ GPIO_CFG(124, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "dtv_pclk" },
	{ GPIO_CFG(125, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_en" },
	{ GPIO_CFG(126, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_vsync" },
	{ GPIO_CFG(127, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_hsync" },
	{ GPIO_CFG(128, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data0" },
	{ GPIO_CFG(129, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data1" },
	{ GPIO_CFG(130, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data2" },
	{ GPIO_CFG(131, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data3" },
	{ GPIO_CFG(132, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data4" },
	{ GPIO_CFG(160, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data5" },
	{ GPIO_CFG(161, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data6" },
	{ GPIO_CFG(162, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data7" },
	{ GPIO_CFG(163, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data8" },
	{ GPIO_CFG(164, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data9" },
	{ GPIO_CFG(165, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat10" },
	{ GPIO_CFG(166, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat11" },
	{ GPIO_CFG(167, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat12" },
	{ GPIO_CFG(168, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat13" },
	{ GPIO_CFG(169, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat14" },
	{ GPIO_CFG(170, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat15" },
	{ GPIO_CFG(171, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat16" },
	{ GPIO_CFG(172, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat17" },
	{ GPIO_CFG(173, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat18" },
	{ GPIO_CFG(174, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat19" },
	{ GPIO_CFG(175, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat20" },
	{ GPIO_CFG(176, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat21" },
	{ GPIO_CFG(177, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat22" },
	{ GPIO_CFG(178, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat23" },
};
#endif

#ifdef HDMI_RESET
static unsigned dtv_reset_gpio =
	GPIO_CFG(37, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
#endif
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL 
static int gpio_set(const char *label, const char *name, int level, int on)
{
	struct vreg *vreg = vreg_get(NULL, label);
	int rc;

	if (IS_ERR(vreg)) {
		rc = PTR_ERR(vreg);
		pr_err("%s: vreg %s get failed (%d)\n",
			__func__, name, rc);
		return rc;
	}

	rc = vreg_set_level(vreg, level);
	if (rc) {
		pr_err("%s: vreg %s set level failed (%d)\n",
			__func__, name, rc);
		return rc;
	}

	if (on)
		rc = vreg_enable(vreg);
	else
		rc = vreg_disable(vreg);
	if (rc)
		pr_err("%s: vreg %s enable failed (%d)\n",
			__func__, name, rc);
	return rc;
}

static int i2c_gpio_power(int on)
{
	int rc = gpio_set("gp7", "LDO8", 1800, on);
	if (rc)
		return rc;
	return gpio_set("gp4", "LDO10", 2600, on);	
}
static int dtv_panel_power(int on)
{
	int flag_on = !!on;
	static int dtv_power_save_on;
	int rc;

	if (dtv_power_save_on == flag_on)
		return 0;

	dtv_power_save_on = flag_on;
	pr_info("%s: %d >>\n", __func__, on);

	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HDMI_5V_EN),
		on);

#ifdef HDMI_RESET
	if (on) {
		/* reset Toshiba WeGA chip -- toggle reset pin -- gpio_180 */
		rc = gpio_tlmm_config(dtv_reset_gpio, GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, dtv_reset_gpio, rc);
			return rc;
		}

		/* bring reset line low to hold reset*/
		gpio_set_value(37, 0);
	}
#endif

	if (on) {
		rc = msm_gpios_enable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = msm_gpios_disable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio disable failed: %d\n",
				__func__, rc);
			return rc;
		}
	}

	rc = i2c_gpio_power(on);
	if (rc)
		return rc;

	mdelay(5);		/* ensure power is stable */

	/*  -- LDO17 for HDMI */
	rc = gpio_set("gp11", "LDO17", 2600, on);
	if (rc)
		return rc;

	mdelay(5);		/* ensure power is stable */

#ifdef HDMI_RESET
	if (on) {
		gpio_set_value(37, 1);	/* bring reset line high */
		mdelay(10);		/* 10 msec before IO can be accessed */
	}
#endif
	pr_info("%s: %d <<\n", __func__, on);

	return rc;
}
#endif	
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL 
static int dtv_panel_power(int on)
{
	int flag_on = !!on;
	static int dtv_power_save_on;
	int rc;
    struct vreg *vreg_ldo11;

	if (dtv_power_save_on == flag_on)
		return 0;
 
	dtv_power_save_on = flag_on;
	pr_info("%s: %d >>\n", __func__, on);
	
    if(!on){   
		rc = msm_gpios_disable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio disable failed: %d\n",
				__func__, rc);
			return rc;
		}            
        printk("dtv_panel_power always turn on\n");
        return 0;
    }else if(hdmi_init_done){				
		rc = msm_gpios_enable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
			return rc;
		}
        return 0;		        
    }
    
	if (on) {
		rc = msm_gpios_enable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
			return rc;
		}
		rc = msm_gpios_enable(hdmi_panel_gpios,
				ARRAY_SIZE(hdmi_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
			return rc;
		}		
	} else {
		rc = msm_gpios_disable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio disable failed: %d\n",
				__func__, rc);
			return rc;
		}
	}
    /* VDDIO 1.8V -- LDO11*/
    vreg_ldo11 = vreg_get(NULL, "gp2");

    if (IS_ERR(vreg_ldo11)) {
        rc = PTR_ERR(vreg_ldo11);
        printk("%s: gp2 vreg get failed (%d)\n",
               __func__, rc);
        return rc;
    }

    rc = vreg_set_level(vreg_ldo11, 1800);
    if (rc) {
        printk("%s: vreg LDO11 set level failed (%d)\n",
               __func__, rc);
        return rc;
    }

    if (on)
        rc = vreg_enable(vreg_ldo11);             
    else
        rc = vreg_disable(vreg_ldo11);

    if (rc) {
        printk("%s: LDO11 vreg enable failed (%d)\n",
               __func__, rc);
        return rc;
    }
    mdelay(5);

	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HDMI_18V_EN),
		on);   
				         		
	gpio_tlmm_config(GPIO_HDMI_5V_EN, GPIO_CFG_ENABLE);			
	
    if (on)
		gpio_set_value(GPIO_PIN(GPIO_HDMI_5V_EN), 1);       
    else {
        printk("dtv_panel_power 5V always turn on\n");	
        ///gpio_set_value(GPIO_PIN(GPIO_HDMI_5V_EN), 0);  		
	}


	mdelay(5);		/* ensure power is stable */

	pr_info("%s: %d <<\n", __func__, on);
	hdmi_init_done = true;
	return rc;
}
#endif

#if defined(CONFIG_FB_MSM_HDMI_ADV7525_PANEL) || defined(CONFIG_FB_MSM_HDMI_ADV7520_PANEL) 
static struct lcdc_platform_data dtv_pdata = {
	.lcdc_power_save   = dtv_panel_power,
};
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL
static struct platform_device hdmi_adv7520_panel_device = {
	.name   = "adv7520",
	.id     = 0,
};
#endif

#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
static struct platform_device hdmi_adv7525_panel_device = {
	.name   = "adv7525",
	.id     = 0,
};
#endif

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
};

//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+{
#ifdef CONFIG_MSM_UART2DM
static struct msm_serial_hs_platform_data msm_uart_dm2_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
};
#endif
//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+}

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (machine_is_msm7x30_fluid()) {
		if (!strcmp(name, "lcdc_sharp_wvga_pt"))
			return 0;
	} else {
		if (!strncmp(name, "mddi_toshiba_wvga_pt", 20))
			return -EPERM;
		else if (!strncmp(name, "lcdc_toshiba_wvga_pt", 20))
			return 0;
		else if (!strcmp(name, "mddi_orise"))
			return -EPERM;
	}
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

static struct platform_device msm_migrate_pages_device = {
	.name   = "msm_migrate_pages",
	.id     = -1,
};

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
       .name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	* the only valid choice at this time. The board structure is
	* set to all zeros by the C runtime initialization and that is now
	* the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	* include/linux/android_pmem.h.
	*/
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
       .name = "pmem_adsp",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
       .name = "pmem_audio",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

static struct platform_device android_pmem_kernel_ebi1_device = {
       .name = "android_pmem",
       .id = 1,
       .dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct platform_device android_pmem_adsp_device = {
       .name = "android_pmem",
       .id = 2,
       .dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_audio_device = {
       .name = "android_pmem",
       .id = 4,
       .dev = { .platform_data = &android_pmem_audio_pdata },
};

#if CONFIG_MSM_HW3D
static struct resource resources_hw3d[] = {
	{
		.start	= 0xA0000000,
		.end	= 0xA00fffff,
		.flags	= IORESOURCE_MEM,
		.name	= "regs",
	},
	{
		.flags	= IORESOURCE_MEM,
		.name	= "smi",
	},
	{
		.flags	= IORESOURCE_MEM,
		.name	= "ebi",
	},
	{
#ifdef CONFIG_ARCH_MSM7X30
        .start  = INT_GRP_3D,
        .end    = INT_GRP_3D,
#else
		.start	= INT_GRAPHICS,
		.end	= INT_GRAPHICS,
#endif
		.flags	= IORESOURCE_IRQ,
		.name	= "gfx",
	},
};

static struct platform_device hw3d_device = {
	.name		= "msm_hw3d",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_hw3d),
	.resource	= resources_hw3d,
};
#endif

static struct kgsl_platform_data kgsl_pdata = {
#ifdef CONFIG_MSM_NPA_SYSTEM_BUS
	/* NPA Flow IDs */
	.high_axi_3d = MSM_AXI_FLOW_3D_GPU_HIGH,
	.high_axi_2d = MSM_AXI_FLOW_2D_GPU_HIGH,
#else
	/* AXI rates in KHz */
	.high_axi_3d = 192000,
	.high_axi_2d = 192000,
#endif
	.max_grp2d_freq = 0,
	.min_grp2d_freq = 0,
	.set_grp2d_async = NULL, /* HW workaround, run Z180 SYNC @ 192 MHZ */
	.max_grp3d_freq = 245760000,
	.min_grp3d_freq = 192000000,
	.set_grp3d_async = set_grp3d_async,
	.imem_clk_name = "imem_clk",
	.grp3d_clk_name = "grp_clk",
	.grp2d0_clk_name = "grp_2d_clk",
};

static struct resource msm_kgsl_resources[] = {
	{
		.name = "kgsl_reg_memory",
		.start = 0xA3500000, /* 3D GRP address */
		.end = 0xA351ffff,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "kgsl_phys_memory",
		.start = 0,
		.end = 0,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "kgsl_yamato_irq",
		.start = INT_GRP_3D,
		.end = INT_GRP_3D,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "kgsl_2d0_reg_memory",
		.start = 0xA3900000, /* Z180 base address */
		.end = 0xA3900FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "kgsl_2d0_irq",
		.start = INT_GRP_2D,
		.end = INT_GRP_2D,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_device_kgsl = {
	.name = "kgsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(msm_kgsl_resources),
	.resource = msm_kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};

//SW2-D5-OwenHung-SF4V5/SF4Y6 keypad backlight+
#if defined(CONFIG_FIH_PROJECT_SF4V5) || defined(CONFIG_FIH_PROJECT_SF4Y6) || defined(CONFIG_FIH_PROJECT_SF8)
static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds",
	.id = -1,
};
#endif
//SW2-D5-OwenHung-SF4V5/SF4Y6 keypad backlight-

//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
static int mddi_toshiba_pmic_bl(int level)
{
	int ret = -EPERM;

/* Div2-SW2-BSP-FBX-LEDS { */
#ifndef CONFIG_LEDS_FIH_FBX_PWM
	ret = pmic_set_led_intensity(LED_LCD, level);
#endif
/* } Div2-SW2-BSP-FBX-LEDS */

	if (ret)
		printk(KERN_WARNING "%s: can't set lcd backlight!\n",
					__func__);
	return ret;
}

static struct msm_panel_common_pdata mddi_toshiba_pdata = {
	.pmic_backlight = mddi_toshiba_pmic_bl,
};

static struct platform_device mddi_toshiba_device = {
	.name   = "mddi_toshiba",
	.id     = 0,
	.dev    = {
		.platform_data = &mddi_toshiba_pdata,
	}
};
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-

#ifdef CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT

static unsigned lcdc_reset_gpio =
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA); 
static int display_common_power(int on)
{
    int rc = 0, flag_on = !!on;
    static int display_common_power_save_on = 0;
    struct vreg *vreg_ldo11, *vreg_ldo15 = NULL;
    //struct vreg *vreg_ldo20, *vreg_ldo16, *vreg_ldo8 = NULL;
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
    int product_id = 0;

    product_id = fih_get_product_id();
/* } FIHTDC-SW5-MULTIMEDIA, Chance */

    printk(KERN_INFO "[DISPLAY] %s(%d): current power status = %d.\n", __func__, on, display_common_power_save_on);
    if (display_common_power_save_on == flag_on)
        return 0;

    display_common_power_save_on = flag_on;

    if (on) {
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
        if(product_id != Product_SF6)
        {
        /* PMIC GPIO VREG_LCM_V1P8_EN pull high */      
        gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_LCM_V1P8_EN), 1);
        }
/* } FIHTDC-SW5-MULTIMEDIA, Chance */ 
        /* reset LCM -- toggle reset pin -- gpio_35 */
        rc = gpio_tlmm_config(lcdc_reset_gpio, GPIO_CFG_ENABLE);
        if (rc) {
            pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
                       __func__, lcdc_reset_gpio, rc);
            return rc;
        }

        gpio_set_value(35, 0);  /* bring reset line low to hold reset*/
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
        if(product_id == Product_SF6)
        {
            mdelay(1); /* wait 1 ms */
            //pr_err("%s: SF6 wait 1ms before turn on power domain\n",  __func__);
        }
/* } FIHTDC-SW5-MULTIMEDIA, Chance */ 
    }
    else
    {
        /* Hard Reset, Reset Pin from high to low */
        gpio_set_value(35, 0); 
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
        if(product_id != Product_SF6)
        {
        /* PMIC GPIO VREG_LCM_V1P8_EN pull low */       
        gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_LCM_V1P8_EN), 0);
        }
/* } FIHTDC-SW5-MULTIMEDIA, Chance */
    }

    /* LCM power -- has 2 power source */
    /* VDDIO 1.8V -- LDO11*/
    vreg_ldo11 = vreg_get(NULL, "gp2");

    if (IS_ERR(vreg_ldo11)) {
        pr_err("%s: gp2 vreg get failed (%ld)\n",
               __func__, PTR_ERR(vreg_ldo11));
        return rc;
    }

    /* VDC 3.05V -- LDO15 */
    vreg_ldo15 = vreg_get(NULL, "gp6");

    if (IS_ERR(vreg_ldo15)) {
        rc = PTR_ERR(vreg_ldo15);
        pr_err("%s: gp6 vreg get failed (%d)\n",
                __func__, rc);
        return rc;
    }

    rc = vreg_set_level(vreg_ldo11, 1800);
    if (rc) {
        pr_err("%s: vreg LDO11 set level failed (%d)\n",
               __func__, rc);
        return rc;
    }

    rc = vreg_set_level(vreg_ldo15, 3050);
    if (rc) {
        pr_err("%s: vreg LDO15 set level failed (%d)\n",
                __func__, rc);
        return rc;
    }
    if (on) 
        rc = vreg_enable(vreg_ldo11);
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
    else
    {
        if(product_id == Product_SF6)
        {
            rc = vreg_disable(vreg_ldo11);
            /* DIV5-MM-KW-pull down immediately { */
            vreg_pull_down_switch(vreg_ldo11, 1);
            /* } DIV5-MM-KW-pull down immediately */
            //pr_err("%s: SF6 turn off LDO11, GP2 1.8V",  __func__);
        }
    }
/* } FIHTDC-SW5-MULTIMEDIA, Chance */ 
    /* FIHTDC, Div2-SW2-BSP, Ming, LCM { */
    /* Use PMIC_GPIO_LCM_V1P8_EN to control LCM_V1P8 when display off */
    ///else
    ///     rc = vreg_disable(vreg_ldo11);
    /* } FIHTDC, Div2-SW2-BSP, Ming, LCM */     
    if (rc) {
        pr_err("%s: LDO11 vreg enable failed (%d)\n",
                __func__, rc);                  
        return rc;
    }                  
        
    if (on)
        rc = vreg_enable(vreg_ldo15);
    else
    {
        rc = vreg_disable(vreg_ldo15);
        /* DIV5-MM-KW-pull down immediately { */
        if(product_id == Product_SF6)
        {
            vreg_pull_down_switch(vreg_ldo15, 1);
        }
        /* } DIV5-MM-KW-pull down immediately */
    }
                    
    if (rc) {
        pr_err("%s: LDO15 vreg enable failed (%d)\n",
                __func__, rc);
        return rc;
    }

    ///mdelay(5);      /* ensure power is stable */ /* FIHTDC-Div2-SW2-BSP, Ming */
        
    if (on) {
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
        if(product_id == Product_SF6)
        {
            mdelay(1); /* wait 1 ms */
            //pr_err("%s: SF6 wait 1 ms before reset pull high",  __func__);
        }
/* } FIHTDC-SW5-MULTIMEDIA, Chance */ 
        gpio_set_value(35, 1);  /* bring reset line high */
        ///mdelay(5);      /* 10 msec before IO can be accessed */ /* FIHTDC-Div2-SW2-BSP, Ming */
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
        if(product_id == Product_SF6)
        {
            mdelay(5); /* wait 1 ms */
            //pr_err("%s: SF6 wait 5 ms after reset pull high",  __func__);
        }
/* } FIHTDC-SW5-MULTIMEDIA, Chance */
    }
    if (on) {
        rc = pmapp_display_clock_config(1);
        if (rc) {
            pr_err("%s pmapp_display_clock_config rc=%d\n",
                    __func__, rc);
            return rc;
        }
    } else { 
        rc = pmapp_display_clock_config(0);
        if (rc) {
            pr_err("%s pmapp_display_clock_config rc=%d\n",
                    __func__, rc);
            return rc;
        }
    }           
    return rc;

}

#else // CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT 
//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
static unsigned wega_reset_gpio =
	GPIO_CFG(180, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);

static struct msm_gpio fluid_vee_reset_gpio[] = {
	{ GPIO_CFG(20, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "vee_reset" },
};

static int display_common_power(int on)
{
	int rc = 0, flag_on = !!on;
	static int display_common_power_save_on;
	struct vreg *vreg_ldo12, *vreg_ldo15 = NULL;
	struct vreg *vreg_ldo20, *vreg_ldo16, *vreg_ldo8 = NULL;

	if (display_common_power_save_on == flag_on)
		return 0;

	display_common_power_save_on = flag_on;

	if (on) {
		/* reset Toshiba WeGA chip -- toggle reset pin -- gpio_180 */
		rc = gpio_tlmm_config(wega_reset_gpio, GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, wega_reset_gpio, rc);
			return rc;
		}

		/* bring reset line low to hold reset*/
		gpio_set_value(180, 0);
	}

	/* Toshiba WeGA power -- has 3 power source */
	/* 1.5V -- LDO20*/
	vreg_ldo20 = vreg_get(NULL, "gp13");

	if (IS_ERR(vreg_ldo20)) {
		rc = PTR_ERR(vreg_ldo20);
		pr_err("%s: gp13 vreg get failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	/* 1.8V -- LDO12 */
	vreg_ldo12 = vreg_get(NULL, "gp9");

	if (IS_ERR(vreg_ldo12)) {
		rc = PTR_ERR(vreg_ldo12);
		pr_err("%s: gp9 vreg get failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	/* 2.6V -- LDO16 */
	vreg_ldo16 = vreg_get(NULL, "gp10");

	if (IS_ERR(vreg_ldo16)) {
		rc = PTR_ERR(vreg_ldo16);
		pr_err("%s: gp10 vreg get failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	if (machine_is_msm7x30_fluid()) {
		/* 1.8V -- LDO8 */
		vreg_ldo8 = vreg_get(NULL, "gp7");

		if (IS_ERR(vreg_ldo8)) {
			rc = PTR_ERR(vreg_ldo8);
			pr_err("%s: gp7 vreg get failed (%d)\n",
				__func__, rc);
			return rc;
		}
	} else {
		/* lcd panel power */
		/* 3.1V -- LDO15 */
		vreg_ldo15 = vreg_get(NULL, "gp6");

		if (IS_ERR(vreg_ldo15)) {
			rc = PTR_ERR(vreg_ldo15);
			pr_err("%s: gp6 vreg get failed (%d)\n",
				__func__, rc);
			return rc;
		}
	}

	rc = vreg_set_level(vreg_ldo20, 1500);
	if (rc) {
		pr_err("%s: vreg LDO20 set level failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	rc = vreg_set_level(vreg_ldo12, 1800);
	if (rc) {
		pr_err("%s: vreg LDO12 set level failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	rc = vreg_set_level(vreg_ldo16, 2600);
	if (rc) {
		pr_err("%s: vreg LDO16 set level failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	if (machine_is_msm7x30_fluid()) {
		rc = vreg_set_level(vreg_ldo8, 1800);
		if (rc) {
			pr_err("%s: vreg LDO8 set level failed (%d)\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = vreg_set_level(vreg_ldo15, 3100);
		if (rc) {
			pr_err("%s: vreg LDO15 set level failed (%d)\n",
				__func__, rc);
			return rc;
		}
	}

	if (on) {
		rc = vreg_enable(vreg_ldo20);
		if (rc) {
			pr_err("%s: LDO20 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		rc = vreg_enable(vreg_ldo12);
		if (rc) {
			pr_err("%s: LDO12 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		rc = vreg_enable(vreg_ldo16);
		if (rc) {
			pr_err("%s: LDO16 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		if (machine_is_msm7x30_fluid()) {
			rc = vreg_enable(vreg_ldo8);
			if (rc) {
				pr_err("%s: LDO8 vreg enable failed (%d)\n",
					__func__, rc);
				return rc;
			}
		} else {
			rc = vreg_enable(vreg_ldo15);
			if (rc) {
				pr_err("%s: LDO15 vreg enable failed (%d)\n",
					__func__, rc);
				return rc;
			}
		}

		mdelay(5);		/* ensure power is stable */

		if (machine_is_msm7x30_fluid()) {
			rc = msm_gpios_request_enable(fluid_vee_reset_gpio,
					ARRAY_SIZE(fluid_vee_reset_gpio));
			if (rc)
				pr_err("%s gpio_request_enable failed rc=%d\n",
							__func__, rc);
			else {
				/* assert vee reset_n */
				gpio_set_value(20, 1);
				gpio_set_value(20, 0);
				mdelay(1);
				gpio_set_value(20, 1);
			}
		}

		gpio_set_value(180, 1); /* bring reset line high */
		mdelay(10);	/* 10 msec before IO can be accessed */
		rc = pmapp_display_clock_config(1);
		if (rc) {
			pr_err("%s pmapp_display_clock_config rc=%d\n",
					__func__, rc);
			return rc;
		}

	} else {
		rc = vreg_disable(vreg_ldo20);
		if (rc) {
			pr_err("%s: LDO20 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}


		rc = vreg_disable(vreg_ldo16);
		if (rc) {
			pr_err("%s: LDO16 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		gpio_set_value(180, 0); /* bring reset line low */

		if (machine_is_msm7x30_fluid()) {
			rc = vreg_disable(vreg_ldo8);
			if (rc) {
				pr_err("%s: LDO8 vreg enable failed (%d)\n",
					__func__, rc);
				return rc;
			}
		} else {
			rc = vreg_disable(vreg_ldo15);
			if (rc) {
				pr_err("%s: LDO15 vreg enable failed (%d)\n",
					__func__, rc);
				return rc;
			}
		}

		mdelay(5);	/* ensure power is stable */

		rc = vreg_disable(vreg_ldo12);
		if (rc) {
			pr_err("%s: LDO12 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		if (machine_is_msm7x30_fluid()) {
			msm_gpios_disable_free(fluid_vee_reset_gpio,
					ARRAY_SIZE(fluid_vee_reset_gpio));
		}

		rc = pmapp_display_clock_config(0);
		if (rc) {
			pr_err("%s pmapp_display_clock_config rc=%d\n",
					__func__, rc);
			return rc;
		}
	}

	return rc;
}
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-
#endif // CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT 

//SW2-6-MM-JH-Display_Flag-00+
#ifdef CONFIG_FB_MSM_MDDI
static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
	*clk_rate *= 2;
	return 0;
}

static struct mddi_platform_data mddi_pdata = {
#ifndef CONFIG_FIH_CONFIG_GROUP
	.mddi_power_save = display_common_power,
#endif
	.mddi_sel_clk = msm_fb_mddi_sel_clk,
};
#endif // CONFIG_FB_MSM_MDDI
//SW2-6-MM-JH-Display_Flag-00-

static struct msm_panel_common_pdata mdp_pdata = {
	.hw_revision_addr = 0xac001270,
/* Div2-SW2-BSP-FBX-LEDS { */
#ifndef CONFIG_LEDS_FIH_FBX_PWM
	.gpio = 30,
#endif
/* } Div2-SW2-BSP-FBX-LEDS */
	.mdp_core_clk_rate = 122880000,
};

/* FIHTDC, Div2-SW2-BSP, Ming, LCM { */
#ifdef CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT 
static int lcd_panel_spi_gpio_num[] = {
			45, /* spi_clk */
			46, /* spi_cs  */
			47, /* spi_mosi */
			48, /* spi_miso */
		};

static struct msm_gpio lcd_panel_gpios[] = {
    { GPIO_CFG(18, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn0" },
    { GPIO_CFG(19, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn1" },
    { GPIO_CFG(20, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu0" },
    { GPIO_CFG(21, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu1" },
    { GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu2" },
    { GPIO_CFG(23, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red0" },
    { GPIO_CFG(24, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red1" },
    { GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red2" },
    { GPIO_CFG(45, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_clk" },
    { GPIO_CFG(46, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_UP,   GPIO_CFG_2MA), "spi_cs0" },
    { GPIO_CFG(47, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_mosi" },
    { GPIO_CFG(48, 0, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_miso" },
    { GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), "lcdc_pclk" }, // FIHTDC-Div2-SW2-BSP, Ming, 4mA
/// { GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_en" },
    { GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_vsync" },
    { GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_hsync" },
    { GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn2" },
    { GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn3" },
    { GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn4" },
    { GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn5" },
/// { GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_grn6" },
/// { GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_grn7" },
    { GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu3" },
    { GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu4" },
    { GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu5" },
/// { GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_blu6" },
/// { GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_blu7" },
    { GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red3" },
    { GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red4" },
    { GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red5" },
/// { GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_red6" },
/// { GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_red7" },
};
/* FIHTDC, Div2-SW2-BSP, Ming, SPI for PR1 { */ 
#ifdef CONFIG_FIH_PROJECT_FBX
static struct msm_gpio lcd_panel_gpios_pr1[] = {
    { GPIO_CFG(18, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn0" },
    { GPIO_CFG(19, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn1" },
    { GPIO_CFG(20, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu0" },
    { GPIO_CFG(21, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu1" },
    { GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu2" },
    { GPIO_CFG(23, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red0" },
    { GPIO_CFG(24, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red1" },
    { GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red2" },
    { GPIO_CFG(45, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_clk" },
    { GPIO_CFG(46, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_UP,   GPIO_CFG_2MA), "spi_cs0" },
/// { GPIO_CFG(47, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_mosi" },
/// { GPIO_CFG(48, 0, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_miso" },
    { GPIO_CFG(48, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_mosi" },
    { GPIO_CFG(47, 0, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "spi_miso" },
    { GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), "lcdc_pclk" }, // FIHTDC-Div2-SW2-BSP, Ming, 4mA
/// { GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_en" },
    { GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_vsync" },
    { GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_hsync" },
    { GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn2" },
    { GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn3" },
    { GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn4" },
    { GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_grn5" },
/// { GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_grn6" },
/// { GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_grn7" },
    { GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu3" },
    { GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu4" },
    { GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_blu5" },
/// { GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_blu6" },
/// { GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_blu7" },
    { GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red3" },
    { GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red4" },
    { GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "lcdc_red5" },
/// { GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_red6" },
/// { GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), "lcdc_red7" },
};
#endif
/* } FIHTDC, Div2-SW2-BSP, Ming, SPI for PR1 */ 
#else // CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT    

//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
static struct msm_gpio lcd_panel_gpios[] = {
/* Workaround, since HDMI_INT is using the same GPIO line (18), and is used as
 * input.  if there is a hardware revision; we should reassign this GPIO to a
 * new open line; and removing it will just ensure that this will be missed in
 * the future.
	{ GPIO_CFG(18, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn0" },
 */
	{ GPIO_CFG(19, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn1" },
	{ GPIO_CFG(20, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu0" },
	{ GPIO_CFG(21, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu1" },
	{ GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu2" },
	{ GPIO_CFG(23, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red0" },
	{ GPIO_CFG(24, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red1" },
	{ GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red2" },
#ifndef CONFIG_SPI_QSD
	{ GPIO_CFG(45, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
#endif
	{ GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_pclk" },
	{ GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_en" },
	{ GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_vsync" },
	{ GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_hsync" },
	{ GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn2" },
	{ GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn3" },
	{ GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn4" },
	{ GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn5" },
	{ GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn6" },
	{ GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn7" },
	{ GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu3" },
	{ GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu4" },
	{ GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu5" },
	{ GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu6" },
	{ GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu7" },
	{ GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red3" },
	{ GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red4" },
	{ GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red5" },
	{ GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red6" },
	{ GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red7" },
};
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-

#endif // CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT 
/* } FIHTDC, Div2-SW2-BSP, Ming, LCM */

//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
static struct msm_gpio lcd_sharp_panel_gpios[] = {
	{ GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu2" },
	{ GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red2" },
	{ GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_pclk" },
	{ GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_en" },
	{ GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_vsync" },
	{ GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_hsync" },
	{ GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn2" },
	{ GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn3" },
	{ GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn4" },
	{ GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn5" },
	{ GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn6" },
	{ GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn7" },
	{ GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu3" },
	{ GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu4" },
	{ GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu5" },
	{ GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu6" },
	{ GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu7" },
	{ GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red3" },
	{ GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red4" },
	{ GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red5" },
	{ GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red6" },
	{ GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red7" },
};
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-

//SW2-6-MM-JH-Display_Flag-00+
#ifdef CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT
static int lcdc_toshiba_panel_power(int on)
{
	int rc, i;
	struct msm_gpio *gp;
    /* DIV5-MM-KW-get product id { */
    int product_id = 0;

    product_id = fih_get_product_id();
    /* } DIV5-MM-KW-get product id */

	rc = display_common_power(on);
	if (rc < 0) {
		printk(KERN_ERR "%s display_common_power failed: %d\n",
				__func__, rc);
		return rc;
	}

    /* FIHTDC, Div2-SW2-BSP, Ming, SPI for PR1 { */ 
    if (fih_get_product_phase()>=Product_PR2||(fih_get_product_id()!=Product_FB0 && fih_get_product_id()!=Product_FD1)) {
        if (on) {
            rc = msm_gpios_enable(lcd_panel_gpios,
                    ARRAY_SIZE(lcd_panel_gpios));
            if (rc < 0) {
                printk(KERN_ERR "%s: gpio enable failed: %d\n",
                    __func__, rc);
            }
        } else {    /* off */
            gp = lcd_panel_gpios;
            for (i = 0; i < ARRAY_SIZE(lcd_panel_gpios); i++) {
                /* ouput low */
                gpio_set_value(GPIO_PIN(gp->gpio_cfg), 0);
                gp++;
            }

            /* DIV5-MM-KW-pull down immediately { */
            if(product_id == Product_SF6)
            {
                gpio_tlmm_config(GPIO_CFG(92, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //lcdc_vsync
                gpio_tlmm_config(GPIO_CFG(93, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //lcdc_hsync
            }
            /* } DIV5-MM-KW-pull down immediately */
        }
    } else { // FB0_PR1, different spi configuration
#ifdef CONFIG_FIH_PROJECT_FBX
        if (on) {
            rc = msm_gpios_enable(lcd_panel_gpios_pr1,
                    ARRAY_SIZE(lcd_panel_gpios_pr1));
            if (rc < 0) {
                printk(KERN_ERR "%s: pr1 gpio config failed: %d\n",
                    __func__, rc);
            }
        } else {    /* off */
            gp = lcd_panel_gpios_pr1;
            for (i = 0; i < ARRAY_SIZE(lcd_panel_gpios_pr1); i++) {
                /* ouput low */
                gpio_set_value(GPIO_PIN(gp->gpio_cfg), 0);
                gp++;
            }
        }
#endif
    }
    /* } FIHTDC, Div2-SW2-BSP, Ming, SPI for PR1 */     

	return rc;
}
#endif
//SW2-6-MM-JH-Display_Flag-00-

//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
static int lcdc_sharp_panel_power(int on)
{
	int rc, i;
	struct msm_gpio *gp;

	rc = display_common_power(on);
	if (rc < 0) {
		printk(KERN_ERR "%s display_common_power failed: %d\n",
				__func__, rc);
		return rc;
	}

	if (on) {
		rc = msm_gpios_enable(lcd_sharp_panel_gpios,
				ARRAY_SIZE(lcd_sharp_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
		}
	} else {	/* off */
		gp = lcd_sharp_panel_gpios;
		for (i = 0; i < ARRAY_SIZE(lcd_sharp_panel_gpios); i++) {
			/* ouput low */
			gpio_set_value(GPIO_PIN(gp->gpio_cfg), 0);
			gp++;
		}
	}

	return rc;
}
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-

//SW2-6-MM-JH-Display_Flag-00+
#ifdef CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT 
static int lcdc_panel_power(int on)
{
	int flag_on = !!on;
	static int lcdc_power_save_on;

	if (lcdc_power_save_on == flag_on)
		return 0;

	lcdc_power_save_on = flag_on;

//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
	if (machine_is_msm7x30_fluid())
		return lcdc_sharp_panel_power(on);
	else
		return lcdc_toshiba_panel_power(on);
#else
        return lcdc_toshiba_panel_power(on);
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-
}
#endif // CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT
//SW2-6-MM-JH-Display_Flag-00-

//SW2-6-MM-JH-Display_Flag-00+
#ifdef CONFIG_FB_MSM_LCDC
static struct lcdc_platform_data lcdc_pdata = {
#ifdef CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT 
	.lcdc_power_save   = lcdc_panel_power,
#endif
};
#endif // CONFIG_FB_MSM_LCDC
//SW2-6-MM-JH-Display_Flag-00-

static int atv_dac_power(int on)
{
	int rc = 0;
	struct vreg *vreg_s4, *vreg_ldo9;

	vreg_s4 = vreg_get(NULL, "s4");
	if (IS_ERR(vreg_s4)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			   __func__, PTR_ERR(vreg_s4));
		return -1;
	}

	if (on) {
		rc = vreg_enable(vreg_s4);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = vreg_disable(vreg_s4);
		if (rc) {
			pr_err("%s: S4 vreg enable failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	}

	vreg_ldo9 = vreg_get(NULL, "gp1");
	if (IS_ERR(vreg_ldo9)) {
		rc = PTR_ERR(vreg_ldo9);
		pr_err("%s: gp1 vreg get failed (%d)\n",
			   __func__, rc);
		return rc;
	}


	if (on) {
		rc = vreg_enable(vreg_ldo9);
		if (rc) {
			pr_err("%s: LDO9 vreg enable failed (%d)\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = vreg_disable(vreg_ldo9);
		if (rc) {
			pr_err("%s: LDO20 vreg enable failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	}

	return rc;
}

static struct tvenc_platform_data atv_pdata = {
	.pm_vid_en   = atv_dac_power,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
//SW2-6-MM-JH-Display_Flag-00+
#ifdef CONFIG_FB_MSM_MDDI
	msm_fb_register_device("pmdh", &mddi_pdata);
#endif
#ifdef CONFIG_FB_MSM_LCDC
	msm_fb_register_device("lcdc", &lcdc_pdata);
#endif
//SW2-6-MM-JH-Display_Flag-00-
#if defined(CONFIG_FB_MSM_HDMI_ADV7525_PANEL) || defined(CONFIG_FB_MSM_HDMI_ADV7520_PANEL) 	
	msm_fb_register_device("dtv", &dtv_pdata);
#endif	
	msm_fb_register_device("tvenc", &atv_pdata);
}

//SW2-6-MM-JH-Display_Flag-00+
#ifdef CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT
static struct msm_panel_common_pdata lcdc_toshiba_panel_data = {
	.gpio_num          = lcd_panel_spi_gpio_num,
};

static struct platform_device lcdc_toshiba_panel_device = {
	.name   = "lcdc_toshiba_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_toshiba_panel_data,
	}
};
#endif // CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT
//SW2-6-MM-JH-Display_Flag-00-

// FIHTDC-SW2-Div6-CW-Project BCM4329 For SF8 +[
#if defined(CONFIG_BROADCOM_BCM4329) && \
    (defined(CONFIG_BROADCOM_BCM4329_BLUETOOTH_POWER) || defined(CONFIG_BROADCOM_BCM4329_WLAN_POWER))
#define BT_MASK     0x01
#define WLAN_MASK   0x02
#define FM_MASK     0x04

#define GPIO_WLAN_BT_REG_ON   168
static unsigned int bcm4329_power_status = 0;
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 For SF8 +]

// FIHTDC-SW2-Div6-CW-Project BCM4329 Bluetooth driver For SF8 +[
#ifdef CONFIG_BROADCOM_BCM4329_BLUETOOTH_POWER
#define GPIO_BTUART_RFR    134
#define GPIO_BTUART_CTS    135
#define GPIO_BTUART_RX     136
#define GPIO_BTUART_TX     137
#define GPIO_PCM_DIN       138
#define GPIO_PCM_DOUT      139
#define GPIO_PCM_SYNC      140
#define GPIO_PCM_BCLK      141

#define GPIO_BT_RST_N      144
#define GPIO_BT_IRQ        147
#define GPIO_BT_WAKEUP     170

static struct platform_device bcm4329_bt_power_device = {
    .name = "bcm4329_bt_power",
    .id     = -1
};

static struct msm_gpio bt_config_power_on[] = {

    { GPIO_CFG(GPIO_BTUART_RFR, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,  GPIO_CFG_2MA), "UART1DM_RFR" },
    { GPIO_CFG(GPIO_BTUART_CTS, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,  GPIO_CFG_2MA), "UART1DM_CTS" },
    { GPIO_CFG(GPIO_BTUART_RX,  1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,  GPIO_CFG_2MA), "UART1DM_RX" },
    { GPIO_CFG(GPIO_BTUART_TX,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,  GPIO_CFG_2MA), "UART1DM_TX" }
};

static struct msm_gpio bt_config_power_off[] = {
 
    { GPIO_CFG(GPIO_BTUART_RFR, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,  GPIO_CFG_2MA), "UART1DM_RFR" },
    { GPIO_CFG(GPIO_BTUART_CTS, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,  GPIO_CFG_2MA), "UART1DM_CTS" },
    { GPIO_CFG(GPIO_BTUART_RX,  0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,  GPIO_CFG_2MA), "UART1DM_RX" },
    { GPIO_CFG(GPIO_BTUART_TX,  0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,  GPIO_CFG_2MA), "UART1DM_TX" }
};

static int bluetooth_power(int on)
{
    int ret = 0;

    printk("KERN_DEBUG %s: POWER %s\r\n", __FUNCTION__, on?"ON":"OFF");

    if (on)
    {
        if ((bcm4329_power_status & ~WLAN_MASK) != 0)
        {
            printk("KERN_DEBUG %s: FM has been enable the power\r\n", __FUNCTION__);
            bcm4329_power_status |= BT_MASK;

            return 0;
        }
  
        ret = bluetooth_fm_power(on);
        if (ret < 0)
        {
            printk(KERN_DEBUG "%s: Power ON bluetooth failed.\r\n", __FUNCTION__);
            return ret;
        }

        bcm4329_power_status |= BT_MASK;
    }
    else
    {
        if ((bcm4329_power_status & ~(WLAN_MASK | BT_MASK)) != 0)
        {
            printk("KERN_DEBUG %s: FM enabled, can't turn bcm4329 bt/fm power\r\n", __FUNCTION__);
            bcm4329_power_status &= ~BT_MASK;

            return 0;
        }

        bcm4329_power_status &= ~BT_MASK;

        ret = bluetooth_fm_power(on);
        if (ret < 0)
        {
            printk(KERN_DEBUG "%s: Power ON bluetooth failed.\r\n", __FUNCTION__);
            return ret;
        }
    }

    return 0;
}

static int bluetooth_fm_power(int on)
{
    int rc;

    printk("KERN_DEBUG %s: POWER %s\r\n", __FUNCTION__, on?"ON":"OFF");

    if (on)
    {
        rc = msm_gpios_enable(bt_config_power_on, ARRAY_SIZE(bt_config_power_on));
        if (rc < 0)
        {
            printk(KERN_DEBUG "%s: Power ON bluetooth failed.\r\n", __FUNCTION__);
            return rc;
        }

        if (bcm4329_power_status == 0)
        {
            printk(KERN_DEBUG "%s: PULL UP GPIO_WLAN_BT_REG_ON\r\n", __FUNCTION__);
            gpio_set_value(GPIO_WLAN_BT_REG_ON, 1);
            mdelay(20);
        }

        gpio_set_value(GPIO_BT_RST_N, 0);
        mdelay(20);
        gpio_set_value(GPIO_BT_RST_N, 1);
        mdelay(100);

        printk(KERN_DEBUG "%s: GPIO_BT_RST (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_BT_RST_N)?"HIGH":"LOW");
        printk(KERN_DEBUG "%s: GPIO_WLAN_BT_REG_ON (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_WLAN_BT_REG_ON)?"HIGH":"LOW");
    }
    else
    {
        rc = msm_gpios_enable(bt_config_power_off, ARRAY_SIZE(bt_config_power_off));
        if (rc < 0)
        {
            printk(KERN_DEBUG "%s: Power OFF bluetooth failed.\r\n", __FUNCTION__);
            return rc;
        }

        gpio_set_value(GPIO_BT_RST_N, 0);

        if (bcm4329_power_status == 0)
        {
            printk(KERN_DEBUG "%s: PULL DOWN GPIO_WLAN_BT_REG_ON\r\n", __FUNCTION__);
            gpio_set_value(GPIO_WLAN_BT_REG_ON, 0);
        }

        mdelay(100);

        printk(KERN_DEBUG "%s: GPIO_BT_RST (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_BT_RST_N)?"HIGH":"LOW");
        printk(KERN_DEBUG "%s: GPIO_WLAN_BT_REG_ON (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_WLAN_BT_REG_ON)?"HIGH":"LOW");
    }

    return 0;
}

static void __init bcm4329_bt_power_init(void)
{
    gpio_set_value(GPIO_WLAN_BT_REG_ON, 0);
    gpio_set_value(GPIO_BT_RST_N, 0);

    bcm4329_bt_power_device.dev.platform_data = &bluetooth_power;
}
#else
//#define bt_power_init(x) do {} while (0)
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 Bluetooth driver For SF8 +]

#if defined(CONFIG_BROADCOM_BCM4329)
static struct platform_device bcm4329_fm_power_device = {
    .name = "bcm4329_fm_power",
    .id     = -1
};

static int fm_power(int on)
{
    int ret = 0;

    printk("KERN_DEBUG %s: POWER %s\r\n", __FUNCTION__, on?"ON":"OFF");

    if (on)
    {
        if ((bcm4329_power_status & ~WLAN_MASK) != 0)
        {
            printk("KERN_DEBUG %s: Bluetooth has been enable the power\r\n", __FUNCTION__);
            bcm4329_power_status |= FM_MASK;

            return 0;
        }
  
        ret = bluetooth_fm_power(on);
        if (ret < 0)
        {
            printk(KERN_DEBUG "%s: Power ON FM failed.\r\n", __FUNCTION__);
            return ret;
        }

        bcm4329_power_status |= FM_MASK;
    }
    else
    {
        if ((bcm4329_power_status & ~(WLAN_MASK | FM_MASK)) != 0)
        {
            printk("KERN_DEBUG %s: Bluetooth enabled, can't turn bcm4329 bt/fm power\r\n", __FUNCTION__);
            bcm4329_power_status &= ~FM_MASK;

            return 0;
        }

        bcm4329_power_status &= ~FM_MASK;

        ret = bluetooth_fm_power(on);
        if (ret < 0)
        {
            printk(KERN_DEBUG "%s: Power ON FM failed.\r\n", __FUNCTION__);
            return ret;
        }
    }

    return 0;
}

static void __init bcm4329_fm_power_init(void)
{
    bcm4329_fm_power_device.dev.platform_data = &fm_power;
}
#endif

// FIHTDC-SW2-Div6-CW-Project BCM4329 WLAN driver For SF8 +[
#ifdef CONFIG_BROADCOM_BCM4329_WLAN_POWER
#define GPIO_WLAN_RST_N      146
#define GPIO_WLAN_IRQ        145
#define GPIO_WLAN_WAKEUP     169

static struct platform_device bcm4329_wifi_power_device = {
    .name = "bcm4329_wifi_power",
    .id     = -1
};

int wifi_power(int on)  //SW5-PT1-Connectivity_FredYu_FixPowerSequence
{
    printk(KERN_DEBUG "%s: POWER %s\r\n", __FUNCTION__, on?"ON":"OFF");

    if (on)
    {
        if (bcm4329_power_status == 0)
        {
            printk(KERN_DEBUG "%s: PULL UP GPIO_WLAN_BT_REG_ON\r\n", __FUNCTION__);
            gpio_set_value(GPIO_WLAN_BT_REG_ON, 1);
            mdelay(20);
        }

        gpio_set_value(GPIO_WLAN_RST_N, 0);
        mdelay(20);
        gpio_set_value(GPIO_WLAN_RST_N, 1);
        bcm4329_power_status |= WLAN_MASK;

        printk(KERN_DEBUG "%s: GPIO_WLAN_RST (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_WLAN_RST_N)?"HIGH":"LOW");
        printk(KERN_DEBUG "%s: GPIO_WLAN_BT_REG_ON (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_WLAN_BT_REG_ON)?"HIGH":"LOW");
    }
    else
    {
        bcm4329_power_status &= ~WLAN_MASK;
        gpio_set_value(GPIO_WLAN_RST_N, 0);

        if (bcm4329_power_status == 0)
        {
            printk(KERN_DEBUG "%s: PULL DOWN GPIO_WLAN_BT_REG_ON\r\n", __FUNCTION__);
            gpio_set_value(GPIO_WLAN_BT_REG_ON, 0);
        }

        printk(KERN_DEBUG "%s: GPIO_WLAN_RST (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_WLAN_RST_N)?"HIGH":"LOW");
        printk(KERN_DEBUG "%s: GPIO_WLAN_BT_REG_ON (%s)\r\n", __FUNCTION__, gpio_get_value(GPIO_WLAN_BT_REG_ON)?"HIGH":"LOW");
    }

    return 0;
}
EXPORT_SYMBOL(wifi_power);  //SW5-PT1-Connectivity_FredYu_FixPowerSequence

int bcm4329_wifi_resume(void)
{
    printk(KERN_DEBUG "%s: START\r\n", __FUNCTION__);

    gpio_set_value(GPIO_WLAN_RST_N, 0);
    mdelay(300);
    gpio_set_value(GPIO_WLAN_RST_N, 1);

    return 0;
}
EXPORT_SYMBOL(bcm4329_wifi_resume);

int bcm4329_wifi_suspend(void)
{
    printk(KERN_DEBUG "%s: START\r\n", __FUNCTION__);

    gpio_set_value(GPIO_WLAN_RST_N, 0);

    return 0;
}
EXPORT_SYMBOL(bcm4329_wifi_suspend);

static void __init bcm4329_wifi_power_init(void)
{
    gpio_set_value(GPIO_WLAN_BT_REG_ON, 0);
    gpio_set_value(GPIO_WLAN_RST_N, 0);

    bcm4329_wifi_power_device.dev.platform_data = &wifi_power;
}
#else
#define wifi_power_init(x) do {} while (0)
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 WLAN driver For SF8 +]

#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
	.id     = -1
};

enum {
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
};

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(134, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Tx" }
};

static struct msm_gpio bt_config_power_off[] = {
	{ GPIO_CFG(134, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Tx" }
};

static struct msm_gpio bt_config_clock[] = {
	{ GPIO_CFG(34, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,    GPIO_CFG_2MA),
		"BT_REF_CLOCK_ENABLE" },
};

static int marimba_bt(int on)
{
	int rc;
	int i;
	struct marimba config = { .mod_id = MARIMBA_SLAVE_ID_MARIMBA };

	struct marimba_config_register {
		u8 reg;
		u8 value;
		u8 mask;
	};

	struct marimba_variant_register {
		const size_t size;
		const struct marimba_config_register *set;
	};

	const struct marimba_config_register *p;

	u8 version;

	const struct marimba_config_register v10_bt_on[] = {
		{ 0xE5, 0x0B, 0x0F },
		{ 0x05, 0x02, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v10_bt_off[] = {
		{ 0xE5, 0x0B, 0x0F },
		{ 0x05, 0x08, 0x0F },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_config_register v201_bt_on[] = {
		{ 0x05, 0x08, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v201_bt_off[] = {
		{ 0x05, 0x08, 0x07 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_config_register v210_bt_on[] = {
		{ 0xE9, 0x01, 0x01 },
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x21, 0x21 },
		{ 0xE3, 0x38, 0xFF },
		{ 0xE4, 0x06, 0xFF },
	};

	const struct marimba_config_register v210_bt_off[] = {
		{ 0x06, 0x88, 0xFF },
		{ 0xE7, 0x00, 0x21 },
		{ 0xE9, 0x00, 0x01 },
		{ 0xE3, 0x00, 0xFF },
		{ 0xE4, 0x00, 0xFF },
	};

	const struct marimba_variant_register bt_marimba[2][4] = {
		{
			{ ARRAY_SIZE(v10_bt_off), v10_bt_off },
			{ 0, NULL },
			{ ARRAY_SIZE(v201_bt_off), v201_bt_off },
			{ ARRAY_SIZE(v210_bt_off), v210_bt_off }
		},
		{
			{ ARRAY_SIZE(v10_bt_on), v10_bt_on },
			{ 0, NULL },
			{ ARRAY_SIZE(v201_bt_on), v201_bt_on },
			{ ARRAY_SIZE(v210_bt_on), v210_bt_on }
		}
	};

	on = on ? 1 : 0;

	rc = marimba_read_bit_mask(&config, 0x11,  &version, 1, 0x1F);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: version read failed: %d\n",
			__func__, rc);
		return rc;
	}

	if ((version >= ARRAY_SIZE(bt_marimba[on])) ||
	    (bt_marimba[on][version].size == 0)) {
		printk(KERN_ERR
			"%s: unsupported version\n",
			__func__);
		return -EIO;
	}

	p = bt_marimba[on][version].set;

	printk(KERN_INFO "%s: found version %d\n", __func__, version);

	for (i = 0; i < bt_marimba[on][version].size; i++) {
		u8 value = (p+i)->value;
		rc = marimba_write_bit_mask(&config,
			(p+i)->reg,
			&value,
			sizeof((p+i)->value),
			(p+i)->mask);
		if (rc < 0) {
			printk(KERN_ERR
				"%s: reg %d write failed: %d\n",
				__func__, (p+i)->reg, rc);
			return rc;
		}
		printk(KERN_INFO "%s: reg 0x%02x value 0x%02x mask 0x%02x\n",
				__func__, (p+i)->reg,
				value, (p+i)->mask);
	}
	return 0;
}

static const char *vregs_bt_name[] = {
	"gp16",
	"s2",
	"wlan"
};
static struct vreg *vregs_bt[ARRAY_SIZE(vregs_bt_name)];

static int bluetooth_power_regulators(int on)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_bt_name); i++) {
		rc = on ? vreg_enable(vregs_bt[i]) :
			  vreg_disable(vregs_bt[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
				__func__, vregs_bt_name[i],
			       on ? "enable" : "disable", rc);
			return -EIO;
		}
	}
	return 0;
}

int fih_bluetooth_status;

static int bluetooth_power(int on)
{
	int rc;
	const char *id = "BTPW";
    
	if (on) {
		rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 1300);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg level on failed (%d)\n",
				__func__, rc);
			return rc;
		}

		rc = bluetooth_power_regulators(on);
		if (rc < 0)
			return -EIO;

		/* } FIHTDC, Div2-SW2-BSP Godfrey, BT  */
		if( ( fih_get_product_id() == Product_FD1 ) && ( ( fih_get_product_phase() != Product_PR1 ) &&
														( fih_get_product_phase() != Product_PR2p5 ) &&
														( fih_get_product_phase() != Product_PR230 ) &&
														( fih_get_product_phase() != Product_PR232 ) &&
														( fih_get_product_phase() != Product_PR3 ) &&
														( fih_get_product_phase() != Product_PR4 ) ) ){
			rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
						  PMAPP_CLOCK_VOTE_ON);
		}
		else{
			rc = pmapp_clock_vote(id, QTR8x00_WCN_CLK,
						  PMAPP_CLOCK_VOTE_ON);
		}
		if (rc < 0)
			return -EIO;

		if (machine_is_msm8x55_svlte_surf() ||
				machine_is_msm8x55_svlte_ffa())
			gpio_set_value(
				GPIO_PIN(bt_config_clock->gpio_cfg), 1);

		rc = marimba_bt(on);
		if (rc < 0)
			return -EIO;

		msleep(10);

		/* } FIHTDC, Div2-SW2-BSP Godfrey, BT  */
		if( ( fih_get_product_id() == Product_FD1 ) && ( ( fih_get_product_phase() != Product_PR1 ) &&
														( fih_get_product_phase() != Product_PR2p5 ) &&
														( fih_get_product_phase() != Product_PR230 ) &&
														( fih_get_product_phase() != Product_PR232 ) &&
														( fih_get_product_phase() != Product_PR3 ) &&
														( fih_get_product_phase() != Product_PR4 ) ) ){
            rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
            			PMAPP_CLOCK_VOTE_PIN_CTRL);
		}
		else{
			rc = pmapp_clock_vote(id, QTR8x00_WCN_CLK,
						PMAPP_CLOCK_VOTE_PIN_CTRL);
		}
		
		if (rc < 0)
		    return -EIO;

		if (machine_is_msm8x55_svlte_surf() ||
				machine_is_msm8x55_svlte_ffa())
			gpio_set_value(
				GPIO_PIN(bt_config_clock->gpio_cfg), 0);

		rc = msm_gpios_enable(bt_config_power_on,
			ARRAY_SIZE(bt_config_power_on));

		if (rc < 0)
			return rc;

	} else {
		rc = msm_gpios_enable(bt_config_power_off,
					ARRAY_SIZE(bt_config_power_off));
		if (rc < 0)
			return rc;

		/* check for initial RFKILL block (power off) */
		if (platform_get_drvdata(&msm_bt_power_device) == NULL)
			goto out;

		rc = marimba_bt(on);
		if (rc < 0)
			return -EIO;

		/* } FIHTDC, Div2-SW2-BSP Godfrey, BT  */
		if( ( fih_get_product_id() == Product_FD1 ) && ( ( fih_get_product_phase() != Product_PR1 ) &&
														( fih_get_product_phase() != Product_PR2p5 ) &&
														( fih_get_product_phase() != Product_PR230 ) &&
														( fih_get_product_phase() != Product_PR232 ) &&
														( fih_get_product_phase() != Product_PR3 ) &&
														( fih_get_product_phase() != Product_PR4 ) ) ){
			rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
						  PMAPP_CLOCK_VOTE_OFF);
		}
		else{
			rc = pmapp_clock_vote(id, QTR8x00_WCN_CLK,
						  PMAPP_CLOCK_VOTE_OFF);
		}
		if (rc < 0)
			return -EIO;

		rc = bluetooth_power_regulators(on);
		if (rc < 0)
			return -EIO;

		rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 0);
		if (rc < 0) {
			printk(KERN_INFO "%s: vreg level off failed (%d)\n",
				__func__, rc);
		}
	}

	// Update the bluetooth status which is used in the detect charger function.
	fih_bluetooth_status = on; 

out:
	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	return 0;
}

static void __init bt_power_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_bt_name); i++) {
		vregs_bt[i] = vreg_get(NULL, vregs_bt_name[i]);
		if (IS_ERR(vregs_bt[i])) {
			printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
			       __func__, vregs_bt_name[i],
			       PTR_ERR(vregs_bt[i]));
			return;
		}
	}

	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa()) {
		rc = msm_gpios_request_enable(bt_config_clock,
					ARRAY_SIZE(bt_config_clock));
		if (rc < 0) {
			printk(KERN_ERR
				"%s: msm_gpios_request_enable failed (%d)\n",
					__func__, rc);
			return;
		}

		rc = gpio_direction_output(GPIO_PIN(bt_config_clock->gpio_cfg),
						0);
		if (rc < 0) {
			printk(KERN_ERR
				"%s: gpio_direction_output failed (%d)\n",
					__func__, rc);
			return;
		}
	}


	msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#else
#define bt_power_init(x) do {} while (0)
#endif

#ifdef CONFIG_BATTERY_FIH_MSM
static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 2800,
	.voltage_max_design	= 4300,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
/* Div2-SW2-BSP-FBX-BATT { */
    .batt_info_if           = {
        .get_chg_source         = msm_batt_get_chg_source,
        .get_batt_status        = msm_batt_get_batt_status,
        .get_batt_capacity      = msm_batt_info_not_support,
        .get_batt_health        = msm_batt_info_not_support,
        .get_batt_temp          = msm_batt_info_not_support,
        .get_batt_voltage       = msm_batt_info_not_support,
    },
/* } Div2-SW2-BSP-FBX-BATT */
};
#endif

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
#ifdef CONFIG_BATTERY_FIH_MSM
	.dev.platform_data  = &msm_psy_batt_data,
#endif
};

/* Div2-SW2-BSP-FBX-BATT { */
#ifdef CONFIG_BATTERY_FIH_MSM
enum {
    HWMODEL_GAUGE_DS2784,
    HWMODEL_GAUGE_BQ27500,
};

static struct i2c_board_info fih_battery_i2c_board_info[] = {
#ifdef CONFIG_DS2482
    {
        I2C_BOARD_INFO("ds2482", 0x30 >> 1),
        .platform_data = &ds2482_pdata,
    },
#endif
#ifdef CONFIG_BATTERY_BQ275X0
    {
        I2C_BOARD_INFO("bq275x0-battery", 0xAA >> 1),
        .platform_data = &bq275x0_pdata,
    },
#ifdef CONFIG_BQ275X0_ROMMODE
    {
        I2C_BOARD_INFO("bq275x0-RomMode", 0x16 >> 1),        
    },
#endif
#endif
};

static int fih_battery_hw_model(void)
{
    int product_id = fih_get_product_id();
    int product_phase = fih_get_product_phase();
    
    if (product_id == Product_FB0 || product_id == Product_FD1)
        if (product_phase >= Product_EVB && product_phase < Product_PR231)
            return HWMODEL_GAUGE_DS2784;
        else
            return HWMODEL_GAUGE_BQ27500;
    else
        return HWMODEL_GAUGE_BQ27500;
}

/* 
 * Assign caculate_capacity and get_battery_status function by HWID
 */
static void __init fih_battery_driver_init(void)
{
    switch(fih_battery_hw_model()) {
    case HWMODEL_GAUGE_DS2784:
#ifdef CONFIG_DS2482
        	i2c_register_board_info(0, &fih_battery_i2c_board_info[0], 1);

#ifdef CONFIG_BATTERY_FIH_DS2784
        	msm_psy_batt_data.batt_info_if.get_batt_capacity = ds2784_batt_get_batt_capacity;
        	msm_psy_batt_data.batt_info_if.get_batt_health   = ds2784_batt_set_batt_health;
        	msm_psy_batt_data.batt_info_if.get_batt_temp     = ds2784_batt_get_batt_temp;
        	msm_psy_batt_data.batt_info_if.get_batt_voltage  = ds2784_batt_get_batt_voltage;
#endif
#endif
        break;
    case HWMODEL_GAUGE_BQ27500:
    default:
#ifdef CONFIG_BATTERY_BQ275X0
    #ifdef CONFIG_DS2482
            i2c_register_board_info(0, &fih_battery_i2c_board_info[1], 1);
        #ifdef CONFIG_BQ275X0_ROMMODE
            i2c_register_board_info(0, &fih_battery_i2c_board_info[2], 1);        
        #endif
    #else
            i2c_register_board_info(0, &fih_battery_i2c_board_info[0], 1);
        #ifdef CONFIG_BQ275X0_ROMMODE
            i2c_register_board_info(0, &fih_battery_i2c_board_info[1], 1);
        #endif
    #endif

        msm_psy_batt_data.batt_info_if.get_batt_capacity = bq275x0_battery_soc;
        msm_psy_batt_data.batt_info_if.get_batt_health   = bq275x0_battery_health;
        msm_psy_batt_data.batt_info_if.get_batt_temp     = bq275x0_battery_temperature;
        msm_psy_batt_data.batt_info_if.get_batt_voltage  = bq275x0_battery_voltage;
#endif		
    }
}
#endif
/* } Div2-SW2-BSP-FBX-BATT */

static char *msm_adc_fluid_device_names[] = {
	"LTC_ADC1",
	"LTC_ADC2",
	"LTC_ADC3",
};

static char *msm_adc_surf_device_names[] = {
	"XO_ADC",
};

static struct msm_adc_platform_data msm_adc_pdata;

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

#ifdef CONFIG_MSM_SDIO_AL
static struct msm_gpio mdm2ap_status = {
	GPIO_CFG(77, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	"mdm2ap_status"
};

struct platform_device msm_device_sdio_al = {
	.name = "msm_sdio_al",
	.id = -1,
	.dev		= {
		.platform_data	= &mdm2ap_status,
	},
};

#endif /* CONFIG_MSM_SDIO_AL */


/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_FIH_FBX_PWM
static struct leds_fbx_pwm_platform_data fbx_leds_pwm_pdata = {
    .r_led_ctl = 30
};

static struct platform_device fbx_leds_pwm_device = {
    .name   = "fbx-leds-pwm",
    .id     = -1,
    .dev    = {
                .platform_data = &fbx_leds_pwm_pdata,
            },
};
#endif
/* } Div2-SW2-BSP-FBX-LEDS */
/* Div2-SW2-BSP-FBX-KEYS { */
#ifdef CONFIG_KEYBOARD_FBX
static struct fbx_kybd_platform_data fbx_kybd_pdata = {
    .pmic_gpio_cam_f    = 1,
    .pmic_gpio_cam_t    = 2,
    .pmic_gpio_vol_up   = 3,
    .pmic_gpio_vol_dn   = 4,
    .sys_gpio_cam_f     = PM8058_GPIO_PM_TO_SYS(1),
    .sys_gpio_cam_t     = PM8058_GPIO_PM_TO_SYS(2),
    .sys_gpio_vol_up    = PM8058_GPIO_PM_TO_SYS(3),
    .sys_gpio_vol_dn    = PM8058_GPIO_PM_TO_SYS(4),
    .hook_sw_pin = 44, /* Div1-FW3-BSP-AUDIO */
};

static struct platform_device fbx_kybd_device = {
    .name   = "fbx_kybd",
    .id     = -1,
    .dev    = {
                .platform_data = &fbx_kybd_pdata,
            },
};
#endif
/* } Div2-SW2-BSP-FBX-KEYS */

//SW2-D5-OwenHung-SF4Y6 Keypad backlight/LEDs driver+
#ifdef CONFIG_LEDS_PWM_SF6
static struct platform_device sf6_leds_pwm_device = {
    .name   = "sf6-leds-pwm",
    .id     = -1,
};
#endif
//SW2-D5-OwenHung-SF4Y6 Keypad backlight/LEDs driver-

//SW2-D5-OwenHung-SF4Y6 keypad driver porting+
#ifdef CONFIG_FIH_PROJECT_SF4Y6
static struct sf6_kybd_platform_data sf6_kybd_pdata = {
    .pmic_gpio_cam_f    = 1,
    .pmic_gpio_cam_t    = 2,
    .pmic_gpio_vol_up   = 3,
    .pmic_gpio_vol_dn   = 4,
    .sys_gpio_cam_f     = PM8058_GPIO_PM_TO_SYS(1),
    .sys_gpio_cam_t     = PM8058_GPIO_PM_TO_SYS(2),
    .sys_gpio_vol_up    = PM8058_GPIO_PM_TO_SYS(3),
    .sys_gpio_vol_dn    = PM8058_GPIO_PM_TO_SYS(4),
};

static struct platform_device sf6_kybd_device = {
    .name   = "sf6_kybd",
    .id     = -1,
    .dev    = {
                .platform_data = &sf6_kybd_pdata,
            },
};
#endif
//SW2-D5-OwenHung-SF4Y6 keypad driver porting+

//SW2-D5-AH-SF4H8 keypad driver porting+
#ifdef CONFIG_KEYBOARD_SF4H8
static struct sf8_kybd_platform_data sf8_kybd_pdata = {
    .pmic_gpio_vol_up   = 0,
    .pmic_gpio_vol_dn   = 1,
    .pmic_gpio_cover_det= 2,    
    .sys_gpio_vol_up    = PM8058_GPIO_PM_TO_SYS(0),
    .sys_gpio_vol_dn    = PM8058_GPIO_PM_TO_SYS(1),
    .sys_gpio_cover_det = PM8058_GPIO_PM_TO_SYS(2),    
};

static struct platform_device sf8_kybd_device = {
    .name   = "sf8_kybd",
    .id     = -1,
    .dev    = {
                .platform_data = &sf8_kybd_pdata,
            },
};
#endif
//SW2-D5-AH-SF4H8 keypad driver porting+

//SW2-5-1-MP-DbgCfgTool-00+[ 
#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define RAM_CONSOLE_PHYS 0x7A00000
#define RAM_CONSOLE_SIZE 0x00020000
static struct resource ram_console_resources[1] = {
        [0] = {
                .start  = RAM_CONSOLE_PHYS,
                .end    = RAM_CONSOLE_PHYS + RAM_CONSOLE_SIZE - 1,
                .flags  = IORESOURCE_MEM,
        },
};

static struct platform_device ram_console_device = {
        .name   = "ram_console",
        .id     = 0,
        .num_resources  = ARRAY_SIZE(ram_console_resources),
        .resource       = ram_console_resources,

};
#endif

#ifdef CONFIG_FIH_LAST_ALOG
#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define ALOG_RAM_CONSOLE_PHYS_MAIN (RAM_CONSOLE_PHYS + RAM_CONSOLE_SIZE)
#else
#define ALOG_RAM_CONSOLE_PHYS_MAIN 0x7A20000
#endif  
#define ALOG_RAM_CONSOLE_SIZE_MAIN 0x00020000 //128KB
#define ALOG_RAM_CONSOLE_PHYS_RADIO (ALOG_RAM_CONSOLE_PHYS_MAIN +  ALOG_RAM_CONSOLE_SIZE_MAIN)
#define ALOG_RAM_CONSOLE_SIZE_RADIO 0x00020000 //128KB
#define ALOG_RAM_CONSOLE_PHYS_EVENTS (ALOG_RAM_CONSOLE_PHYS_RADIO + ALOG_RAM_CONSOLE_SIZE_RADIO) 
#define ALOG_RAM_CONSOLE_SIZE_EVENTS 0x00020000 //128KB
#define ALOG_RAM_CONSOLE_PHYS_SYSTEM (ALOG_RAM_CONSOLE_PHYS_EVENTS + ALOG_RAM_CONSOLE_SIZE_EVENTS) 
#define ALOG_RAM_CONSOLE_SIZE_SYSTEM 0x00020000 //128KB

static struct resource alog_ram_console_resources[4] = {
        [0] = {
        .name = "alog_main_buffer",
                .start  = ALOG_RAM_CONSOLE_PHYS_MAIN,
                .end    = ALOG_RAM_CONSOLE_PHYS_MAIN + ALOG_RAM_CONSOLE_SIZE_MAIN - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
            .name = "alog_radio_buffer",
                .start  = ALOG_RAM_CONSOLE_PHYS_RADIO,
                .end    = ALOG_RAM_CONSOLE_PHYS_RADIO + ALOG_RAM_CONSOLE_SIZE_RADIO - 1,
                .flags  = IORESOURCE_MEM,
        },
        [2] = {
        .name = "alog_events_buffer",
                .start  = ALOG_RAM_CONSOLE_PHYS_EVENTS,
                .end    = ALOG_RAM_CONSOLE_PHYS_EVENTS + ALOG_RAM_CONSOLE_SIZE_EVENTS - 1,
                .flags  = IORESOURCE_MEM,
        },
        [3] = {
		.name = "alog_system_buffer",
                .start  = ALOG_RAM_CONSOLE_PHYS_SYSTEM,
                .end    = ALOG_RAM_CONSOLE_PHYS_SYSTEM + ALOG_RAM_CONSOLE_SIZE_SYSTEM - 1,
                .flags  = IORESOURCE_MEM,
        },
};

static struct platform_device alog_ram_console_device = {
        .name   = "alog_ram_console",
        .id     = 0,
        .num_resources  = ARRAY_SIZE(alog_ram_console_resources),
        .resource       = alog_ram_console_resources,
};
#endif
//SW2-5-1-MP-DbgCfgTool-00+]

static struct platform_device *devices[] __initdata = {
#if defined(CONFIG_SERIAL_MSM) || defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart2,
#endif
	&msm_device_smd,
	&msm_device_dmov,
/* Div2-SW2-BSP-FBX-OW { */
#ifdef CONFIG_SMC91X
	&smc91x_device,
#endif
#ifdef CONFIG_SMSC911X
	&smsc911x_device,
#endif
/* } Div2-SW2-BSP-FBX-OW */
	&msm_device_nand,
#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_ANDROID
	&usb_mass_storage_device,
	&rndis_device,
	&android_usb_device,
#endif
//SW2-6-MM-JH-SPI-00+
#ifdef CONFIG_SPI_QSD
	&qsd_device_spi,
#endif
//SW2-6-MM-JH-SPI-00-
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi6,
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	&msm_migrate_pages_device,
//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
	&mddi_toshiba_device,
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-
//SW2-6-MM-JH-Display_Flag-00+
#ifdef CONFIG_FIH_LCDC_TOSHIBA_WVGA_PT 
	&lcdc_toshiba_panel_device,
#endif
//SW2-6-MM-JH-Display_Flag-00-
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00+
#if 0
	&lcdc_sharp_panel_device,
#endif
//SW2-6-MM-JH-Unused_Display_Codes-00-
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL 
	&hdmi_adv7520_panel_device,
#endif
#ifdef CONFIG_FB_MSM_HDMI_ADV7525_PANEL
	&hdmi_adv7525_panel_device,
#endif
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */  
	&android_pmem_kernel_ebi1_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+{
#ifdef CONFIG_MSM_UART2DM
	&msm_device_uart_dm2, 
#endif
//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+}
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
	&qup_device_i2c,
#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
	&msm_bt_power_device,
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 WLAN driver For SF8 +[
#ifdef CONFIG_BROADCOM_BCM4329_WLAN_POWER
	&bcm4329_wifi_power_device,
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 WLAN driver For SF8 +]
// FIHTDC-SW2-Div6-CW-Project BCM4329 BLUETOOTH driver For SF8 +[
#ifdef CONFIG_BROADCOM_BCM4329_BLUETOOTH_POWER
	&bcm4329_bt_power_device,
	&bcm4329_fm_power_device,
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 BLUETOOTH driver For SF8 +]
	&msm_device_kgsl,
#if CONFIG_MSM_HW3D
	&hw3d_device,
#endif
//Div2-SW6-MM-HL-Camera-BringUp-00+{
#ifdef CONFIG_FIH_MT9P111
        &msm_camera_sensor_mt9p111,
#endif
#ifdef CONFIG_FIH_HM0356
        &msm_camera_sensor_hm0356,
#endif
//Div2-SW6-MM-HL-Camera-BringUp-00+} 
#ifdef CONFIG_FIH_HM0357
        &msm_camera_sensor_hm0357,
#endif

//Div2-SW6-MM-MC-Camera-BringUpForSF5-00+{
#ifdef CONFIG_FIH_TCM9001MD
	&msm_camera_sensor_tcm9001md,
#endif
//Div2-SW6-MM-MC-Camera-BringUpForSF5-00+}

#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_MT9D112
	&msm_camera_sensor_mt9d112,
#endif
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_MT9P012
	&msm_camera_sensor_mt9p012,
#endif
#ifdef CONFIG_VX6953
	&msm_camera_sensor_vx6953,
#endif
#ifdef CONFIG_SN12M0PZ
	&msm_camera_sensor_sn12m0pz,
#endif
	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	&msm_device_tsif,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif
	&msm_batt_device,
	&msm_adc_device,
//  FTM phone function, Henry.Wang 2010.4.30+
#ifdef CONFIG_FIH_FTM_PHONE
	&ftm_phone_device,
#endif
//  FTM phone function, Henry.Wang 2010.4.30-
/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_FIH_FBX_PWM
	&fbx_leds_pwm_device,
#endif
/* } Div2-SW2-BSP-FBX-LEDS */
/* Div2-SW2-BSP-FBX-KEYS { */
#ifdef CONFIG_KEYBOARD_FBX
	&fbx_kybd_device,
#endif
/* } Div2-SW2-BSP-FBX-KEYS */
//SW2-D5-OwenHung-SF4Y6 Keypad backlight/LEDs driver+
#ifdef CONFIG_LEDS_PWM_SF6
	&sf6_leds_pwm_device,
#endif	
//SW2-D5-OwenHung-SF4Y6 Keypad backlight/LEDs driver-

//SW2-D5-AriesHuang-SF4V5/SF4H8 porting keypad backlight +{
#if defined(CONFIG_FIH_PROJECT_SF4V5) || defined(CONFIG_FIH_PROJECT_SF4Y6) || defined(CONFIG_FIH_PROJECT_SF8)
	&msm_device_pmic_leds,
#endif	
//SW2-D5-AriesHuang-SF4V5/SF4H8 porting keypad backlight +}

//SW2-D5-OwenHung-SF4Y6 keypad driver porting+
#ifdef CONFIG_FIH_PROJECT_SF4Y6
	&sf6_kybd_device,
#endif
//SW2-D5-OwenHung-SF4Y6 keypad driver porting-

//SW2-D5-AriesHuang-SF4H8 keypad driver porting +{
#ifdef CONFIG_KEYBOARD_SF4H8
    &sf8_kybd_device,
#endif
//SW2-D5-AriesHuang-SF4H8 keypad driver porting +}

/* Div1-FW3-BSP-AUDIO */
#ifdef CONFIG_FIH_FBX_AUDIO
	&headset_sensor_device,
#endif
//SW2-5-1-MP-DbgCfgTool-00+[
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
#ifdef CONFIG_FIH_LAST_ALOG
	&alog_ram_console_device,
#endif
//SW2-5-1-MP-DbgCftTool-00+]
};

static struct msm_gpio msm_i2c_gpios_hw[] = {
	{ GPIO_CFG(70, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio msm_i2c_gpios_io[] = {
	{ GPIO_CFG(70, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(16, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};
static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(16, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};

static void
msm_i2c_gpio_config(int adap_id, int config_type)
{
	struct msm_gpio *msm_i2c_table;

	/* Each adapter gets 2 lines from the table */
	if (adap_id > 0)
		return;
	if (config_type)
		msm_i2c_table = &msm_i2c_gpios_hw[adap_id*2];
	else
		msm_i2c_table = &msm_i2c_gpios_io[adap_id*2];
	msm_gpios_enable(msm_i2c_table, 2);
}
/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
static struct vreg *qup_vreg;
#endif
static void
qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc = 0;
	struct msm_gpio *qup_i2c_table;
	/* Each adapter gets 2 lines from the table */
	if (adap_id != 4)
		return;
	if (config_type)
		qup_i2c_table = qup_i2c_gpios_hw;
	else
		qup_i2c_table = qup_i2c_gpios_io;
	rc = msm_gpios_enable(qup_i2c_table, 2);
	if (rc < 0)
		printk(KERN_ERR "QUP GPIO enable failed: %d\n", rc);
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	if (qup_vreg) {
		int rc = vreg_set_level(qup_vreg, 1800);
		if (rc) {
			pr_err("%s: vreg LVS1 set level failed (%d)\n",
			__func__, rc);
		}
		rc = vreg_enable(qup_vreg);
		if (rc) {
			pr_err("%s: vreg_enable() = %d \n",
			__func__, rc);
		}
	}
#endif
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 100000,
	.pri_clk = 70,
	.pri_dat = 71,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000021",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
static void __init msm_device_i2c_power_domain(void)
{
    struct vreg *vreg_ldo12;
	struct vreg *vreg_ldo8;  ///i2c_gpio_power
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++[
//Div2D5-OwenHuang-I2C-Enable_Bus_VDDIO-00+{
#if defined(CONFIG_FIH_PROJECT_SF4Y6) || defined(CONFIG_FIH_PROJECT_SF4V5) //Div2D5-OwenHuang-SF5_ALSPS_Vreg_GP7_Setting-01* //Div2D5-OwenHuang-SF5_Reset_L8-00*
	struct vreg *vreg_gp7;
#endif
//Div2D5-OwenHuang-I2C-Enable_Bus_VDDIO-00+}
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++]
    int rc;
    /* 1.8V -- LDO12 */
    vreg_ldo12 = vreg_get(NULL, "gp9");

    if (IS_ERR(vreg_ldo12)) {
        pr_err("%s: gp9 vreg get failed (%ld)\n",
               __func__, PTR_ERR(vreg_ldo12));
        return;
    }
    
    rc = vreg_set_level(vreg_ldo12, 3000);
    if (rc) {
        pr_err("%s: vreg LDO12 set level failed (%d)\n",
               __func__, rc);
        return;
    }
            
    rc = vreg_enable(vreg_ldo12);
        if (rc) {
            pr_err("%s: LDO12 vreg enable failed (%d)\n",
                   __func__, rc);
            return;
        }
    /* VDDIO 1.8V -- LDO8*/  ///i2c_gpio_power
    vreg_ldo8 = vreg_get(NULL, "gp7");

    if (IS_ERR(vreg_ldo8)) {
        rc = PTR_ERR(vreg_ldo8);
        printk("%s: gp7 vreg get failed (%d)\n",
               __func__, rc);
        return;
    }

    rc = vreg_set_level(vreg_ldo8, 1800);
    if (rc) {
        printk("%s: vreg LDO8 set level failed (%d)\n",
               __func__, rc);
        return;
    }

    rc = vreg_enable(vreg_ldo8);      
    if (rc) {
            pr_err("%s: LDO8 vreg enable failed (%d)\n",
                   __func__, rc);
            return;
    }
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++[
//Div2D5-OwenHuang-I2C-Enable_Bus_VDDIO-00+{
#if defined(CONFIG_FIH_PROJECT_SF4Y6) || defined(CONFIG_FIH_PROJECT_SF4V5) //Div2D5-OwenHuang-SF5_ALSPS_Vreg_GP7_Setting-01* //Div2D5-OwenHuang-SF5_Reset_L8-00*
	vreg_gp7 = vreg_get(NULL, "gp7");

    if (IS_ERR(vreg_gp7)) {
        pr_err("%s: gp7 vreg get failed (%ld)\n",
               __func__, PTR_ERR(vreg_gp7));
        return;
    }
    
    rc = vreg_set_level(vreg_gp7, 1800);
    if (rc) {
        pr_err("%s: vreg gp7 set level failed (%d)\n",
               __func__, rc);
        return;
    }
            
	//Div2D5-OwenHuang-SF5_Reset_L8-00-{
    /*rc = vreg_enable(vreg_gp7);
    if (rc) {
            pr_err("%s: gp7 vreg enable failed (%d)\n",
                   __func__, rc);
            return;
        }*/ 
    //Div2D5-OwenHuang-SF5_Reset_L8-00-}

	vreg_pull_down_switch(vreg_gp7, 1); //Div2D5-OwenHuang-SF5_Reset_L8-00+


	//Div2D5-OwenHuang-SF6_AKM8975C-Framework_Porting-04+{
	//reset again to ensure light sensor can initialize successfully
	rc = vreg_disable(vreg_gp7);
    if (rc) 
	{
        pr_err("%s: gp7 vreg enable failed (%d)\n",
                   __func__, rc);
        return;
    }
		
	printk(KERN_INFO "%s, shutdown vreg_gp7\n", __func__);
		
	msleep(100);

	printk(KERN_INFO "%s, Power-on vreg_gp7\n", __func__);

    rc = vreg_enable(vreg_gp7);
    if (rc) 
	{
        pr_err("%s: gp7 vreg enable failed (%d)\n",
                   __func__, rc);
        return;
    } 
	//Div2D5-OwenHuang-SF6_AKM8975C-Framework_Porting-04+}

	vreg_gp7 = NULL;
#endif
//Div2D5-OwenHuang-I2C-Enable_Bus_VDDIO-00+}
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++]
}
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */

static void __init msm_device_i2c_init(void)
{
	if (msm_gpios_request(msm_i2c_gpios_hw, ARRAY_SIZE(msm_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static struct msm_i2c_platform_data msm_i2c_2_pdata = {
	.clk_freq = 100000,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000022",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_2_init(void)
{
	msm_device_i2c_2.dev.platform_data = &msm_i2c_2_pdata;
}

static struct msm_i2c_platform_data qup_i2c_pdata = {
	.clk_freq = 384000,
	.pclk = "camif_pad_pclk",
	.msm_i2c_config_gpio = qup_i2c_gpio_config,
};

static void __init qup_device_i2c_init(void)
{
	if (msm_gpios_request(qup_i2c_gpios_hw, ARRAY_SIZE(qup_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	qup_device_i2c.dev.platform_data = &qup_i2c_pdata;
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	qup_vreg = vreg_get(NULL, "lvsw1");
	if (IS_ERR(qup_vreg)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(qup_vreg));
	}
#endif
}

#ifdef CONFIG_I2C_SSBI
static struct msm_ssbi_platform_data msm_i2c_ssbi6_pdata = {
	.rsl_id = "D:PMIC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI2,
};

static struct msm_ssbi_platform_data msm_i2c_ssbi7_pdata = {
	.rsl_id = "D:CODEC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

static struct msm_acpu_clock_platform_data msm7x30_clock_data = {
	.acpu_switch_time_us = 50,
	.vdd_switch_time_us = 62,
};

static void __init msm7x30_init_irq(void)
{
	msm_init_irq();
}

static struct msm_gpio msm_nand_ebi2_cfg_data[] = {
	{GPIO_CFG(86, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "ebi2_cs1"},
	{GPIO_CFG(115, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "ebi2_busy1"},
};

struct vreg *vreg_s3;
struct vreg *vreg_mmc;

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};
#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT)
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+{
#ifndef CONFIG_FIH_PROJECT_SF4Y6
static struct msm_gpio sdc1_lvlshft_cfg_data[] = {
	{GPIO_CFG(35, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA), "sdc1_lvlshft"},
};
#endif
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+}
#endif
static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(38, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc1_clk"},
#ifdef CONFIG_FIH_PROJECT_SF4Y6
        {GPIO_CFG(39, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc1_cmd"},//Div5-Kernel-JC-GPIOUPDATE*
#else
        //Div2-SW6-MM-HL-Camera-Flash-02*{
#ifndef CONFIG_FIH_AAT1272
        {GPIO_CFG(39, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_cmd"},
#endif
        //Div2-SW6-MM-HL-Camera-Flash-02*}
#endif
//Div5-Kernel-JC-GPIOUPDATE*[
#ifdef CONFIG_FIH_PROJECT_SF4Y6
	{GPIO_CFG(40, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc1_dat_3"},
	{GPIO_CFG(41, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc1_dat_2"},
	{GPIO_CFG(42, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc1_dat_1"},
	{GPIO_CFG(43, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc1_dat_0"},
#else
	{GPIO_CFG(40, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_3"},
	{GPIO_CFG(41, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_2"},
	{GPIO_CFG(42, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_1"},
	{GPIO_CFG(43, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_0"},
#endif
//Div5-Kernel-JC-GPIOUPDATE*]
};
//Div5-Kernel-JC-GPIOUPDATE+[
#ifdef CONFIG_FIH_PROJECT_SF4Y6
static struct msm_gpio sdc1_sleep_cfg_data[] = {
	{GPIO_CFG(38, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc1_clk"},
	{GPIO_CFG(39, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc1_cmd"},
	{GPIO_CFG(40, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc1_dat_3"},
	{GPIO_CFG(41, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc1_dat_2"},
	{GPIO_CFG(42, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc1_dat_1"},
	{GPIO_CFG(43, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc1_dat_0"},
};
#endif
//Div5-Kernel-JC-GPIOUPDATE+]

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(64, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc2_clk"},
	{GPIO_CFG(65, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_cmd"},
	{GPIO_CFG(66, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_3"},
	{GPIO_CFG(67, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_2"},
	{GPIO_CFG(68, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_1"},
	{GPIO_CFG(69, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_0"},

#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	{GPIO_CFG(115, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_4"},
	{GPIO_CFG(114, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_5"},
	{GPIO_CFG(113, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_6"},
	{GPIO_CFG(112, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_7"},
#endif
};

static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(110, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc3_clk"},
//Div5-Kernel-JC-GPIOUPDATE*[
#ifdef CONFIG_FIH_PROJECT_SF4Y6
	{GPIO_CFG(111, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc3_cmd"},
	{GPIO_CFG(116, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc3_dat_3"},
	{GPIO_CFG(117, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc3_dat_2"},
	{GPIO_CFG(118, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc3_dat_1"},
	{GPIO_CFG(119, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc3_dat_0"},
#else
	{GPIO_CFG(111, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_cmd"},
	{GPIO_CFG(116, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_3"},
	{GPIO_CFG(117, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_2"},
	{GPIO_CFG(118, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_1"},
	{GPIO_CFG(119, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_0"},
#endif
//Div5-Kernel-JC-GPIOUPDATE*]
};

static struct msm_gpio sdc3_sleep_cfg_data[] = {
	{GPIO_CFG(110, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_clk"},
	{GPIO_CFG(111, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_cmd"},
	{GPIO_CFG(116, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_3"},
	{GPIO_CFG(117, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_2"},
	{GPIO_CFG(118, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_1"},
	{GPIO_CFG(119, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_0"},
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(58, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc4_clk"},
	{GPIO_CFG(59, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_cmd"},
	{GPIO_CFG(60, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc4_dat_0"},
};

//Div251-PK-SD_SLEEP_table-00+{
static struct msm_gpio sdc4_sleep_cfg_data[] = {
	{GPIO_CFG(58, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_clk"},
	{GPIO_CFG(59, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_cmd"},
	{GPIO_CFG(60, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_0"},
};
//Div251-PK-SD_SLEEP_table-00+}

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
//Div5-Kernel-JC-GPIOUPDATE*[
#ifdef CONFIG_FIH_PROJECT_SF4Y6		
		.sleep_cfg_data = sdc1_sleep_cfg_data,
#else
		.sleep_cfg_data = NULL,
#endif
//Div5-Kernel-JC-GPIOUPDATE*]
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
		.sleep_cfg_data = sdc3_sleep_cfg_data,
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
		.sleep_cfg_data = sdc4_sleep_cfg_data, //Div251-PK-SD_SLEEP_table-00+
	},
};

struct sdcc_vreg {
	struct vreg *vreg_data;
	unsigned level;
};

static struct sdcc_vreg sdcc_vreg_data[4];

static unsigned long vreg_sts, gpio_sts;

static uint32_t msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
		} else {
			msm_gpios_disable_free(curr->cfg_data, curr->size);
		}
	}

	return rc;
}

static uint32_t msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_vreg *curr;
	static int enabled_once[] = {0, 0, 0, 0};

	curr = &sdcc_vreg_data[dev_id - 1];

	if (!(test_bit(dev_id, &vreg_sts)^enable))
		return rc;

	if (!enable || enabled_once[dev_id - 1])
		return 0;

	if (enable) {
		set_bit(dev_id, &vreg_sts);
		rc = vreg_set_level(curr->vreg_data, curr->level);
		if (rc) {
			printk(KERN_ERR "%s: vreg_set_level() = %d \n",
					__func__, rc);
		}
		rc = vreg_enable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		}
		enabled_once[dev_id - 1] = 1;
	} else {
		clear_bit(dev_id, &vreg_sts);
		rc = vreg_disable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		}
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	rc = msm_sdcc_setup_gpio(pdev->id, (vdd ? 1 : 0));
	if (rc)
		goto out;

	if (pdev->id == 4) /* S3 is always ON and cannot be disabled */
	{
        rc = msm_sdcc_setup_vreg(pdev->id, (vdd ? 1 : 0));

		if(vdd)
			gpio_set_value(sd_enable_pin, 1);
		else
			gpio_set_value(sd_enable_pin, 0);			
			
    }
out:
	return rc;
}

#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT) && \
	defined(CONFIG_CSDIO_VENDOR_ID) && \
	defined(CONFIG_CSDIO_DEVICE_ID) && \
	(CONFIG_CSDIO_VENDOR_ID == 0x70 && CONFIG_CSDIO_DEVICE_ID == 0x1117)

#define MBP_ON  1
#define MBP_OFF 0

#define MBP_RESET_N \
	GPIO_CFG(44, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA)
#define MBP_INT0 \
	GPIO_CFG(46, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA)

#define MBP_MODE_CTRL_0 \
	GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
#define MBP_MODE_CTRL_1 \
	GPIO_CFG(36, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
#define MBP_MODE_CTRL_2 \
	GPIO_CFG(34, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
#define TSIF_EN \
	GPIO_CFG(35, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,	GPIO_CFG_2MA)
#define TSIF_DATA \
	GPIO_CFG(36, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,	GPIO_CFG_2MA)
#define TSIF_CLK \
	GPIO_CFG(34, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)

static struct msm_gpio mbp_cfg_data[] = {
	{GPIO_CFG(44, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"mbp_reset"},
	{GPIO_CFG(85, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"mbp_io_voltage"},
};

static int mbp_config_gpios_pre_init(int enable)
{
	int rc = 0;

	if (enable) {
		rc = msm_gpios_request_enable(mbp_cfg_data,
			ARRAY_SIZE(mbp_cfg_data));
		if (rc) {
			printk(KERN_ERR
				"%s: Failed to turnon GPIOs for mbp chip(%d)\n",
				__func__, rc);
		}
	} else
		msm_gpios_disable_free(mbp_cfg_data, ARRAY_SIZE(mbp_cfg_data));
	return rc;
}

static int mbp_setup_rf_vregs(int state)
{
	struct vreg *vreg_rf = NULL;
	struct vreg *vreg_rf_switch	= NULL;
	int rc;

	vreg_rf = vreg_get(NULL, "s2");
	if (IS_ERR(vreg_rf)) {
		pr_err("%s: s2 vreg get failed (%ld)",
				__func__, PTR_ERR(vreg_rf));
		return -EFAULT;
	}
	vreg_rf_switch = vreg_get(NULL, "rf");
	if (IS_ERR(vreg_rf_switch)) {
		pr_err("%s: rf vreg get failed (%ld)",
				__func__, PTR_ERR(vreg_rf_switch));
		return -EFAULT;
	}

	if (state) {
		rc = vreg_set_level(vreg_rf, 1300);
		if (rc) {
			pr_err("%s: vreg s2 set level failed (%d)\n",
					__func__, rc);
			return rc;
		}

		rc = vreg_enable(vreg_rf);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable(s2) = %d\n",
					__func__, rc);
		}

		rc = vreg_set_level(vreg_rf_switch, 2600);
		if (rc) {
			pr_err("%s: vreg rf switch set level failed (%d)\n",
					__func__, rc);
			return rc;
		}
		rc = vreg_enable(vreg_rf_switch);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable(rf) = %d\n",
					__func__, rc);
		}
	} else {
		(void) vreg_disable(vreg_rf);
		(void) vreg_disable(vreg_rf_switch);
	}
	return 0;
}

static int mbp_setup_vregs(int state)
{
	struct vreg *vreg_analog = NULL;
	struct vreg *vreg_io = NULL;
	int rc;

	vreg_analog = vreg_get(NULL, "gp4");
	if (IS_ERR(vreg_analog)) {
		pr_err("%s: gp4 vreg get failed (%ld)",
				__func__, PTR_ERR(vreg_analog));
		return -EFAULT;
	}
	vreg_io = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_io)) {
		pr_err("%s: s3 vreg get failed (%ld)",
				__func__, PTR_ERR(vreg_io));
		return -EFAULT;
	}
	if (state) {
		rc = vreg_set_level(vreg_analog, 2600);
		if (rc) {
			pr_err("%s: vreg_set_level failed (%d)",
					__func__, rc);
		}
		rc = vreg_enable(vreg_analog);
		if (rc) {
			pr_err("%s: analog vreg enable failed (%d)",
					__func__, rc);
		}
		rc = vreg_set_level(vreg_io, 1800);
		if (rc) {
			pr_err("%s: vreg_set_level failed (%d)",
					__func__, rc);
		}
		rc = vreg_enable(vreg_io);
		if (rc) {
			pr_err("%s: io vreg enable failed (%d)",
					__func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_analog);
		if (rc) {
			pr_err("%s: analog vreg disable failed (%d)",
					__func__, rc);
		}
		rc = vreg_disable(vreg_io);
		if (rc) {
			pr_err("%s: io vreg disable failed (%d)",
					__func__, rc);
		}
	}
	return rc;
}

static int mbp_set_tcxo_en(int enable)
{
	int rc;
	const char *id = "UBMC";
	struct vreg *vreg_analog = NULL;

	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A1,
		enable ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
	if (rc < 0) {
		printk(KERN_ERR "%s: unable to %svote for a1 clk\n",
			__func__, enable ? "" : "de-");
		return -EIO;
	}
	if (!enable) {
		vreg_analog = vreg_get(NULL, "gp4");
		if (IS_ERR(vreg_analog)) {
			pr_err("%s: gp4 vreg get failed (%ld)",
					__func__, PTR_ERR(vreg_analog));
			return -EFAULT;
		}

		(void) vreg_disable(vreg_analog);
	}
	return rc;
}

static void mbp_set_freeze_io(int state)
{
	if (state)
		gpio_set_value(85, 0);
	else
		gpio_set_value(85, 1);
}

static int mbp_set_core_voltage_en(int enable)
{
	int rc;
	struct vreg *vreg_core1p2 = NULL;

	vreg_core1p2 = vreg_get(NULL, "gp16");
	if (IS_ERR(vreg_core1p2)) {
		pr_err("%s: gp16 vreg get failed (%ld)",
				__func__, PTR_ERR(vreg_core1p2));
		return -EFAULT;
	}
	if (enable) {
		rc = vreg_set_level(vreg_core1p2, 1200);
		if (rc) {
			pr_err("%s: vreg_set_level failed (%d)",
					__func__, rc);
		}
		(void) vreg_enable(vreg_core1p2);

		return 80;
	} else {
		gpio_set_value(85, 1);
		return 0;
	}
	return rc;
}

static void mbp_set_reset(int state)
{
	if (state)
		gpio_set_value(GPIO_PIN(MBP_RESET_N), 0);
	else
		gpio_set_value(GPIO_PIN(MBP_RESET_N), 1);
}

static int mbp_config_interface_mode(int state)
{
	if (state) {
		gpio_tlmm_config(MBP_MODE_CTRL_0, GPIO_CFG_ENABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_1, GPIO_CFG_ENABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_2, GPIO_CFG_ENABLE);
		gpio_set_value(GPIO_PIN(MBP_MODE_CTRL_0), 0);
		gpio_set_value(GPIO_PIN(MBP_MODE_CTRL_1), 1);
		gpio_set_value(GPIO_PIN(MBP_MODE_CTRL_2), 0);
	} else {
		gpio_tlmm_config(MBP_MODE_CTRL_0, GPIO_CFG_DISABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_1, GPIO_CFG_DISABLE);
		gpio_tlmm_config(MBP_MODE_CTRL_2, GPIO_CFG_DISABLE);
	}
	return 0;
}

static int mbp_setup_adc_vregs(int state)
{
	struct vreg *vreg_adc = NULL;
	int rc;

	vreg_adc = vreg_get(NULL, "s4");
	if (IS_ERR(vreg_adc)) {
		pr_err("%s: s4 vreg get failed (%ld)",
				__func__, PTR_ERR(vreg_adc));
		return -EFAULT;
	}
	if (state) {
		rc = vreg_set_level(vreg_adc, 2200);
		if (rc) {
			pr_err("%s: vreg_set_level failed (%d)",
					__func__, rc);
		}
		rc = vreg_enable(vreg_adc);
		if (rc) {
			pr_err("%s: enable vreg adc failed (%d)",
					__func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_adc);
		if (rc) {
			pr_err("%s: disable vreg adc failed (%d)",
					__func__, rc);
		}
	}
	return rc;
}

static int mbp_power_up(void)
{
	int rc;

	rc = mbp_config_gpios_pre_init(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_config_gpios_pre_init() done\n", __func__);

	rc = mbp_setup_vregs(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: gp4 (2.6) and s3 (1.8) done\n", __func__);

	rc = mbp_set_tcxo_en(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: tcxo clock done\n", __func__);

	mbp_set_freeze_io(MBP_OFF);
	pr_debug("%s: set gpio 85 to 1 done\n", __func__);

	udelay(100);
	mbp_set_reset(MBP_ON);

	udelay(300);
	rc = mbp_config_interface_mode(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_config_interface_mode() done\n", __func__);

	udelay(100 + mbp_set_core_voltage_en(MBP_ON));
	pr_debug("%s: power gp16 1.2V done\n", __func__);

	mbp_set_freeze_io(MBP_ON);
	pr_debug("%s: set gpio 85 to 0 done\n", __func__);

	udelay(100);

	rc = mbp_setup_rf_vregs(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: s2 1.3V and rf 2.6V done\n", __func__);

	rc = mbp_setup_adc_vregs(MBP_ON);
	if (rc)
		goto exit;
	pr_debug("%s: s4 2.2V  done\n", __func__);

	udelay(200);

	mbp_set_reset(MBP_OFF);
	pr_debug("%s: close gpio 44 done\n", __func__);

	msleep(20);
exit:
	return rc;
}

static int mbp_power_down(void)
{
	int rc;
	struct vreg *vreg_adc = NULL;

	vreg_adc = vreg_get(NULL, "s4");
	if (IS_ERR(vreg_adc)) {
		pr_err("%s: s4 vreg get failed (%ld)",
				__func__, PTR_ERR(vreg_adc));
		return -EFAULT;
	}

	mbp_set_reset(MBP_ON);
	pr_debug("%s: mbp_set_reset(MBP_ON) done\n", __func__);

	udelay(100);

	rc = mbp_setup_adc_vregs(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: vreg_disable(vreg_adc) done\n", __func__);

	udelay(5);

	rc = mbp_setup_rf_vregs(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_setup_rf_vregs(MBP_OFF) done\n", __func__);

	udelay(5);

	mbp_set_freeze_io(MBP_OFF);
	pr_debug("%s: mbp_set_freeze_io(MBP_OFF) done\n", __func__);

	udelay(100);
	rc = mbp_set_core_voltage_en(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_set_core_voltage_en(MBP_OFF) done\n", __func__);

	gpio_set_value(85, 1);

	rc = mbp_set_tcxo_en(MBP_OFF);
	if (rc)
		goto exit;
	pr_debug("%s: mbp_set_tcxo_en(MBP_OFF) done\n", __func__);

	rc = mbp_config_gpios_pre_init(MBP_OFF);
	if (rc)
		goto exit;
exit:
	return rc;
}

static void (*mbp_status_notify_cb)(int card_present, void *dev_id);
static void *mbp_status_notify_cb_devid;
static int mbp_power_status;
static int mbp_power_init_done;

static uint32_t mbp_setup_power(struct device *dv,
	unsigned int power_status)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);

	if (power_status == mbp_power_status)
		goto exit;
	if (power_status) {
		pr_debug("turn on power of mbp slot");
		rc = mbp_power_up();
		mbp_power_status = 1;
	} else {
		pr_debug("turn off power of mbp slot");
		rc = mbp_power_down();
		mbp_power_status = 0;
	}
exit:
	return rc;
};

int mbp_register_status_notify(void (*callback)(int, void *),
	void *dev_id)
{
	mbp_status_notify_cb = callback;
	mbp_status_notify_cb_devid = dev_id;
	return 0;
}

static unsigned int mbp_status(struct device *dev)
{
	return mbp_power_status;
}

static uint32_t msm_sdcc_setup_power_mbp(struct device *dv, unsigned int vdd)
{
	struct platform_device *pdev;
	uint32_t rc = 0;

	pdev = container_of(dv, struct platform_device, dev);
	rc = msm_sdcc_setup_power(dv, vdd);
	if (rc) {
		pr_err("%s: Failed to setup power (%d)\n",
			__func__, rc);
		goto out;
	}
	if (!mbp_power_init_done) {
		mbp_setup_power(dv, 1);
		mbp_setup_power(dv, 0);
		mbp_power_init_done = 1;
	}
	if (vdd >= 0x8000) {
		rc = mbp_setup_power(dv, (0x8000 == vdd) ? 0 : 1);
		if (rc) {
			pr_err("%s: Failed to config mbp chip power (%d)\n",
				__func__, rc);
			goto out;
		}
		if (mbp_status_notify_cb) {
			mbp_status_notify_cb(mbp_power_status,
				mbp_status_notify_cb_devid);
		}
	}
out:
	/* should return 0 only */
	return 0;
}

#endif

#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int msm7x30_sdcc_slot_status(struct device *dev)
{
	return (unsigned int)!gpio_get_value(sd_detect_pin);
}
#endif

static int msm_sdcc_get_wpswitch(struct device *dv)
{
	void __iomem *wp_addr = 0;
	uint32_t ret = 0;
	struct platform_device *pdev;

	if (!(machine_is_msm7x30_surf()))
		return -1;
	pdev = container_of(dv, struct platform_device, dev);

	wp_addr = ioremap(FPGA_SDCC_STATUS, 4);
	if (!wp_addr) {
		pr_err("%s: Could not remap %x\n", __func__, FPGA_SDCC_STATUS);
		return -ENOMEM;
	}

	ret = (((readl(wp_addr) >> 4) >> (pdev->id-1)) & 0x01);
	pr_info("%s: WP Status for Slot %d = 0x%x \n", __func__,
							pdev->id, ret);
	iounmap(wp_addr);

	return ret;
}
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+{
#ifdef CONFIG_FIH_PROJECT_SF4Y6
static struct mmc_platform_data msm7x30_sdc1_data = {
    .ocr_mask   = MMC_VDD_27_28 | MMC_VDD_28_29,
    .translate_vdd  = msm_sdcc_setup_power,
    .mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
    //.sdiowakeup_irq = MSM_GPIO_TO_INT(42),    //Div2-SW6-Conn-JC-WiFi-SDIOINT-00+
#endif
#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
    .dummy52_required = 1,
#endif
    .msmsdcc_fmin   = 144000,
    .msmsdcc_fmid   = 24576000,
    .msmsdcc_fmax   = 49152000,
    .nonremovable   = 1,
};    
#else /*CONFIG_FIH_PROJECT_SF4Y6*/
#if (CONFIG_CSDIO_VENDOR_ID == 0x70 && CONFIG_CSDIO_DEVICE_ID == 0x1117)
static struct mmc_platform_data msm7x30_sdc1_data = {
	.ocr_mask	= MMC_VDD_165_195 | MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power_mbp,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.status	        = mbp_status,
	.register_status_notify = mbp_register_status_notify,
#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 24576000,
	.nonremovable	= 0,
};
#else
static struct mmc_platform_data msm7x30_sdc1_data = {
	.ocr_mask	= MMC_VDD_165_195,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif
#endif
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+}
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm7x30_sdc2_data = {
	.ocr_mask	= MMC_VDD_165_195 | MMC_VDD_27_28,
	.translate_vdd	= msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm7x30_sdc3_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
	//.sdiowakeup_irq = MSM_GPIO_TO_INT(118),	//Div2-SW6-Conn-JC-WiFi-DisableWiFiINT-00+
#endif
#ifdef CONFIG_MMC_MSM_SDC3_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct mmc_platform_data msm7x30_sdc4_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm7x30_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, PMIC_GPIO_SD_DET - 1),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.wpswitch    = msm_sdcc_get_wpswitch,
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+{
#ifndef CONFIG_FIH_PROJECT_SF4Y6
static void msm_sdc1_lvlshft_enable(void)
{
	int rc;

	/* Enable LDO5, an input to the FET that powers slot 1 */
	rc = vreg_set_level(vreg_mmc, 2850);
	if (rc)
		printk(KERN_ERR "%s: vreg_set_level() = %d \n",	__func__, rc);

	rc = vreg_enable(vreg_mmc);
	if (rc)
		printk(KERN_ERR "%s: vreg_enable() = %d \n", __func__, rc);

	/* Enable GPIO 35, to turn on the FET that powers slot 1 */
	rc = msm_gpios_request_enable(sdc1_lvlshft_cfg_data,
				ARRAY_SIZE(sdc1_lvlshft_cfg_data));
	if (rc)
		printk(KERN_ERR "%s: Failed to enable GPIO 35\n", __func__);

	rc = gpio_direction_output(GPIO_PIN(sdc1_lvlshft_cfg_data[0].gpio_cfg),
				1);
	if (rc)
		printk(KERN_ERR "%s: Failed to turn on GPIO 35\n", __func__);
}
#endif
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+}
#endif

//Div2-5-3-Peripheral-LL-UsbPorting-00+{
#define NV_PRD_ID_I 50001
//Div2-5-3-Peripheral-LL-UsbCustomized-01*{
#define NV_FIH_USB_DEVICE_CUSTOMER_I 50034
int fih_usb_full_func = 0xC000;
struct usb_device_custom_nv {
    uint32_t magic_num;
    uint16_t vendor_id;
    uint16_t product_id;
    char product_name[32];
    char manufacturer_name[32];
    char reserved[52];
};
//Div2-5-3-Peripheral-LL-UsbCustomized-01*}
static int sync_from_custom_nv(void);
static int sync_from_custom_nv()
{
    uint32_t smem_proc_comm_oem_cmd1 = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1 = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2 = NV_PRD_ID_I;
    uint32_t product_id[32];
    char serial_number[32];
    struct usb_device_custom_nv usb_product;//Div2-5-3-Peripheral-LL-UsbCustomized-00+
    struct smem_host_oem_info *usb_type_info = NULL;
    unsigned int info_size;
    int len = 0;
    int i;
    char *src;
    char *k_serial;

    usb_type_info = smem_get_entry(SMEM_ID_VENDOR2, &info_size);
    if(usb_type_info) {
        android_usb_pdata.product_id = usb_type_info->host_usb_id;
    }
    if(msm_proc_comm_oem(smem_proc_comm_oem_cmd1, &smem_proc_comm_oem_data1, product_id, &smem_proc_comm_oem_data2) == 0) {
        memcpy(serial_number, product_id, sizeof(serial_number));
        len = strlen(serial_number);
        if(len > 0) {
            //strlcpy(serial_number, (char*)product_id, sizeof(serial_number));
            printk(KERN_INFO"%s: read serial number (%s)\n",__func__, serial_number);
            src = serial_number;
            //eliminate space char from string start index
            while(isspace(*src)) {
                src++;
            }
            k_serial = (char*)kzalloc(16, GFP_KERNEL);
            strlcpy(k_serial, src, 16+1);
            src = k_serial;
            //end of non alpha and number character
            while(isalnum(*src)) {
                src++;
            }
            *src = '\0';
            if(strlen(k_serial)){
                android_usb_pdata.serial_number = k_serial;
            }
        }
    }

    src = android_usb_pdata.serial_number;

    rndis_pdata.ethaddr[0] = 0x02;
    for (i = 0; *src; i++) {
        /* XOR the USB serial across the remaining bytes */
        rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
    }
    //Div2-5-3-Peripheral-LL-UsbCustomized-01*{
    smem_proc_comm_oem_data2 = NV_FIH_USB_DEVICE_CUSTOMER_I;
    if(msm_proc_comm_oem(smem_proc_comm_oem_cmd1, &smem_proc_comm_oem_data1, product_id, &smem_proc_comm_oem_data2) == 0) {
        memcpy(&usb_product, product_id, sizeof(usb_product));
        printk(KERN_INFO"%s: USB MAGIC NUMBER (%d)", __func__, usb_product.magic_num);
        if(usb_product.magic_num == 0x12345678) {//magic number must be 0x12345678 could be effective
            printk(KERN_INFO"%s: USB CUSTOMIZED VID(0X%X) PID(0X%X) PRODUCT(%s) MANUFACTURER(%s)\n",
                __func__, usb_product.vendor_id, usb_product.product_id, usb_product.product_name, usb_product.manufacturer_name);
            if(usb_product.vendor_id && usb_product.product_id) {
                android_usb_pdata.vendor_id = usb_product.vendor_id;
                if(android_usb_pdata.vendor_id == 0x12d1) {//huawei
                    fih_usb_full_func = 0x1021;//open all usb functions
                    android_usb_pdata.num_products = ARRAY_SIZE(usb_products_12d1);
                    android_usb_pdata.products = usb_products_12d1;
                    if(android_usb_pdata.product_id == 0xc002) {//recovery mode
                        android_usb_pdata.product_id = 0x1022;
                    } else if(android_usb_pdata.product_id == 0xc000) {//diag debug mode enable
                        android_usb_pdata.product_id = 0x1021;
                    } else {
                        android_usb_pdata.product_id = usb_product.product_id;
                    }
                }
            }
            if(strlen(usb_product.product_name)) {
                k_serial = (char*)kzalloc(32, GFP_KERNEL);
                strlcpy(k_serial, usb_product.product_name, sizeof(usb_product.product_name));
                src = k_serial;
                while(isalnum(*src)||isspace(*src)) {
                    src++;
                }
                *src = '\0';
                if(strlen(k_serial)) {
                    android_usb_pdata.product_name = k_serial;
                }
            }
            if(strlen(usb_product.manufacturer_name)) {
                k_serial = (char*)kzalloc(32, GFP_KERNEL);
                strlcpy(k_serial, usb_product.manufacturer_name, sizeof(usb_product.manufacturer_name));
                src = k_serial;
                while(isalnum(*src)||isspace(*src)) {
                    src++;
                }
                *src = '\0';
                if(strlen(k_serial)) {
                    android_usb_pdata.manufacturer_name = k_serial;
                    rndis_pdata.vendorDescr = k_serial;
                    mass_storage_pdata.vendor = k_serial;
                }
            }
        }
    }
#ifdef CONFIG_FIH_FTM
    if(android_usb_pdata.vendor_id == 0x12d1) {//Huawei
        android_usb_pdata.product_id = 0x1023;
    } else if(android_usb_pdata.vendor_id == 0x489) {//FIH
        android_usb_pdata.product_id = 0xc003;
    }
    android_usb_pdata.serial_number = 0; //set serial number NULL
#endif
    printk(KERN_INFO"%s: USB VID(0X%X) PID(0X%X) PRODUCT(%s) MANUFACTURER(%s) SN(%s)\n", 
        __func__, android_usb_pdata.vendor_id, android_usb_pdata.product_id, android_usb_pdata.product_name, android_usb_pdata.manufacturer_name, android_usb_pdata.serial_number);
    //Div2-5-3-Peripheral-LL-UsbCustomized-01*}
    return 1;
}
//Div2-5-3-Peripheral-LL-UsbPorting-00+}

static void __init msm7x30_init_mmc(void)
{
	vreg_s3 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_s3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_s3));
		return;
	}

	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_mmc));
		return;
	}
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
#ifndef CONFIG_FIH_PROJECT_SF4Y6
	if (machine_is_msm7x30_fluid()) {
		msm7x30_sdc1_data.ocr_mask =  MMC_VDD_27_28 | MMC_VDD_28_29;
		msm_sdc1_lvlshft_enable();
	}
#endif
	sdcc_vreg_data[0].vreg_data = vreg_s3;
	sdcc_vreg_data[0].level = 1800;
	msm_add_sdcc(1, &msm7x30_sdc1_data);
#endif
//Div2-SW6-Conn-JC-WiFi_Slot1_SF6+}
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (machine_is_msm8x55_svlte_surf())
		msm7x30_sdc2_data.msmsdcc_fmax =  24576000;
	sdcc_vreg_data[1].vreg_data = vreg_s3;
	sdcc_vreg_data[1].level = 1800;
	msm_add_sdcc(2, &msm7x30_sdc2_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	sdcc_vreg_data[2].vreg_data = vreg_s3;
	sdcc_vreg_data[2].level = 1800;
	msm_sdcc_setup_gpio(3, 1);
	msm_add_sdcc(3, &msm7x30_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
    sdcc_vreg_data[3].vreg_data = vreg_mmc;
    sdcc_vreg_data[3].level = 2850;
//Div2-SW2-BSP-SD, HW detction GPIO PIN, Chihchia 2010.5.10 +
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	switch(fih_get_product_id())
	{
		case Product_FB0:
		{
			if(fih_get_product_phase() ==Product_PR1)
				sd_detect_pin = 38;
			else
				sd_detect_pin = 142;
		}
		break;
		case Product_FB1:
		case Product_FB3:
		case Product_FD1:
		{
			sd_detect_pin = 142;
		}
		break;
		case Product_SF6:
		{
			if(fih_get_product_phase() ==Product_PR3)
				sd_detect_pin = 143;
			else
				sd_detect_pin = 142;
		}
		break;
		case Product_SFH:
		{
			sd_detect_pin = 142;
		}
		break;
		case Product_SF8:
		case Product_SH8:
		case Product_SFC:
		{
			sd_detect_pin = 0;
		}
		break;
		default:
			sd_detect_pin = 142;
		break;
	}

	if(sd_detect_pin)
	{
		gpio_tlmm_config(GPIO_CFG(sd_detect_pin, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		msm7x30_sdc4_data.status_irq  = MSM_GPIO_TO_INT(sd_detect_pin);	
	}
	else
	{
		msm7x30_sdc4_data.status = NULL;
		msm7x30_sdc4_data.status_irq  = 0;	
	}
#endif  
//Div2-SW2-BSP-SD, Chihchia 2010.5.10 -

// +++ Enable pin, Chihchia 2010.8.10
	switch(fih_get_product_id())
	{
		case Product_FB0:
		case Product_FB1:
		case Product_FB3:
		case Product_SF6:
		case Product_FD1:
		{
			sd_enable_pin = 85;
		}
		break;
		case Product_SF5:
		{
			sd_enable_pin = 100;
		}
		break;
		case Product_SF8:
		case Product_SFH:
		case Product_SH8:
		case Product_SFC:
		{
			sd_enable_pin = 175;
		}
		break;				
		default:
			sd_enable_pin = 85;
		break;
	}

    gpio_tlmm_config(GPIO_CFG(sd_enable_pin, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
        GPIO_CFG_ENABLE);		
	
	printk("[SD] id:%d, phase:%d, sd_enable_pin:%d, sd_detect_pin:%d\n", fih_get_product_id(), fih_get_product_phase(), sd_enable_pin, sd_detect_pin);
	    
    msm_add_sdcc(4, &msm7x30_sdc4_data);
#endif

}

static void __init msm7x30_init_nand(void)
{
	char *build_id;
	struct flash_platform_data *plat_data;

	build_id = socinfo_get_build_id();
	if (build_id == NULL) {
		pr_err("%s: Build ID not available from socinfo\n", __func__);
		return;
	}

	if (build_id[8] == 'C' &&
			!msm_gpios_request_enable(msm_nand_ebi2_cfg_data,
			ARRAY_SIZE(msm_nand_ebi2_cfg_data))) {
		plat_data = msm_device_nand.dev.platform_data;
		plat_data->interleave = 1;
		printk(KERN_INFO "%s: Interleave mode Build ID found\n",
			__func__);
	}
}

#ifdef CONFIG_SERIAL_MSM_CONSOLE
static struct msm_gpio uart2_config_data[] = {
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++[
//Div2D5-OwenHuang-ALS-INT_Pin_Had_Been_Requested-00+{
#ifndef CONFIG_FIH_PROJECT_SF4Y6
	{ GPIO_CFG(49, 2, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_RFR"},
#else
	//do not configure GPIO_49, it will be used for ALS interrupt pin for SF6
#endif
//DIV5-BSP-CH-SF6-SENSOR-PORTING00++]
#ifndef CONFIG_FIH_PROJECT_FB400
	{ GPIO_CFG(50, 2, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_CTS"},
#endif
	{ GPIO_CFG(51, 2, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_Rx"},
	{ GPIO_CFG(52, 2, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_Tx"},
};

static void msm7x30_init_uart2(void)
{
	msm_gpios_request_enable(uart2_config_data,
			ARRAY_SIZE(uart2_config_data));

}
#endif

/* TSIF begin */
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)

#define TSIF_B_SYNC      GPIO_CFG(37, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define TSIF_B_DATA      GPIO_CFG(36, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define TSIF_B_EN        GPIO_CFG(35, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define TSIF_B_CLK       GPIO_CFG(34, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)

static const struct msm_gpio tsif_gpios[] = {
	{ .gpio_cfg = TSIF_B_CLK,  .label =  "tsif_clk", },
	{ .gpio_cfg = TSIF_B_EN,   .label =  "tsif_en", },
	{ .gpio_cfg = TSIF_B_DATA, .label =  "tsif_data", },
	{ .gpio_cfg = TSIF_B_SYNC, .label =  "tsif_sync", },
};

static struct msm_tsif_platform_data tsif_platform_data = {
	.num_gpios = ARRAY_SIZE(tsif_gpios),
	.gpios = tsif_gpios,
	.tsif_pclk = "tsif_pclk",
	.tsif_ref_clk = "tsif_ref_clk",
};
#endif /* defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE) */
/* TSIF end   */

/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_PMIC8058
static void __init pmic8058_leds_init(void)
{
	if (machine_is_msm7x30_surf()) {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].platform_data
			= &pm8058_surf_leds_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].data_size
			= sizeof(pm8058_surf_leds_data);
	} else if (!machine_is_msm7x30_fluid()) {
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].platform_data
			= &pm8058_ffa_leds_data;
		pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].data_size
			= sizeof(pm8058_ffa_leds_data);
	}
}
#endif
/* } Div2-SW2-BSP-FBX-LEDS */

static struct msm_spm_platform_data msm_spm_data __initdata = {
	.reg_base_addr = MSM_SAW_BASE,

	.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x05,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x18,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x00006666,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFF000666,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x03,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

	.awake_vlevel = 0xF2,
	.retention_vlevel = 0xE0,
	.collapse_vlevel = 0x72,
	.retention_mid_vlevel = 0xE0,
	.collapse_mid_vlevel = 0xE0,

	.vctl_timeout_us = 50,
};

//Div2-SW2-BSP,JOE HSU,+++
//#ifdef CONFIG_FIH_CONFIG_GROUP
extern void fxx_info_init(void);
//#endif
//Div2-SW2-BSP,JOE HSU,---

#if defined(CONFIG_TOUCHSCREEN_TSC2007) || \
	defined(CONFIG_TOUCHSCREEN_TSC2007_MODULE)

#define TSC2007_TS_PEN_INT	20

static struct msm_gpio tsc2007_config_data[] = {
	{ GPIO_CFG(TSC2007_TS_PEN_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	"tsc2007_irq" },
};

static struct vreg *vreg_tsc_s3;
static struct vreg *vreg_tsc_s2;

static int tsc2007_init(void)
{
	int rc;

	vreg_tsc_s3 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_tsc_s3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_tsc_s3));
		return -ENODEV;
	}

	rc = vreg_set_level(vreg_tsc_s3, 1800);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_set_level;
	}

	rc = vreg_enable(vreg_tsc_s3);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_set_level;
	}

	vreg_tsc_s2 = vreg_get(NULL, "s2");
	if (IS_ERR(vreg_tsc_s2)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_tsc_s2));
		goto fail_vreg_get;
	}

	rc = vreg_set_level(vreg_tsc_s2, 1300);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_s2_level;
	}

	rc = vreg_enable(vreg_tsc_s2);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_s2_level;
	}

	rc = msm_gpios_request_enable(tsc2007_config_data,
			ARRAY_SIZE(tsc2007_config_data));
	if (rc) {
		pr_err("%s: Unable to request gpios\n", __func__);
		goto fail_gpio_req;
	}

	return 0;

fail_gpio_req:
	vreg_disable(vreg_tsc_s2);
fail_vreg_s2_level:
	vreg_put(vreg_tsc_s2);
fail_vreg_get:
	vreg_disable(vreg_tsc_s3);
fail_vreg_set_level:
	vreg_put(vreg_tsc_s3);
	return rc;
}

static int tsc2007_get_pendown_state(void)
{
	int rc;

	rc = gpio_get_value(TSC2007_TS_PEN_INT);
	if (rc < 0) {
		pr_err("%s: MSM GPIO %d read failed\n", __func__,
						TSC2007_TS_PEN_INT);
		return rc;
	}

	return (rc == 0 ? 1 : 0);
}

static void tsc2007_exit(void)
{
	vreg_disable(vreg_tsc_s3);
	vreg_put(vreg_tsc_s3);
	vreg_disable(vreg_tsc_s2);
	vreg_put(vreg_tsc_s2);

	msm_gpios_disable_free(tsc2007_config_data,
		ARRAY_SIZE(tsc2007_config_data));
}

static int tsc2007_power_shutdown(bool enable)
{
	int rc;

	if (enable == false) {
		rc = vreg_enable(vreg_tsc_s2);
		if (rc) {
			pr_err("%s: vreg_enable failed\n", __func__);
			return rc;
		}
		rc = vreg_enable(vreg_tsc_s3);
		if (rc) {
			pr_err("%s: vreg_enable failed\n", __func__);
			vreg_disable(vreg_tsc_s2);
			return rc;
		}
		/* Voltage settling delay */
		msleep(20);
	} else {
		rc = vreg_disable(vreg_tsc_s2);
		if (rc) {
			pr_err("%s: vreg_disable failed\n", __func__);
			return rc;
		}
		rc = vreg_disable(vreg_tsc_s3);
		if (rc) {
			pr_err("%s: vreg_disable failed\n", __func__);
			vreg_enable(vreg_tsc_s2);
			return rc;
		}
	}

	return rc;
}

static struct tsc2007_platform_data tsc2007_ts_data = {
	.model = 2007,
	.x_plate_ohms = 300,
	.irq_flags    = IRQF_TRIGGER_LOW,
	.init_platform_hw = tsc2007_init,
	.exit_platform_hw = tsc2007_exit,
	.power_shutdown	  = tsc2007_power_shutdown,
	.invert_x	  = true,
	.invert_y	  = true,
	/* REVISIT: Temporary fix for reversed pressure */
	.invert_z1	  = true,
	.invert_z2	  = true,
	.get_pendown_state = tsc2007_get_pendown_state,
};

static struct i2c_board_info tsc_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("tsc2007", 0x48),
		.irq		= MSM_GPIO_TO_INT(TSC2007_TS_PEN_INT),
		.platform_data = &tsc2007_ts_data,
	},
};
#endif

static const char *vregs_isa1200_name[] = {
	"gp7",
	"gp10",
};

static const int vregs_isa1200_val[] = {
	1800,
	2600,
};
static struct vreg *vregs_isa1200[ARRAY_SIZE(vregs_isa1200_name)];

static int isa1200_power(int vreg_on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++) {
		if (!vregs_isa1200[i]) {
			pr_err("%s: vreg_get %s failed (%d)\n",
				__func__, vregs_isa1200_name[i], rc);
			goto vreg_fail;
		}

		rc = vreg_on ? vreg_enable(vregs_isa1200[i]) :
			  vreg_disable(vregs_isa1200[i]);
		if (rc < 0) {
			pr_err("%s: vreg %s %s failed (%d)\n",
				__func__, vregs_isa1200_name[i],
			       vreg_on ? "enable" : "disable", rc);
			goto vreg_fail;
		}
	}
	return 0;

vreg_fail:
	while (i)
		vreg_disable(vregs_isa1200[--i]);
	return rc;
}

static int isa1200_dev_setup(bool enable)
{
	int i, rc;

	if (enable == true) {
		for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++) {
			vregs_isa1200[i] = vreg_get(NULL,
						vregs_isa1200_name[i]);
			if (IS_ERR(vregs_isa1200[i])) {
				pr_err("%s: vreg get %s failed (%ld)\n",
					__func__, vregs_isa1200_name[i],
					PTR_ERR(vregs_isa1200[i]));
				rc = PTR_ERR(vregs_isa1200[i]);
				goto vreg_get_fail;
			}
			rc = vreg_set_level(vregs_isa1200[i],
					vregs_isa1200_val[i]);
			if (rc) {
				pr_err("%s: vreg_set_level() = %d\n",
					__func__, rc);
				goto vreg_get_fail;
			}
		}

		rc = gpio_tlmm_config(GPIO_CFG(HAP_LVL_SHFT_MSM_GPIO, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: Could not configure gpio %d\n",
					__func__, HAP_LVL_SHFT_MSM_GPIO);
			goto vreg_get_fail;
		}

		rc = gpio_request(HAP_LVL_SHFT_MSM_GPIO, "haptics_shft_lvl_oe");
		if (rc) {
			pr_err("%s: unable to request gpio %d (%d)\n",
					__func__, HAP_LVL_SHFT_MSM_GPIO, rc);
			goto vreg_get_fail;
		}

		gpio_set_value(HAP_LVL_SHFT_MSM_GPIO, 1);
	} else {
		for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++)
			vreg_put(vregs_isa1200[i]);

		gpio_free(HAP_LVL_SHFT_MSM_GPIO);
	}

	return 0;
vreg_get_fail:
	while (i)
		vreg_put(vregs_isa1200[--i]);
	return rc;
}
static struct isa1200_platform_data isa1200_1_pdata = {
	.name = "vibrator",
	.power_on = isa1200_power,
	.dev_setup = isa1200_dev_setup,
	.pwm_ch_id = 1, /*channel id*/
	/*gpio to enable haptic*/
	.hap_en_gpio = PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HAP_ENABLE),
	.max_timeout = 15000,
	.mode_ctrl = PWM_GEN_MODE,
	.pwm_fd = {
		.pwm_div = 256,
	},
	.is_erm = false,
	.smart_en = true,
	.ext_clk_en = true,
	.chip_en = 1,
};

static struct i2c_board_info msm_isa1200_board_info[] = {
	{
		I2C_BOARD_INFO("isa1200_1", 0x90>>1),
		.platform_data = &isa1200_1_pdata,
	},
};


static int kp_flip_mpp_config(void)
{
	return pm8058_mpp_config_digital_in(PM_FLIP_MPP,
		PM8058_MPP_DIG_LEVEL_S3, PM_MPP_DIN_TO_INT);
}

static struct flip_switch_pdata flip_switch_data = {
	.name = "kp_flip_switch",
	.flip_gpio = PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS) + PM_FLIP_MPP,
	.left_key = KEY_OPEN,
	.right_key = KEY_CLOSE,
	.active_low = 0,
	.wakeup = 1,
	.flip_mpp_config = kp_flip_mpp_config,
};

static struct platform_device flip_switch_device = {
	.name   = "kp_flip_switch",
	.id	= -1,
	.dev    = {
		.platform_data = &flip_switch_data,
	}
};

static const char *vregs_tma300_name[] = {
	"gp6",
	"gp7",
};

static const int vregs_tma300_val[] = {
	3050,
	1800,
};
static struct vreg *vregs_tma300[ARRAY_SIZE(vregs_tma300_name)];

static int tma300_power(int vreg_on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_tma300_name); i++) {
		if (!vregs_tma300[i]) {
			printk(KERN_ERR "%s: vreg_get %s failed (%d)\n",
				__func__, vregs_tma300_name[i], rc);
			goto vreg_fail;
		}

		rc = vreg_on ? vreg_enable(vregs_tma300[i]) :
			  vreg_disable(vregs_tma300[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
				__func__, vregs_tma300_name[i],
			       vreg_on ? "enable" : "disable", rc);
			goto vreg_fail;
		}
	}
	return 0;

vreg_fail:
	while (i)
		vreg_disable(vregs_tma300[--i]);
	return rc;
}

#define TS_GPIO_IRQ 150

static int tma300_dev_setup(bool enable)
{
	int i, rc;

	if (enable) {
		/* get voltage sources */
		for (i = 0; i < ARRAY_SIZE(vregs_tma300_name); i++) {
			vregs_tma300[i] = vreg_get(NULL, vregs_tma300_name[i]);
			if (IS_ERR(vregs_tma300[i])) {
				pr_err("%s: vreg get %s failed (%ld)\n",
					__func__, vregs_tma300_name[i],
					PTR_ERR(vregs_tma300[i]));
				rc = PTR_ERR(vregs_tma300[i]);
				goto vreg_get_fail;
			}
			rc = vreg_set_level(vregs_tma300[i],
					vregs_tma300_val[i]);
			if (rc) {
				pr_err("%s: vreg_set_level() = %d\n",
					__func__, rc);
				i++;
				goto vreg_get_fail;
			}
		}

		/* enable interrupt gpio */
		rc = gpio_tlmm_config(GPIO_CFG(TS_GPIO_IRQ, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: Could not configure gpio %d\n",
					__func__, TS_GPIO_IRQ);
			goto vreg_get_fail;
		}

		rc = gpio_request(TS_GPIO_IRQ, "ts_irq");
		if (rc) {
			pr_err("%s: unable to request gpio %d (%d)\n",
					__func__, TS_GPIO_IRQ, rc);
			goto vreg_get_fail;
		}

		/* virtual keys */
		tma300_vkeys_attr.attr.name = "virtualkeys.msm_tma300_ts";
		properties_kobj = kobject_create_and_add("board_properties",
					NULL);
		if (properties_kobj)
			rc = sysfs_create_group(properties_kobj,
				&tma300_properties_attr_group);
		if (!properties_kobj || rc)
			pr_err("%s: failed to create board_properties\n",
					__func__);
	} else {
		/* put voltage sources */
		for (i = 0; i < ARRAY_SIZE(vregs_tma300_name); i++)
			vreg_put(vregs_tma300[i]);
		/* free gpio */
		gpio_free(TS_GPIO_IRQ);
		/* destroy virtual keys */
		if (properties_kobj)
			sysfs_remove_group(properties_kobj,
				&tma300_properties_attr_group);
	}
	return 0;

vreg_get_fail:
	while (i)
		vreg_put(vregs_tma300[--i]);
	return rc;
}

static struct cy8c_ts_platform_data cy8ctma300_pdata = {
	.power_on = tma300_power,
	.dev_setup = tma300_dev_setup,
	.ts_name = "msm_tma300_ts",
	.dis_min_x = 0,
	.dis_max_x = 479,
	.dis_min_y = 0,
	.dis_max_y = 799,
	.res_x	 = 479,
	.res_y	 = 1009,
	.min_tid = 1,
	.max_tid = 255,
	.min_touch = 0,
	.max_touch = 255,
	.min_width = 0,
	.max_width = 255,
	.use_polling = 1,
	.invert_y = 1,
	.nfingers = 4,
};

static struct i2c_board_info cy8ctma300_board_info[] = {
	{
		I2C_BOARD_INFO("cy8ctma300", 0x2),
		.platform_data = &cy8ctma300_pdata,
	}
};

//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+{
#ifdef CONFIG_MSM_UART2DM
#define AMDH0_BASE_PHYS     0xAC200000
#define ADMH0_GP_CTL        (ct_adm_base + 0x3D8)
static void __init uart2dm_device_init(void)
{	
	void __iomem *ct_adm_base = 0;
	u32 uart2dm_mux = 0;
	
	ct_adm_base = ioremap(AMDH0_BASE_PHYS, PAGE_SIZE);
	if (!ct_adm_base) {
		pr_err("%s: Could not remap %x\n", __func__, AMDH0_BASE_PHYS);
	}

	uart2dm_mux = ioread32(ADMH0_GP_CTL); 
	uart2dm_mux = uart2dm_mux & ((u32)0xFFFFCFFF);
	iowrite32(uart2dm_mux, ADMH0_GP_CTL);
	uart2dm_mux = (ioread32(ADMH0_GP_CTL) & (0x3 << 12)) >> 12;

	iounmap(ct_adm_base);

	gpio_tlmm_config(GPIO_CFG(85, 3, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(87, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	//gpio_tlmm_config(GPIO_CFG(88, 3, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	//gpio_tlmm_config(GPIO_CFG(89, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
}
#endif
//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+}

//DIV5-PHONE-JH-WiMAX_GPIO-01+[
#if defined(CONFIG_FIH_PROJECT_SF4Y6) && defined(CONFIG_FIH_WIMAX_GCT_SDIO)

#define HOST_WAKEUP_WiMAX_N    143
#define WiMAX_WAKEUP_HOST_N_PR3    142   //DIV5-CONN-MW-POWER SAVING MODE-01+
#define WiMAX_V3P8_FET_CTRL_N  148
#define WiMAX_ROM_RP           149

static struct msm_gpio wimax_config_data[] = {
    { GPIO_CFG(HOST_WAKEUP_WiMAX_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "host_wakeup_wimax_n" },
    { GPIO_CFG(WiMAX_V3P8_FET_CTRL_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "wimax_v3p8_fet_ctrl_n" },
    { GPIO_CFG(WiMAX_ROM_RP, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "wimax_rom_rp" },
};

//DIV5-CONN-MW-POWER SAVING MODE-01+[
static struct msm_gpio wimax_config_data_PR3[] = {
    { GPIO_CFG(WiMAX_WAKEUP_HOST_N_PR3, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "wimax_wakeup_host_n_pr3" },
    { GPIO_CFG(WiMAX_V3P8_FET_CTRL_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "wimax_v3p8_fet_ctrl_n" },
    { GPIO_CFG(WiMAX_ROM_RP, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "wimax_rom_rp" },
};
//DIV5-CONN-MW-POWER SAVING MODE-01+]
static int wimax_gpio_init(void)
{
    int rc = 0;
	int wimax_product_phase = fih_get_product_phase(); //DIV5-PHONE-JH-SF6.B-477-01+
//DIV5-CONN-MW-POWER SAVING MODE-01+[
    if (fih_get_product_id() == Product_SF6) 
    {
        if (fih_get_product_phase() <= Product_PR2)
    rc = msm_gpios_request_enable(wimax_config_data, ARRAY_SIZE(wimax_config_data));
        else
            rc = msm_gpios_request_enable(wimax_config_data_PR3, ARRAY_SIZE(wimax_config_data_PR3));
     }
//DIV5-CONN-MW-POWER SAVING MODE-01+]
    if (rc < 0) {
        printk(KERN_ERR
                "%s: msm_gpios_request_enable failed (%d)\n",
                __func__, rc);
        goto out;
    }

    gpio_set_value(WiMAX_ROM_RP, 0);
//DIV5-PHONE-JH-SF6.B-477-01*[	
	if (wimax_product_phase >= Product_PR2){
        gpio_set_value(WiMAX_V3P8_FET_CTRL_N, 0);
	}else{
        gpio_set_value(WiMAX_V3P8_FET_CTRL_N, 1);
	}
//DIV5-PHONE-JH-SF6.B-477-01*]	

out:
    return rc;
}
#endif
//DIV5-PHONE-JH-WiMAX_GPIO-01+]

static void __init msm7x30_init(void)
{
	int rc;
	//SW5-Multimedia-TH-MT9P111forV6-00+{
	int pid = Product_FB0;
	int cnt = 0;
	int camera_num = 0;
	//SW5-Multimedia-TH-MT9P111forV6-00+}
	uint32_t usb_hub_gpio_cfg_value = GPIO_CFG(56,
						0,
						GPIO_CFG_OUTPUT,
						GPIO_CFG_NO_PULL,
						GPIO_CFG_2MA);
	uint32_t soc_version = 0;
						
	fih_get_oem_info(); //KC-OEMInfo-00+
	fih_get_host_oem_info(); // SW2-5-1-MP-HostOemInfo-00+
  
	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);

	soc_version = socinfo_get_version();

	msm_clock_init(msm_clocks_7x30, msm_num_clocks_7x30);
#ifdef CONFIG_SERIAL_MSM_CONSOLE
	msm7x30_init_uart2();
#endif
	msm_spm_init(&msm_spm_data, 1);
	msm_acpu_clock_init(&msm7x30_clock_data);
/* Div2-SW2-BSP-FBX-OW { */
#ifdef CONFIG_SMSC911X
	if (machine_is_msm7x30_surf() || machine_is_msm7x30_fluid())
		msm7x30_cfg_smsc911x();
#endif
/* } Div2-SW2-BSP-FBX-OW */
#ifdef CONFIG_USB_FUNCTION
	msm_hsusb_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	if (SOCINFO_VERSION_MAJOR(soc_version) >= 2 &&
			SOCINFO_VERSION_MINOR(soc_version) >= 1) {
		pr_debug("%s: SOC Version:2.(1 or more)\n", __func__);
		msm_otg_pdata.ldo_set_voltage = 0;
	}

	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#ifdef CONFIG_USB_GADGET
	msm_otg_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif
#endif
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(136);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+{
#ifdef CONFIG_MSM_UART2DM
	msm_uart_dm2_pdata.wakeup_irq = gpio_to_irq(85); 
	msm_device_uart_dm2.dev.platform_data = &msm_uart_dm2_pdata;
	//uart2dm_device_init(); //Div2D5-OwenHuang-SF5_Reset_L8-00-
#endif
//Div2D5-OwenHuang-BSP2030_SF5_IR_MCU_Settings-00+}
	camera_sensor_hwpin_init();

#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	msm_device_tsif.dev.platform_data = &tsif_platform_data;
#endif
	if (machine_is_msm7x30_fluid()) {
		msm_adc_pdata.dev_names = msm_adc_fluid_device_names;
		msm_adc_pdata.num_adc = ARRAY_SIZE(msm_adc_fluid_device_names);
	} else {
		msm_adc_pdata.dev_names = msm_adc_surf_device_names;
		msm_adc_pdata.num_adc = ARRAY_SIZE(msm_adc_surf_device_names);
	}
	platform_add_devices(devices, ARRAY_SIZE(devices));
#ifdef CONFIG_USB_EHCI_MSM
	msm_add_host(0, &msm_usb_host_pdata);
#endif

	sync_from_custom_nv();//Div2-5-3-Peripheral-LL-UsbPorting-00+
    
	msm7x30_init_mmc();
	msm7x30_init_nand();
//SW2-6-MM-JH-SPI-00+
#ifdef CONFIG_SPI_QSD
	msm_qsd_spi_init();

	if (machine_is_msm7x30_fluid())
		spi_register_board_info(lcdc_sharp_spi_board_info,
			ARRAY_SIZE(lcdc_sharp_spi_board_info));
	else
		spi_register_board_info(lcdc_toshiba_spi_board_info,
			ARRAY_SIZE(lcdc_toshiba_spi_board_info));
#endif
//SW2-6-MM-JH-SPI-00-

	msm_fb_add_devices();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */    
	msm_device_i2c_power_domain();
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */
	msm_device_i2c_init();
	msm_device_i2c_2_init();
	qup_device_i2c_init();
	buses_init();
	//Div2D5-OwenHuang-SF5_Reset_L8-00+{
#ifdef CONFIG_MSM_UART2DM
	uart2dm_device_init();
#endif
//Div2D5-OwenHuang-SF5_Reset_L8-00+}
	msm7x30_init_marimba();
#ifdef CONFIG_MSM7KV2_AUDIO
	snddev_poweramp_gpio_init();
	aux_pcm_gpio_init();
#endif

/* Div2-SW2-BSP-FBX-BATT { */
#ifdef CONFIG_BATTERY_FIH_MSM
	fih_battery_driver_init();
#endif
/* } Div2-SW2-BSP-FBX-BATT */

	i2c_register_board_info(0, msm_i2c_board_info,
			ARRAY_SIZE(msm_i2c_board_info));

	if (!machine_is_msm8x55_svlte_ffa())
		marimba_pdata.tsadc = &marimba_tsadc_pdata;

	if (machine_is_msm7x30_fluid())
		i2c_register_board_info(0, cy8info,
					ARRAY_SIZE(cy8info));

	i2c_register_board_info(2, msm_marimba_board_info,
			ARRAY_SIZE(msm_marimba_board_info));

	//SW5-Multimedia-TH-MT9P111forV6-00+{
	pid = fih_get_product_id();
	camera_num = ARRAY_SIZE(msm_camera_boardinfo);
	if (pid == Product_SF6 || IS_SF8_SERIES_PRJ())//Div2-SW6-MM-MC-ImplementCameraFTMforSF8Serials-00*
	{
		for (cnt = 0; cnt < camera_num; cnt++)
		{
			if(strncasecmp(msm_camera_boardinfo[cnt].type, "mt9p111", 7) == 0)
			{
				printk(KERN_INFO "%s: Old camera slave address = %x .\n", __func__, msm_camera_boardinfo[cnt].addr);
				msm_camera_boardinfo[cnt].addr = (0x7A >> 1);
				printk(KERN_INFO "%s: New camera slave address = %x .\n", __func__, msm_camera_boardinfo[cnt].addr);
			}
		}

            if (IS_SF8_SERIES_PRJ())
                printk(KERN_INFO "%s: This is SF8 series project.....\n", __func__);
	}
	//SW5-Multimedia-TH-MT9P111forV6-00+}

	i2c_register_board_info(2, msm_i2c_gsbi7_timpani_info,
			ARRAY_SIZE(msm_i2c_gsbi7_timpani_info));

	i2c_register_board_info(4 /* QUP ID */, msm_camera_boardinfo,
				ARRAY_SIZE(msm_camera_boardinfo));

	bt_power_init();
// FIHTDC-SW2-Div6-CW-Project BCM4329 WLAN driver For SF8 +[
#ifdef CONFIG_BROADCOM_BCM4329_WLAN_POWER
	bcm4329_wifi_power_init();
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 WLAN driver For SF8 +]
// FIHTDC-SW2-Div6-CW-Project BCM4329 BLUETOOTH driver For SF8 +[
#ifdef CONFIG_BROADCOM_BCM4329_BLUETOOTH_POWER
	bcm4329_bt_power_init();
	bcm4329_fm_power_init();
#endif
// FIHTDC-SW2-Div6-CW-Project BCM4329 BLUETOOTH driver For SF8 +]
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi6.dev.platform_data = &msm_i2c_ssbi6_pdata;
	msm_device_ssbi7.dev.platform_data = &msm_i2c_ssbi7_pdata;
#endif
	if (machine_is_msm7x30_fluid())
		i2c_register_board_info(0, msm_isa1200_board_info,
			ARRAY_SIZE(msm_isa1200_board_info));

#if defined(CONFIG_TOUCHSCREEN_TSC2007) || \
	defined(CONFIG_TOUCHSCREEN_TSC2007_MODULE)
	if (machine_is_msm8x55_svlte_ffa())
		i2c_register_board_info(2, tsc_i2c_board_info,
				ARRAY_SIZE(tsc_i2c_board_info));
#endif

	if (machine_is_msm7x30_surf())
		platform_device_register(&flip_switch_device);

/* Div2-SW2-BSP-FBX-LEDS { */
#ifdef CONFIG_LEDS_PMIC8058
	pmic8058_leds_init();
#endif
/* } Div2-SW2-BSP-FBX-LEDS */

	if (machine_is_msm7x30_fluid()) {
		/* Initialize platform data for fluid v2 hardware */
		if (SOCINFO_VERSION_MAJOR(
				socinfo_get_platform_version()) == 2) {
			cy8ctma300_pdata.res_y = 920;
			cy8ctma300_pdata.invert_y = 0;
			cy8ctma300_pdata.use_polling = 0;
			cy8ctma300_board_info[0].irq =
				MSM_GPIO_TO_INT(TS_GPIO_IRQ);
		}
		i2c_register_board_info(0, cy8ctma300_board_info,
			ARRAY_SIZE(cy8ctma300_board_info));
	}

	if (machine_is_msm8x55_svlte_surf() || machine_is_msm8x55_svlte_ffa()) {
		rc = gpio_tlmm_config(usb_hub_gpio_cfg_value, GPIO_CFG_ENABLE);
		if (rc)
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, usb_hub_gpio_cfg_value, rc);
	}

//Div2-SW2-BSP,JOE HSU,+++
//#ifdef CONFIG_FIH_CONFIG_GROUP
    /*call product id functions for init verification*/
	fih_get_product_id();
	fih_get_product_phase();
	fih_get_band_id();
	fxx_info_init();
//#endif      
//Div2-SW2-BSP,JOE HSU,---

//DIV5-PHONE-JH-WiMAX_GPIO-01+[
#if defined(CONFIG_FIH_PROJECT_SF4Y6) && defined(CONFIG_FIH_WIMAX_GCT_SDIO)
	if (wimax_gpio_init())
		printk(KERN_INFO "%s: wimax gpio init fails\n", __func__); 
#endif
//DIV5-PHONE-JH-WiMAX_GPIO-01+]
}

static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static void __init pmem_sf_size_setup(char **p)
{
	pmem_sf_size = memparse(*p, p);
}
__early_param("pmem_sf_size=", pmem_sf_size_setup);

static unsigned fb_size = MSM_FB_SIZE;
static void __init fb_size_setup(char **p)
{
	fb_size = memparse(*p, p);
}
__early_param("fb_size=", fb_size_setup);

static unsigned gpu_phys_size = MSM_GPU_PHYS_SIZE;
static void __init gpu_phys_size_setup(char **p)
{
	gpu_phys_size = memparse(*p, p);
}
__early_param("gpu_phys_size=", gpu_phys_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

static unsigned fluid_pmem_adsp_size = MSM_FLUID_PMEM_ADSP_SIZE;
static void __init fluid_pmem_adsp_size_setup(char **p)
{
	fluid_pmem_adsp_size = memparse(*p, p);
}
__early_param("fluid_pmem_adsp_size=", fluid_pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static void __init pmem_audio_size_setup(char **p)
{
	pmem_audio_size = memparse(*p, p);
}
__early_param("pmem_audio_size=", pmem_audio_size_setup);

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static void __init msm7x30_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;
/*
   Request allocation of Hardware accessible PMEM regions
   at the beginning to make sure they are allocated in EBI-0.
   This will allow 7x30 with two mem banks enter the second
   mem bank into Self-Refresh State during Idle Power Collapse.

    The current HW accessible PMEM regions are
    1. Frame Buffer.
       LCDC HW can access msm_fb_resources during Idle-PC.

    2. Audio
       LPA HW can access android_pmem_audio_pdata during Idle-PC.
*/
	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

	size = pmem_audio_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = gpu_phys_size;
	if (size) {
		addr = alloc_bootmem(size);
		msm_kgsl_resources[1].start = __pa(addr);
		msm_kgsl_resources[1].end = msm_kgsl_resources[1].start + size - 1;
		pr_info("allocating %lu bytes at %p (%lx physical) for "
			"KGSL\n", size, addr, __pa(addr));
	}

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_sf_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for sf "
			"pmem arena\n", size, addr, __pa(addr));
	}

	if machine_is_msm7x30_fluid()
		size = fluid_pmem_adsp_size;
	else
		size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}
}

static void __init msm7x30_map_io(void)
{
	msm_shared_ram_phys = 0x00100000;
	msm_map_msm7x30_io();
	msm7x30_allocate_memory_regions();
}

MACHINE_START(MSM7X30_SURF, "QCT MSM7X30 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM7X30_FFA, "QCT MSM7X30 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM7X30_FLUID, "QCT MSM7X30 FLUID")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM8X55_SURF, "QCT MSM8X55 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(MSM8X55_FFA, "TRIUMPH")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
MACHINE_START(MSM8X55_SVLTE_SURF, "QCT MSM8X55 SVLTE SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
MACHINE_START(MSM8X55_SVLTE_FFA, "QCT MSM8X55 SVLTE FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
