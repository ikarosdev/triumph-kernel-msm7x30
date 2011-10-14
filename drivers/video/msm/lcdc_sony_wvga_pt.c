/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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

#include <linux/delay.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <mach/pmic.h>
#include <mach/vreg.h>
#include <mach/msm_iomap.h>
#include <linux/clk.h>
#include "msm_fb.h"
#include <mach/rpc_pmapp.h>  // for pmapp_display_clock_config()
#include "../../../arch/arm/mach-msm/smd_private.h"  // for fih_get_product_id() and fih_get_product_phase()

static int SPI_CS_ACTIVE_HIGH = 0;  /* cs active low */

static int spi_sclk;  /* clock */
static int spi_cs;    /* chip select */
static int spi_mosi;  /* master output */

static int GPIO_LCM_RESET = 35;
//static int GPIO_LCM_PWM = 98;
//SW2-6-MM-JH-Ortustech_Panel-00+
static int GPIO_LCM_ID = 178;
//SW2-6-MM-JH-Ortustech_Panel-00-

static struct clk *gp_clk;
static struct vreg *vreg_gp6;    // 2.8V VDD
static struct vreg *vreg_lvsw1;  // 1.8V VDDIO

static boolean display_backlight_on = FALSE;

//SW2-6-MM-JH-Ortustech_Panel-00+
#define MAX_PARAMETER_LEN 130

struct init_table {
    unsigned int  cmd;
             int  len;
    unsigned char para[MAX_PARAMETER_LEN];
};

enum {
    PANEL_SONY = 0,
    PANEL_ORTUSTECH,
};

static int panel;
//SW2-6-MM-JH-Ortustech_Panel-00-

static void spi_cs_active(void)
{
    gpio_set_value(spi_cs, SPI_CS_ACTIVE_HIGH ? 1 : 0);
}

static void spi_cs_inactive(void)
{
    gpio_set_value(spi_cs, SPI_CS_ACTIVE_HIGH ? 0 : 1);
}

static void spi_write_byte(char dc, uint8 data)
{
    uint32 bit;
    int bnum;

    gpio_set_value(spi_sclk, 0); /* clk low */
    udelay(1);

    /* dc: 0 for command, 1 for parameter */
    gpio_set_value(spi_mosi, dc);
    udelay(1);

    gpio_set_value(spi_sclk, 1); /* clk high */
    udelay(1);

    bnum = 8;   /* 8 data bits */
    bit = 0x80;
    while (bnum) {
        gpio_set_value(spi_sclk, 0); /* clk low */
        udelay(1);

        if (data & bit)
            gpio_set_value(spi_mosi, 1);
        else
            gpio_set_value(spi_mosi, 0);
        udelay(1);

        gpio_set_value(spi_sclk, 1); /* clk high */
        udelay(1);

        bit >>= 1;
        bnum--;
    }
}

//SW2-6-MM-JH-Ortustech_Panel-00+
//static int spi_write(char cmd, uint32 data, int num)
static int spi_write(char cmd, unsigned char* data, int num)
//SW2-6-MM-JH-Ortustech_Panel-00-
{
    spi_cs_active();
    udelay(1);

    /* command byte first */
    spi_write_byte(0, cmd);
    udelay(1);

    /* followed by parameter bytes */
//SW2-6-MM-JH-Ortustech_Panel-00+
    while (num > 0) {
        spi_write_byte(1, *data++);
        udelay(1);
        num--;
    }
//SW2-6-MM-JH-Ortustech_Panel-00-

    spi_cs_inactive();
    udelay(1);
    return 0;
}

//SW2-6-MM-JH-Ortustech_Panel-00+
/* Panel    : Sony 3.69" WVGA (480x800) LCDC TFT LCD */
/* Driver IC: Samsung S6D16D0 */
static struct init_table sony_init_table[] = {
    { 0x11,  0, {0x00} },  // Sleep out
    { 0x3A,  1, {0x06} },  // 18bits/pixel
    { 0x29,  0, {0x00} },  // Display on
    {    0, -1, {0} }      // end of table
};

static struct init_table sony_off_table[] = {
    { 0x28,  0, {0x00} },  // Display off
    {    0,  0, {1}    },  // Wait 1 msec
    { 0x10,  1, {0x00} },  // Sleep in
    {    0,  0, {85}   },  // Wait 85 msec
    {    0, -1, {0} }      // end of table
};

