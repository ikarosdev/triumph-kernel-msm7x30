/*
 * Copyright (C) 2008 QUALCOMM Incorporated.
 * Copyright (c) 2008 QUALCOMM USA, INC.
 *
 * All source code in this file is licensed under the following license
 * except where indicated.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#include "msm_fb.h"
#include "mddihost.h"
#include "mddihosti.h"
#include <asm/gpio.h>
#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
#include <mach/vreg.h>

static int mddi_toshiba_fwvga_lcd_on(struct	platform_device *pdev);
static int mddi_toshiba_fwvga_lcd_off(struct platform_device *pdev);
static void mddi_toshiba_fwvga_lcd_set_backlight(struct msm_fb_data_type *mfd);

static unsigned int GPIO_LCM_RESET = 35;
static struct vreg *vreg_gp6;    // 2.8V VDD
static struct vreg *vreg_lvsw1;  // 1.8V VDDIO

struct init_table {
    unsigned int reg;
    unsigned int val;
};

/* Panel    : Toshiba 4.02" FWVGA (480x854) MDDI TFT LCD */
/* Driver IC: Novatek NT35560 (MDDI to RGB)              */
static struct init_table NT35560_TOSHIBA_init_table[] = {
        { 0, 120 },          // Wait for 120ms
//#ifndef CONFIG_FB_MSM_MDDI_2
//        { 0xAE00, 0x0000 },  // SET_MIPI_MDDI: 0x0000: MDDI 1.0; default: MDDI 1.2
//#endif
/*Div2-SW6-SC-Panel_Init_timing-00*{*/
//        { 0x1100, 0x0000 },  // EXIT_SLEEP_MODE: Exit the Sleep-In Mode
//        { 0, 120 },          // Wait for 120ms
/*Div2-SW6-SC-Panel_Init_timing-00*}*/
        { 0xC980, 0x0001 },
        { 0x7DC0, 0x0001 },
        { 0x0180, 0x0014 },
        { 0x0280, 0x0000 },
        { 0x0380, 0x0033 },
        { 0x0480, 0x0048 },
        { 0x0780, 0x0000 },
        { 0x0880, 0x0044 },
        { 0x0980, 0x0054 },
        { 0x0A80, 0x0012 },
        { 0x1280, 0x0000 },
        { 0x1380, 0x0010 },
        { 0x1480, 0x000D },
        { 0x1580, 0x00A0 },
        { 0x1A80, 0x0067 },
        { 0x1F80, 0x0000 },
        { 0x2080, 0x0001 },
        { 0x2180, 0x0063 },

        //Gamma Setting (2.4) {
        { 0x2480, 0x0009 },
        { 0x2580, 0x0028 },
        { 0x2680, 0x0062 },
        { 0x2780, 0x0079 },
        { 0x2880, 0x001E },
        { 0x2980, 0x0036 },
        { 0x2A80, 0x0064 },
        { 0x2B80, 0x008B },
        { 0x2D80, 0x0021 },
        { 0x2F80, 0x0029 },
        { 0x3080, 0x00C2 },
        { 0x3180, 0x001A },
        { 0x3280, 0x0038 },
        { 0x3380, 0x004D },
        { 0x3480, 0x009F },
        { 0x3580, 0x00BD },
        { 0x3680, 0x00D9 },
        { 0x3780, 0x0076 },
        { 0x3880, 0x0009 },
        { 0x3980, 0x0026 },
        { 0x3A80, 0x0042 },
        { 0x3B80, 0x0060 },
        { 0x3D80, 0x0032 },
        { 0x3F80, 0x0047 },
        { 0x4080, 0x0065 },
        { 0x4180, 0x003D },
        { 0x4280, 0x0016 },
        { 0x4380, 0x001E },
        { 0x4480, 0x0074 },
        { 0x4580, 0x001B },
        { 0x4680, 0x0049 },
        { 0x4780, 0x0061 },
        { 0x4880, 0x0086 },
        { 0x4980, 0x009D },
        { 0x4A80, 0x00D7 },
        { 0x4B80, 0x0076 },
        { 0x4C80, 0x0009 },
        { 0x4D80, 0x0028 },
        { 0x4E80, 0x0062 },
        { 0x4F80, 0x0079 },
        { 0x5080, 0x001E },
        { 0x5180, 0x0036 },
        { 0x5280, 0x0064 },
        { 0x5380, 0x0094 },
        { 0x5480, 0x0021 },
        { 0x5580, 0x0029 },
        { 0x5680, 0x00C7 },
        { 0x5780, 0x001D },
        { 0x5880, 0x003E },
        { 0x5980, 0x0053 },
        { 0x5A80, 0x009D },
        { 0x5B80, 0x00AE },
        { 0x5C80, 0x00D0 },
        { 0x5D80, 0x0076 },
        { 0x5E80, 0x0009 },
        { 0x5F80, 0x002F },
        { 0x6080, 0x0051 },
        { 0x6180, 0x0062 },
        { 0x6280, 0x002C },
        { 0x6380, 0x0041 },
        { 0x6480, 0x0062 },
        { 0x6580, 0x0038 },
        { 0x6680, 0x0016 },
        { 0x6780, 0x001E },
        { 0x6880, 0x006B },
        { 0x6980, 0x001B },
        { 0x6A80, 0x0049 },
        { 0x6B80, 0x0061 },
        { 0x6C80, 0x0086 },
        { 0x6D80, 0x009D },
        { 0x6E80, 0x00D7 },
        { 0x6F80, 0x0076 },
        { 0x7080, 0x0019 },
        { 0x7180, 0x0028 },
        { 0x7280, 0x0062 },
        { 0x7380, 0x00A1 },
        { 0x7480, 0x0034 },
        { 0x7580, 0x0045 },
        { 0x7680, 0x0069 },
        { 0x7780, 0x00AB },
        { 0x7880, 0x0021 },
        { 0x7980, 0x0029 },
        { 0x7A80, 0x00D1 },
        { 0x7B80, 0x0018 },
        { 0x7C80, 0x003A },
        { 0x7D80, 0x0050 },
        { 0x7E80, 0x00AA },
        { 0x7F80, 0x00CE },
        { 0x8080, 0x00D0 },
        { 0x8180, 0x0076 },
        { 0x8280, 0x0009 },
        { 0x8380, 0x002F },
        { 0x8480, 0x0031 },
        { 0x8580, 0x0055 },
        { 0x8680, 0x002F },
        { 0x8780, 0x0045 },
        { 0x8880, 0x0067 },
        { 0x8980, 0x002E },
        { 0x8A80, 0x0016 },
        { 0x8B80, 0x001E },
        { 0x8C80, 0x0054 },
        { 0x8D80, 0x0016 },
        { 0x8E80, 0x003A },
        { 0x8F80, 0x004B },
        { 0x9080, 0x005E },
        { 0x9180, 0x009D },
        { 0x9280, 0x00D7 },
        { 0x9380, 0x0066 },
        //Gamma Setting (2.4) }

        { 0x9480, 0x00BF },
        { 0x9580, 0x0000 },
        { 0x9680, 0x0000 },
        { 0x9780, 0x00B4 },
        { 0x9880, 0x000D },
        { 0x9980, 0x002C },
        { 0x9A80, 0x000A },
        { 0x9B80, 0x0001 },
        { 0x9C80, 0x0001 },
        { 0x9D80, 0x0000 },
        { 0x9E80, 0x0000 },
        { 0x9F80, 0x0000 },
        { 0xA080, 0x000A },
        { 0xA280, 0x0006 },
        { 0xA380, 0x002E },
        { 0xA480, 0x000E },
        { 0xA580, 0x00C0 },
        { 0xA680, 0x0001 },
        { 0xA780, 0x0000 },
        { 0xA980, 0x0000 },
        { 0xAA80, 0x0000 },
        { 0xE780, 0x0000 },
        { 0xED80, 0x0000 },
        { 0xF380, 0x00CC },
        { 0xFB80, 0x0000 },
        { 0x68C0, 0x0008 },