/* Panel    : Ortustech 3.69" WVGA (480x800) LCDC TFT LCD */
/* Driver IC: Himax HX8363 */
static struct init_table ortustech_init_table[] = {
    { 0xB9,   3, {0xFF, 0x83, 0x63} },  // Enable extentended commands
    { 0xB1,  12, {0x81, 0x24, 0x04, 0x02, 0x02, 0x03, 0x10, 0x10, 0x34, 0x3C, 0x3F, 0x3F} },  // Set power
    { 0x11,   0, {0x00} },  // Sleep out
    { 0x20,   0, {0x00} },  // Display inversion off
    { 0x36,   1, {0x00} },  // Memory access control
//    { 0x3A,   1, {0x70} },  // Interface pixel format: 24bits/pixel
    { 0x3A,   1, {0x60} },  // Interface pixel format: 18bits/pixel
    {    0,   0, {120}  },  // Wait 120 msec
    { 0xB1,  12, {0x78, 0x24, 0x04, 0x02, 0x02, 0x03, 0x10, 0x10, 0x34, 0x3C, 0x3F, 0x3F} },  // Set power
    { 0xB3,   1, {0x01} },  // Set RGB interface related register
    { 0xB4,   9, {0x00, 0x08, 0x6E, 0x07, 0x01, 0x01, 0x62, 0x01, 0x57} },  // Set display waveform cycle

    { 0x26,   1, {0x01} },  // 0x01: Gamma 2.2 (default); 0x02: Gamma 1.8; 0x04: Gamma 2.5; 0x08: Gamma 1.0
/*
// old LUT
    { 0xC1, 127, {0x01, 0x00, 0x02, 0x0A, 0x0F, 0x14, 0x19, 0x21, 0x29, 0x2F,
                  0x36, 0x41, 0x4B, 0x53, 0x5F, 0x67, 0x6F, 0x78, 0x82, 0x89,
                  0x92, 0x9C, 0xA5, 0xAE, 0xB7, 0xBE, 0xC6, 0xCD, 0xD6, 0xDF,
                  0xE9, 0xF2, 0xF7, 0xFF, 0x20, 0x0F, 0x74, 0xA9, 0xBD, 0x41,
                  0x70, 0x85, 0xC0, 0x00, 0x00, 0x10, 0x15, 0x1A, 0x21, 0x26,
                  0x2C, 0x32, 0x39, 0x41, 0x48, 0x50, 0x57, 0x5F, 0x66, 0x6E,
                  0x75, 0x7D, 0x84, 0x8C, 0x93, 0x9B, 0xA5, 0xAF, 0xB9, 0xC3,
                  0xCD, 0xD7, 0xE1, 0xEB, 0xF5, 0xFF, 0x06, 0xCB, 0x22, 0x22,
                  0x22, 0x20, 0x00, 0x00, 0xC0, 0x00, 0x0A, 0x14, 0x1E, 0x28,
                  0x28, 0x31, 0x37, 0x3E, 0x48, 0x51, 0x59, 0x61, 0x6A, 0x6F,
                  0x78, 0x81, 0x89, 0x90, 0x98, 0xA0, 0xA9, 0xB3, 0xB9, 0xC0,
                  0xC8, 0xCD, 0xD6, 0xDF, 0xE9, 0xF2, 0xF7, 0xFF, 0x00, 0x32,
                  0x0B, 0xCE, 0xCB, 0xF1, 0x68, 0x85, 0xC0} },  // Set gamma curve related setting (DGC LUT)
*/
// new LUT
    { 0xC1, 127, {0x01,
                  0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48,
                  0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x98,
                  0xA0, 0xA8, 0xB0, 0xB8, 0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8,
                  0xF0, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00,

                  0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48,
                  0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x98,
                  0xA0, 0xA8, 0xB0, 0xB8, 0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8,
                  0xF0, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00,

                  0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48,
                  0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x98,
                  0xA0, 0xA8, 0xB0, 0xB8, 0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8,
                  0xF0, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00} },  // Set gamma curve related setting (DGC LUT)