        { 0x3500, 0x0000 },  // SET_TEAR_ON: Tearing Effect Line ON
        { 0x4400, 0x0000 },  // SET_TEAR_SCANLINE
        { 0x4401, 0x0000 },  // SET_TEAR_SCANLINE
//SW2-6-MM-JH-SET_ADDRESS_MODE-00+
        { 0x3600, 0x00D4 },  // SET_ADDRESS_MODE: MY = 1, MX = 1, ML = 1, MH = 1
//SW2-6-MM-JH-SET_ADDRESS_MODE-00-
        { 0x5100, 0x0000 },  // WRDISBV: Write Display Brightness
/*Div2-SW6-SC-Panel_Init_timing-00+{*/
        { 0x5300, 0x0000 },  // WRCTRLD: Write CTRL Display
        { 0, 10 },           // Wait for 10ms
        { 0x1100, 0x0000 },  // EXIT_SLEEP_MODE: Exit the Sleep-In Mode
/*Div2-SW6-SC-Panel_Init_timing-00+}*/
        { 0, 120 },
        { 0, 0 },
};

void panel_init(struct init_table *init_table)
{
    unsigned int n;
    printk(KERN_INFO "MDDI: panel_init()\n");

    n = 0;
    while (init_table[n].reg != 0 || init_table[n].val != 0) {
        if (init_table[n].reg != 0) {
            // printk(KERN_INFO "write Address(0x%08x), Data(0x%08x)\n", init_table[n].reg, init_table[n].val);
            mddi_queue_register_write(init_table[n].reg, init_table[n].val, FALSE, 0);
        } else {
            printk(KERN_INFO "mdelay %d\n", init_table[n].val);
            mddi_wait(init_table[n].val);
            printk(KERN_INFO "mdelay %d done\n", init_table[n].val);
        }

        n++;
    }

    printk(KERN_INFO "MDDI: panel_init() done\n");
}

//SW2-6-MM-JH-Panel_first_on-00+
static int panel_first_on = 1; // Set 1 if panel is initialized in appsboot
//SW2-6-MM-JH-Panel_first_on-00-