    { 0xCC,   1, {0x0B} },  // Set panel
    { 0xE0,  30, {0x03, 0x49, 0x4E, 0x4C, 0x57, 0xF4, 0x0B, 0x4E, 0x92, 0x57,
                  0x1A, 0x99, 0x96, 0x0C, 0x10, 0x01, 0x47, 0x4D, 0x57, 0x62,
                  0xFF, 0x0A, 0x4E, 0xD1, 0x16, 0x19, 0x98, 0xD6, 0x0E, 0x11} },  // Set gamma curve related setting
    {    0,   0, {5}    },  // Wait 5 msec
    { 0x29,   0, {0x00} },  // Display on
    {    0,  -1, {0} }      // end of table
};

static struct init_table ortustech_off_table[] = {
    { 0x28,  0, {0x00} },  // Display off
    {    0,  0, {5}    },  // Wait 5 msec
    { 0x10,  0, {0x00} },  // Sleep in
    {    0,  0, {34}   },  // Wait 2 frames
    {    0, -1, {0} }      // end of table
};
//SW2-6-MM-JH-Ortustech_Panel-00-

//SW2-6-MM-JH-Ortustech_Panel-00+
//static void panel_init(void)
void panel_init(struct init_table *init_table)
//SW2-6-MM-JH-Ortustech_Panel-00-
{
    unsigned int n = 0;
    printk(KERN_INFO "[DISPLAY] %s\n", __func__);

    gpio_set_value(spi_cs, SPI_CS_ACTIVE_HIGH ? 0 : 1);
    gpio_set_value(spi_sclk, 1);
    gpio_set_value(spi_mosi, 1);

//SW2-6-MM-JH-Ortustech_Panel-00+
    while (init_table[n].len >= 0) {
        if(init_table[n].cmd == 0 && init_table[n].len == 0 && init_table[n].para[0] != 0) {
            mdelay(init_table[n].para[0]); //thread_sleep(init_table[n].para[0]);
        } else {
            spi_write(init_table[n].cmd, init_table[n].para, init_table[n].len);
        }
        n++;
    }
//SW2-6-MM-JH-Ortustech_Panel-00-

//SW2-6-MM-JH-Pull_Down_SPI-00+
    gpio_set_value(spi_cs,   0);
    gpio_set_value(spi_sclk, 0);
    gpio_set_value(spi_mosi, 0);
//SW2-6-MM-JH-Pull_Down_SPI-00+

    printk(KERN_INFO "[DISPLAY] %s done\n", __func__);
}

static int panel_first_on = 1;  // Set 1 if panel is initialized in appsboot