static int mddi_toshiba_fwvga_lcd_on(struct platform_device *pdev)
{
    printk(KERN_INFO "MDDI: mddi_toshiba_fwvga_lcd_on\n");

//SW2-6-MM-JH-Panel_first_on-00+
    if ( !panel_first_on ) {
        // LCM_RESET -> Low
        printk(KERN_INFO "Set GPIO %d: %d -> 0\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));
        gpio_set_value(GPIO_LCM_RESET, 0); // GPIO_LCM_RESET = 35
        printk(KERN_INFO "GPIO %d = %d\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));
    }

    // PM8058_L15_V2P85
    vreg_set_level(vreg_gp6, 2800);  // VDD
    vreg_enable(vreg_gp6);

    // PM8058_L11_V1P8
    vreg_set_level(vreg_lvsw1, 1800);  // VDDIO
    vreg_enable(vreg_lvsw1);

    mdelay(1);

    if ( !panel_first_on ) {
        // LCM_RESET -> High
        printk(KERN_INFO "Set GPIO %d: %d -> 1\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));
        gpio_set_value(GPIO_LCM_RESET, 1);
        printk(KERN_INFO "GPIO %d = %d\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));

        mdelay(15);

        panel_init(NT35560_TOSHIBA_init_table);
    }

    panel_first_on = 0;
//SW2-6-MM-JH-Panel_first_on-00-

    return 0;
}

static int mddi_toshiba_fwvga_lcd_off(struct platform_device *pdev)
{
    printk(KERN_INFO "MDDI: mddi_toshiba_fwvga_lcd_off\n");

    mddi_queue_register_write( 0x2800, 0x0000, FALSE, 0);  // SET_DISPLAY_OFF
    mddi_queue_register_write( 0x1000, 0x0000, FALSE, 0);  // ENTER_SLEEP_MODE
    mddi_wait(70);

    // LCM_RESET -> Low
    gpio_set_value(GPIO_LCM_RESET, 0);

    // PM8058_LVS1_V1P8
    vreg_disable(vreg_lvsw1);

    // PM8058_L15_V2P85
    vreg_disable(vreg_gp6);

    return 0;
}

static void mddi_toshiba_fwvga_lcd_shutdown(struct platform_device *dev)
{
    printk(KERN_INFO "MDDI: mddi_toshiba_fwvga_lcd_shutdown\n");

    mddi_queue_register_write( 0x2800, 0x0000, FALSE, 0);  // SET_DISPLAY_OFF
    mddi_queue_register_write( 0x1000, 0x0000, FALSE, 0);  // ENTER_SLEEP_MODE
    mddi_wait(70);

    // LCM_RESET -> Low
    gpio_set_value(GPIO_LCM_RESET, 0);

    // PM8058_LVS1_V1P8
    vreg_disable(vreg_lvsw1);

    // PM8058_L15_V2P85
    vreg_disable(vreg_gp6);
}

/*Div2-SW6-SC-Panel_Init_timing-00+{*/
static unsigned char bLCD_BKL_ON_State = 0;
/*Div2-SW6-SC-Panel_Init_timing-00+}*/

//SW2-6-MM-JH-PWM-01+
uint16 PWM[11] = { 0,  //Backlight off, 0%
                  28,  //Min level,  11.3%,  40.8 cd/m^2
                  51,  //            20.4%,  75.9 cd/m^2
                  74,  //            29.3%, 110.3 cd/m^2
                  97,  //            38.2%, 144.3 cd/m^2
                 122,  //            48.0%, 180.4 cd/m^2
                 146,  //            57.2%, 214.4 cd/m^2
                 172,  //            67.6%, 250.0 cd/m^2
                 197,  //            77.3%, 284.1 cd/m^2
                 224,  //            88.0%, 319.2 cd/m^2
                 255   //Max Level,   100%, 353.7 cd/m^2
};
//SW2-6-MM-JH-PWM-01-

static void mddi_toshiba_fwvga_lcd_set_backlight(struct msm_fb_data_type *mfd)
{
    printk(KERN_INFO "MDDI: set backlight level = %d\n", mfd->bl_level);

	/*Div2-SW6-SC-Panel_Init_timing-00*{*/
    if (!bLCD_BKL_ON_State) {
        mddi_queue_register_write( 0x2900, 0x0000, FALSE, 0);  // SET_DISPLAY_ON
        bLCD_BKL_ON_State = 1;
    } else if (!mfd->bl_level) {
        mddi_queue_register_write( 0x2800, 0x0000, FALSE, 0);  // SET_DISPLAY_OFF
        bLCD_BKL_ON_State = 0;
    }
	/*Div2-SW6-SC-Panel_Init_timing-00*}*/

    mddi_queue_register_write( 0x00C0, 0x0001, FALSE, 0);  // CMD2 Page CTRL: Page 1
    mddi_queue_register_write( 0x22C0, 0x0004, FALSE, 0);  // ABC CTRL11: PWMDIV; PWMDIV[7:0] = 4; PWM Frequency = 19531.3
    mddi_queue_register_write( 0x5300, 0x0036, FALSE, 0);  // WRCTRLD: Write CTRL Display; BCTRL=1, A=1,BL=1, DB=1
//SW2-6-MM-JH-PWM-00+
    mddi_queue_register_write( 0x71C0, PWM[mfd->bl_level], FALSE, 0);  // CABC_FORCE
//SW2-6-MM-JH-PWM-00-
    mddi_queue_register_write( 0x18C0, 0x0005, FALSE, 0);  // ABC_CTRL1(FORCE_LABC, LENONR, FORCE_CABC_PWM)
}

//SW2-6-MM-JH-Backlight_PWM-01+
static void mddi_toshiba_fwvga_lcd_set_backlight_pwm(struct msm_fb_data_type *mfd, struct backlight_regs *bl_regs)
{
    printk(KERN_INFO "MDDI: set backlight pwm = %d\n", bl_regs->pwm);

    mddi_queue_register_write( 0x00C0, 0x0001, FALSE, 0);  // CMD2 Page CTRL: Page 1
    mddi_queue_register_write( 0x22C0, 0x0004, FALSE, 0);  // ABC CTRL11: PWMDIV; PWMDIV[7:0] = 4; PWM Frequency = 19531.3
    mddi_queue_register_write( 0x5300, 0x0036, FALSE, 0);  // WRCTRLD: Write CTRL Display; BCTRL=1, A=1,BL=1, DB=1
    mddi_queue_register_write( 0x71C0, bl_regs->pwm , FALSE, 0);  // CABC_FORCE
    mddi_queue_register_write( 0x18C0, 0x0005, FALSE, 0);  // ABC_CTRL1(FORCE_LABC, LENONR, FORCE_CABC_PWM)
}
//SW2-6-MM-JH-Backlight_PWM-01-

static int __init mddi_toshiba_fwvga_lcd_probe(struct platform_device *pdev)
{
    vreg_lvsw1 = vreg_get(NULL, "lvsw1");
    vreg_gp6   = vreg_get(NULL, "gp6");

    vreg_pull_down_switch(vreg_gp6, 1);

    msm_fb_add_device(pdev);

    return 0;
}

static struct platform_driver this_driver = {
    .probe    = mddi_toshiba_fwvga_lcd_probe,
    .shutdown = mddi_toshiba_fwvga_lcd_shutdown,
    .driver   = {
        .name = "mddi_toshiba_fwvga",
    },
};

static struct msm_fb_panel_data mddi_toshiba_fwvga_panel_data = {
    .on  = mddi_toshiba_fwvga_lcd_on,
    .off = mddi_toshiba_fwvga_lcd_off,
    .set_backlight = mddi_toshiba_fwvga_lcd_set_backlight,
    //SW2-6-MM-JH-Backlight_PWM-01+
    .set_backlight_regs = mddi_toshiba_fwvga_lcd_set_backlight_pwm,
    //SW2-6-MM-JH-Backlight_PWM-01-
};

static struct platform_device this_device = {
    .name   = "mddi_toshiba_fwvga",
    .id     = 1,
    .dev    = {
        .platform_data = &mddi_toshiba_fwvga_panel_data,
    }
};

static int __init mddi_toshiba_fwvga_init(void)
{
    int ret;
    struct msm_panel_info *pinfo;

    ret = platform_driver_register(&this_driver);

    if (!ret) {
        pinfo = &mddi_toshiba_fwvga_panel_data.panel_info;
        pinfo->xres = 480;
        pinfo->yres = 854;
        pinfo->type = MDDI_PANEL;
        pinfo->pdest = DISPLAY_1;
        pinfo->mddi.vdopkt = MDDI_DEFAULT_PRIM_PIX_ATTR;
        pinfo->wait_cycle = 0;
//SW2-6-MM-JH-Panel_24bit-00+
        pinfo->bpp = 24;
//SW2-6-MM-JH-Panel_24bit-00-
        pinfo->fb_num = 2;

//SW2-6-MM-JH-VSYNC-00+
        pinfo->lcd.vsync_enable = TRUE;
        pinfo->lcd.refx100 = 6050;
//SW2-6-MM-JH-VSYNC-02+
        pinfo->lcd.v_back_porch  = 11; // 11 lines
//SW2-6-MM-JH-VSYNC-02-
        pinfo->lcd.v_front_porch = 4;  // 4 lines
        pinfo->lcd.v_pulse_width = 1;  // 1 line
        pinfo->lcd.hw_vsync_mode = TRUE;
//SW2-6-MM-JH-VSYNC-01+
//        pinfo->lcd.vsync_notifier_period = (1 * HZ);
        pinfo->lcd.vsync_notifier_period = 30;  // 0.3sec
//        pinfo->lcd.vsync_notifier_period = 10;
//SW2-6-MM-JH-VSYNC-01-
        pinfo->lcd.rev = 2;
        /*Div2-SW6-SC-Add_panel_size-00+{*/
        pinfo->width = 50; //50.04mm
        pinfo->height = 89;//89.03mm
        /*Div2-SW6-SC-Add_panel_size-00+}*/

        pinfo->clk_rate = 184320000;
        pinfo->clk_min  = 184320000;
        pinfo->clk_max  = 184320000;
//        pinfo->clk_rate = 245760000;
//        pinfo->clk_min  = 245760000;
//        pinfo->clk_max  = 245760000;

        printk(KERN_INFO "MDDI: pinfo->clk_rate = %d\n", pinfo->clk_rate);
//SW2-6-MM-JH-VSYNC-00-

        pinfo->bl_max = 10;
        pinfo->bl_min = 1;
//SW2-6-MM-JH-Backlight_PWM-01+
        pinfo->bl_regs.bl_type = BL_TYPE_PWM;
        pinfo->bl_regs.pwm_max = 255;
        pinfo->bl_regs.pwm_min = 1;
//SW2-6-MM-JH-Backlight_PWM-01-
        ret = platform_device_register(&this_device);

        if (ret)
            platform_driver_unregister(&this_driver);
    }

    return ret;
}

module_init(mddi_toshiba_fwvga_init);