static int lcdc_panel_on(struct platform_device *pdev)
{
    int ret;

    if ( panel_first_on == 1 ) {
        // PM8058_L15_V2P85
        vreg_set_level(vreg_gp6, 2800);  // VDD
        vreg_enable(vreg_gp6);

        // PM8058_L11_V1P8
        vreg_set_level(vreg_lvsw1, 1800);  // VDDIO
        vreg_enable(vreg_lvsw1);

        panel_first_on = 0;
        return 0;
    }

    ret = gpio_tlmm_config(GPIO_CFG(GPIO_LCM_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    if (ret) {
        printk(KERN_INFO "[DISPLAY] %s: gpio_tlmm_config: %d failed...\n", __func__, GPIO_LCM_RESET);
        return ret;
    }

    printk(KERN_INFO "[DISPLAY] %s: Power ON...\n", __func__);

    // LCM_RESET -> Low
    printk(KERN_INFO "Set GPIO %d: %d -> 0\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));
    gpio_set_value(GPIO_LCM_RESET, 0); // GPIO_LCM_RESET = 35
    printk(KERN_INFO "GPIO %d = %d\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));

    vreg_set_level(vreg_gp6, 2800);  // VDD
    vreg_enable(vreg_gp6);

    vreg_set_level(vreg_lvsw1, 1800);  // VDDIO
    vreg_enable(vreg_lvsw1);

//SW2-6-MM-JH-Ortustech_Panel-00+
    if (gpio_get_value(GPIO_LCM_ID) == 0) {
        mdelay(1);
    } else {
        mdelay(10);
    }
//SW2-6-MM-JH-Ortustech_Panel-00-

    // LCM_RESET -> High
    printk(KERN_INFO "Set GPIO %d: %d -> 1\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));
    gpio_set_value(GPIO_LCM_RESET, 1);
    printk(KERN_INFO "GPIO %d = %d\n", GPIO_LCM_RESET, gpio_get_value(GPIO_LCM_RESET));

//SW2-6-MM-JH-Ortustech_Panel-00+
    if (gpio_get_value(GPIO_LCM_ID) == 0) {
        mdelay(10);
    } else {
        mdelay(44);  // Wait 10 msec + 2 frames
    }
//SW2-6-MM-JH-Ortustech_Panel-00-

//SW2-6-MM-JH-Current_Leakage-00+
//    gpio_tlmm_config( GPIO_CFG( 18, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_grn0  */
//    gpio_tlmm_config( GPIO_CFG( 19, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_grn1  */
//    gpio_tlmm_config( GPIO_CFG( 20, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_blu0  */
//    gpio_tlmm_config( GPIO_CFG( 21, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_blu1  */
//    gpio_tlmm_config( GPIO_CFG( 22, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_blu2  */
//    gpio_tlmm_config( GPIO_CFG( 23, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_red0  */
//    gpio_tlmm_config( GPIO_CFG( 24, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_red1  */
//    gpio_tlmm_config( GPIO_CFG( 25, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_red2  */
//    gpio_tlmm_config( GPIO_CFG( 45, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* spi_clk    */
//    gpio_tlmm_config( GPIO_CFG( 46, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,   GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* spi_cs0    */
//    gpio_tlmm_config( GPIO_CFG( 47, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* spi_mosi   */
//    gpio_tlmm_config( GPIO_CFG( 90, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), GPIO_CFG_ENABLE);  /* lcdc_pclk  */
//    gpio_tlmm_config( GPIO_CFG( 91, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_en    */
    gpio_tlmm_config( GPIO_CFG( 92, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_vsync */
    gpio_tlmm_config( GPIO_CFG( 93, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_hsync */
//    gpio_tlmm_config( GPIO_CFG( 94, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_grn2  */
//    gpio_tlmm_config( GPIO_CFG( 95, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_grn3  */
//    gpio_tlmm_config( GPIO_CFG( 96, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_grn4  */
//    gpio_tlmm_config( GPIO_CFG( 97, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_grn5  */
//    gpio_tlmm_config( GPIO_CFG(100, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_blu3  */
//    gpio_tlmm_config( GPIO_CFG(101, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_blu4  */
//    gpio_tlmm_config( GPIO_CFG(102, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_blu5  */
//    gpio_tlmm_config( GPIO_CFG(105, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_red3  */
//    gpio_tlmm_config( GPIO_CFG(106, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_red4  */
//    gpio_tlmm_config( GPIO_CFG(107, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_red5  */
//    gpio_tlmm_config( GPIO_CFG(178, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcm_id     */
//SW2-6-MM-JH-Current_Leakage-00-

    ret = pmapp_display_clock_config(1);
    if (ret) {
        pr_err("%s pmapp_display_clock_config rc=%d\n",
                __func__, ret);
        return ret;
    }

//SW2-6-MM-JH-Ortustech_Panel-00+
    if ( panel == PANEL_SONY )
        panel_init(sony_init_table);
    else
        panel_init(ortustech_init_table);
//SW2-6-MM-JH-Ortustech_Panel-00-

    return 0;
}

static int lcdc_panel_off(struct platform_device *pdev)
{
    int ret;

    printk(KERN_INFO "[DISPLAY] %s\n", __func__);

//SW2-6-MM-JH-Ortustech_Panel-00+
    if ( panel == PANEL_SONY ) {
        panel_init(sony_off_table);
    } else {
        panel_init(ortustech_off_table);
    }
//SW2-6-MM-JH-Ortustech_Panel-00-

    // LCM_RESET -> Low
    gpio_set_value(GPIO_LCM_RESET, 0);

    // PM8058_LVS1_V1P8
    vreg_disable(vreg_lvsw1);

    // PM8058_L15_V2P85
    vreg_disable(vreg_gp6);

    ret = pmapp_display_clock_config(0);
    if (ret) {
        pr_err("%s pmapp_display_clock_config rc=%d\n",
                __func__, ret);
        return ret;
    }

//SW2-6-MM-JH-Current_Leakage-00+
    /* pull down vsync and hsync to prevent from current leakage */
    gpio_tlmm_config( GPIO_CFG( 92, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_vsync */
    gpio_tlmm_config( GPIO_CFG( 93, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  /* lcdc_hsync */
//SW2-6-MM-JH-Current_Leakage-00-

    return 0;
}

//SW2-6-MM-JH-PWM-00+
/* M = 1, N = 150 */
uint16 PWM[11] = { 0,  //Backlight off,  0%
                   3,  //Min level,   2.00%,  10 cd/m^2
                  18,  //            12.03%,  64 cd/m^2
                  33,  //            22.02%, 118 cd/m^2
                  49,  //            32.70%, 172 cd/m^2
                  65,  //            43.36%, 226 cd/m^2
                  81,  //            54.02%, 280 cd/m^2
                  98,  //            65.38%, 334 cd/m^2
                 115,  //            76.67%, 388 cd/m^2
                 132,  //            83.30%, 442 cd/m^2
                 150   //Max Level, 100.00%, 496 cd/m^2
};
//SW2-6-MM-JH-PWM-00-

static void lcdc_set_backlight(struct msm_fb_data_type *mfd)
{
    int bl_level;
//    int ret = -EPERM;

    bl_level = mfd->bl_level;
    printk(KERN_INFO "[DISPLAY] %s: level = %d / %d\n", __func__, bl_level, mfd->panel_info.bl_max);

//    ret = gpio_tlmm_config(GPIO_CFG(GPIO_LCM_PWM, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
//    if (ret)
//        printk(KERN_INFO "[DISPLAY] %s: gpio_tlmm_config: %d failed...\n", __func__, GPIO_LCM_PWM);

//        mdelay(5);

    if (bl_level == 0) {
        if (display_backlight_on == TRUE) {
//SW2-6-MM-JH-PWM-00+
            // GP_MD_REG (CLK_MD)
            writel( ((mfd->panel_info.bl_regs.gp_clk_m & 0x01ff) << 16) | 0xffff, MSM_CLK_CTL_BASE + 0x58);

            // GP_NS_REG (CLK_NS)
            writel( (~(mfd->panel_info.bl_regs.gp_clk_n - mfd->panel_info.bl_regs.gp_clk_m) << 16) | 0xb58, MSM_CLK_CTL_BASE + 0x5c);
//SW2-6-MM-JH-PWM-00-

            clk_disable(gp_clk);
            display_backlight_on = FALSE;
        }
    } else {
        if (display_backlight_on == FALSE) {
            clk_enable(gp_clk);
            display_backlight_on = TRUE;
        }

//SW2-6-MM-JH-PWM-00+
        // M/N:D counter, M=1 for modulo-n counter
        // TCXO(SYS_CLK)=19.2MHz, PreDivSel=Div-4, N=150, 32kHz=19.2MHz/4/150
        // D = duty_cycle x N, 2D = duty_cycle x 2N

        mfd->panel_info.bl_regs.gp_clk_d = PWM[bl_level];

        // GP_MD_REG (CLK_MD)
        writel( ((mfd->panel_info.bl_regs.gp_clk_m & 0x01ff) << 16) | (~(mfd->panel_info.bl_regs.gp_clk_d * 2) & 0xffff), MSM_CLK_CTL_BASE + 0x58);

        // GP_NS_REG (CLK_NS)
        writel( (~(mfd->panel_info.bl_regs.gp_clk_n - mfd->panel_info.bl_regs.gp_clk_m) << 16) | 0xb58, MSM_CLK_CTL_BASE + 0x5c);
//SW2-6-MM-JH-PWM-00-
    }
}

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
//SW2-6-MM-JH-InitGPIO_SPI-00+
    if ((fih_get_product_phase() >= Product_PR2) || (fih_get_product_id() != Product_SF8)) {
        spi_sclk = 45;  /* clock */
        spi_cs   = 48;  /* chip select */
        spi_mosi = 47;  /* master output */
    } else {
        // SF8_PR1
        spi_sclk = 45;  /* clock */
        spi_cs   = 46;  /* chip select */
        spi_mosi = 47;  /* master output */
    }
//SW2-6-MM-JH-InitGPIO_SPI-00-

    gp_clk = clk_get(NULL, "gp_clk");
    if (IS_ERR(gp_clk))
    {
        printk(KERN_ERR "[DISPLAY] %s: could not get gp_clk.\n", __func__);
        gp_clk = NULL;
    }

    vreg_lvsw1 = vreg_get(NULL, "lvsw1");
    if (IS_ERR(vreg_lvsw1)) {
        pr_err("%s: lvsw1 vreg get failed (%ld)\n", __func__, PTR_ERR(vreg_lvsw1));
    }

    vreg_gp6   = vreg_get(NULL, "gp6");
    if (IS_ERR(vreg_gp6)) {
        pr_err("%s: gp6 vreg get failed (%ld)\n", __func__, PTR_ERR(vreg_gp6));
    }

    vreg_pull_down_switch(vreg_gp6, 1);

    msm_fb_add_device(pdev);

    return 0;
}

static struct platform_driver this_driver = {
    .probe  = lcdc_panel_probe,
    .driver = {
        .name   = "lcdc_sony_wvga",
    },
};

static struct msm_fb_panel_data lcdc_panel_data = {
    .on = lcdc_panel_on,
    .off = lcdc_panel_off,
    .set_backlight = lcdc_set_backlight,
};

static struct platform_device this_device = {
    .name   = "lcdc_sony_wvga",
    .id     = 1,
    .dev    = {
        .platform_data = &lcdc_panel_data,
    }
};

static int __init lcdc_sony_panel_init(void)
{
    int ret;
    struct msm_panel_info *pinfo;

//SW2-6-MM-JH-Ortustech_Panel-00+
    if ( gpio_get_value(GPIO_LCM_ID) == 0) {
        printk(KERN_INFO "Panel: Sony WVGA\n");
        panel = PANEL_SONY;
    } else {
        printk(KERN_INFO "Panel: Ortustech WVGA\n");
        panel = PANEL_ORTUSTECH;
    }

    if ( panel == PANEL_ORTUSTECH ) {
        this_driver.driver.name = "lcdc_ortustech_wvga";
        this_device.name = "lcdc_ortustech_wvga";
    }
//SW2-6-MM-JH-Ortustech_Panel-00-

    ret = platform_driver_register(&this_driver);
    if (ret)
        return ret;

    pinfo = &lcdc_panel_data.panel_info;
    pinfo->xres = 480;
    pinfo->yres = 800;
    pinfo->type = LCDC_PANEL;
    pinfo->pdest = DISPLAY_1;
    pinfo->wait_cycle = 0;
    pinfo->bpp = 18;
    pinfo->fb_num = 2;
	/*Div2-SW6-SC-Add_panel_size-00+{*/
	pinfo->width  = 48;  // 48.24mm
	pinfo->height = 80;  // 80.4 mm
	/*Div2-SW6-SC-Add_panel_size-00+}*/

//    pinfo->clk_rate = 32768000;
//    pinfo->clk_rate = 30720000;
    pinfo->clk_rate = 24576000;

//SW2-6-MM-JH-Ortustech_Panel-00+
    if ( panel == PANEL_SONY ) {
        pinfo->lcdc.h_back_porch  = 14;
        pinfo->lcdc.h_front_porch = 14;
        pinfo->lcdc.h_pulse_width =  4;
        pinfo->lcdc.v_back_porch  = 10;
        pinfo->lcdc.v_front_porch = 10;
        pinfo->lcdc.v_pulse_width =  2;
    } else {
        pinfo->lcdc.h_back_porch  = 10;
        pinfo->lcdc.h_front_porch =  8;
        pinfo->lcdc.h_pulse_width = 10;
        pinfo->lcdc.v_back_porch  =  2;
        pinfo->lcdc.v_front_porch =  2;
        pinfo->lcdc.v_pulse_width =  2;
    }
//SW2-6-MM-JH-Ortustech_Panel-00-

    pinfo->lcdc.border_clr = 0;     /* blk */
    pinfo->lcdc.underflow_clr = 0xff;  /* blue */
    pinfo->lcdc.hsync_skew = 0;

//SW2-6-MM-JH-PWM-00+
    pinfo->bl_max = 10;
    pinfo->bl_min = 1;
    pinfo->bl_regs.gp_clk_m = 1;
    pinfo->bl_regs.gp_clk_n = 150;
//SW2-6-MM-JH-PWM-00-

//SW2-6-MM-JH-Backlight_PWM-01+
    pinfo->bl_regs.bl_type = BL_TYPE_GP_CLK;
//SW2-6-MM-JH-Backlight_PWM-01-

    ret = platform_device_register(&this_device);
    if (ret) {
        printk(KERN_ERR "%s not able to register the device\n", __func__);
        goto fail_driver;
    }

    return ret;

fail_driver:
    platform_driver_unregister(&this_driver);
    return ret;
}

device_initcall(lcdc_sony_panel_init);
