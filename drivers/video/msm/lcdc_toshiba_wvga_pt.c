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
#ifdef CONFIG_SPI_QSD
#include <linux/spi/spi.h>
#endif
#include <mach/gpio.h>
#include <mach/pmic.h>
#include <mach/vreg.h> // FIHTDC, Ming, LCM
#include <mach/msm_iomap.h> // FIHTDC-Div2-SW2-BSP, Ming, LCM
#include <linux/clk.h>    // FIHTDC-Div2-SW2-BSP, Ming, LCM
#include "msm_fb.h"
#include "../../../arch/arm/mach-msm/smd_private.h" // FIHTDC-Div2-SW2-BSP, Ming, HWID

#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
#include "mddihosti.h"
#endif
#ifdef CONFIG_SPI_QSD
#define LCDC_TOSHIBA_SPI_DEVICE_NAME "lcdc_toshiba_ltm030dd40"
static struct spi_device *lcdc_toshiba_spi_client;
#else
static int spi_cs;
static int spi_sclk;
static int spi_mosi;
static int spi_miso;
#endif
struct toshiba_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

/* FIHTDC, Div2-SW2-BSP, Ming { */
// Already done in APPSBOOT.
//static struct toshiba_state_type toshiba_state = { .disp_initialized = FALSE, .display_on = TRUE, .disp_powered_up = TRUE }; ///{ 0 };
static struct toshiba_state_type toshiba_state = {0};
/* } FIHTDC, Div2-SW2-BSP, Ming */

static struct msm_panel_common_pdata *lcdc_toshiba_pdata;

static struct clk *gp_clk;  // FIHTDC-Div2-SW2-BSP, Ming, LCM
static boolean display_backlight_on = FALSE;  // FIHTDC-Div2-SW2-BSP, Ming
//Div2-SW2-BSP,JoeHsu,+++
static int panel_type =	0;//0=Toshiba Panel,1=Truly Panel
static int panel_read_proc(char *page, char **start, off_t off,
				int count, int *eof, void *data);
//Div2-SW2-BSP,JoeHsu,---				

/* SW5-1-MM-KW-Backlight_PWM-00+ { */
/* M = 1, N = 150 */
uint16 PWM[11] = { 0,  //Backlight off,  0%
                   4,  //Min level,   0.03%,  10 cd/m^2
                  19,  //            12.69%,  48 cd/m^2
                  34,  //            22.69%,  86 cd/m^2
                  49,  //            32.69%, 124 cd/m^2
                  65,  //            43.33%, 162 cd/m^2
                  81,  //            54.10%, 200 cd/m^2
                  98,  //            65.38%, 238 cd/m^2
                 115,  //            76.79%, 276 cd/m^2
                 133,  //            83.78%, 314 cd/m^2
                 150   //Max Level, 100.00%, 352 cd/m^2
};
/* SW5-1-MM-KW-Backlight_PWM-00- } */

#ifndef CONFIG_SPI_QSD
static void toshiba_spi_write_byte(char dc, uint8 data)
{
	uint32 bit;
	int bnum;

	gpio_set_value(spi_sclk, 0); /* clk low */
	/* dc: 0 for command, 1 for parameter */
	gpio_set_value(spi_mosi, dc);
	udelay(1);	/* at least 20 ns */
	gpio_set_value(spi_sclk, 1); /* clk high */
	udelay(1);	/* at least 20 ns */
	bnum = 8;	/* 8 data bits */
	bit = 0x80;
	while (bnum) {
		gpio_set_value(spi_sclk, 0); /* clk low */
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
#endif

static int toshiba_spi_write(char cmd, uint32 data, int num)
{
	char *bp;
#ifdef CONFIG_SPI_QSD
	char                tx_buf[4];
	int                 rc, i;
	struct spi_message  m;
	struct spi_transfer t;
	uint32 final_data = 0;

	if (!lcdc_toshiba_spi_client) {
		printk(KERN_ERR "%s lcdc_toshiba_spi_client is NULL\n",
			__func__);
		return -EINVAL;
	}

	memset(&t, 0, sizeof t);
	t.tx_buf = tx_buf;
	spi_setup(lcdc_toshiba_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	/* command byte first */
	final_data |= cmd << 23;
	t.len = num + 2;
	if (t.len < 4)
		t.bits_per_word = 8 * t.len;
	/* followed by parameter bytes */
	if (num) {
		bp = (char *)&data;;
		bp += (num - 1);
		i = 1;
		while (num) {
			final_data |= 1 << (((4 - i) << 3) - i - 1);
			final_data |= *bp << (((4 - i - 1) << 3) - i - 1);
			num--;
			bp--;
			i++;
		}
	}

	bp = (char *)&final_data;
	for (i = 0; i < t.len; i++)
		tx_buf[i] = bp[3 - i];
	t.rx_buf = NULL;
	rc = spi_sync(lcdc_toshiba_spi_client, &m);
	if (rc)
		printk(KERN_ERR "spi_sync _write failed %d\n", rc);
	return rc;
#else
	gpio_set_value(spi_cs, 1);	/* cs high */

	/* command byte first */
	toshiba_spi_write_byte(0, cmd);

	/* followed by parameter bytes */
	if (num) {
		bp = (char *)&data;;
		bp += (num - 1);
		while (num) {
			toshiba_spi_write_byte(1, *bp);
			num--;
			bp--;
		}
	}

	gpio_set_value(spi_cs, 0);	/* cs low */
	udelay(1);
	return 0;
#endif
}

static int toshiba_spi_read_bytes(char cmd, uint32 *data, int num)
{
#ifdef CONFIG_SPI_QSD
	char            tx_buf[5];
	char		    rx_buf[5];
	int                 rc;
	struct spi_message  m;
	struct spi_transfer t;

	if (!lcdc_toshiba_spi_client) {
		printk(KERN_ERR "%s lcdc_toshiba_spi_client is NULL\n",
			 __func__);
		return -EINVAL;
	}

	memset(&t, 0, sizeof t);
	t.tx_buf = tx_buf;
	t.rx_buf = rx_buf;
	spi_setup(lcdc_toshiba_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	/* command byte first */
	tx_buf[0] = 0 | ((cmd >> 1) & 0x7f);
	tx_buf[1] = (cmd & 0x01) << 7;
	tx_buf[2] = 0;
	tx_buf[3] = 0;
	tx_buf[4] = 0;

	t.len = 5;

	rc = spi_sync(lcdc_toshiba_spi_client, &m);
	*data = 0;
	*data = ((rx_buf[1] & 0x1f) << 19) | (rx_buf[2] << 11) |
		(rx_buf[3] << 3) | ((rx_buf[4] & 0xe0) >> 5);
	if (rc)
		printk(KERN_ERR "spi_sync _read failed %d\n", rc);
	return rc;
#else
	uint32 dbit, bits;
	int bnum;

	gpio_set_value(spi_cs, 1);	/* cs high */

	/* command byte first */
	toshiba_spi_write_byte(0, cmd);

	if (num > 1) {
		/* extra dc bit */
		gpio_set_value(spi_sclk, 0); /* clk low */
		udelay(1);
		dbit = gpio_get_value(spi_miso);/* dc bit */
		udelay(1);
		gpio_set_value(spi_sclk, 1); /* clk high */
	}

	/* followed by data bytes */
	bnum = num * 8;	/* number of bits */
	bits = 0;
	while (bnum) {
		bits <<= 1;
		gpio_set_value(spi_sclk, 0); /* clk low */
		udelay(1);
		dbit = gpio_get_value(spi_miso);
		udelay(1);
		gpio_set_value(spi_sclk, 1); /* clk high */
		bits |= dbit;
		bnum--;
	}

	*data = bits;

	udelay(1);
	gpio_set_value(spi_cs, 0);	/* cs low */
	udelay(1);
	return 0;
#endif
}

//Div2-SW2-BSP,JoeHsu ,+++

static int truly_spi_write_cmd(unsigned int cmd)
{
	unsigned short i, byMask=0x20;

	gpio_set_value(spi_cs, 0);	/* cs low */
	gpio_set_value(spi_sclk, 0); /* clk low */
	gpio_set_value(spi_mosi, 0);
  
  //udelay(1);

//the 1st byte
  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x80)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);
   byMask = cmd;

  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x8000)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1); 
   
//the 2nd byte   
   byMask=0x00; 
  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x80)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   
   //udelay(1);
    byMask = cmd << 8;
  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x8000)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }    
	gpio_set_value(spi_cs, 1);    

	return 0;

}

static int truly_spi_write_data(unsigned int data)
{
	unsigned short i, byMask=0x40;

	gpio_set_value(spi_cs, 0);	/* cs low */
	gpio_set_value(spi_sclk, 0); /* clk low */
	gpio_set_value(spi_mosi, 0);
  
  //udelay(1);

  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x80)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);
   byMask = data;

  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x80)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);
   
	gpio_set_value(spi_cs, 1);    

	return 0;
}

static int truly_spi_write(unsigned int cmd,unsigned int data)
{
  truly_spi_write_cmd(cmd);
  truly_spi_write_data(data); 	

	return 0;
}

static int truly_spi_read_cmd(unsigned int cmd)
{
	unsigned short i, byMask=0x20;

	gpio_set_value(spi_cs, 0);	/* cs low */
	gpio_set_value(spi_sclk, 0); /* clk low */
	gpio_set_value(spi_mosi, 0);
  
  //udelay(1);
//the 1st byte
  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x80)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);
   byMask = cmd;

  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x8000)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);
   
//the 2nd byte
   byMask = 0x00; 
  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x80)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);
   byMask = cmd << 8;

  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x8000)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);
   
	gpio_set_value(spi_cs, 1);    

	return 0;
}

static int truly_spi_read_data(void)
{
	unsigned short i, byMask=0xC0,data=0;

	gpio_set_value(spi_cs, 0);	/* cs low */
	gpio_set_value(spi_sclk, 0); /* clk low */
	gpio_set_value(spi_mosi, 0);
  
  //udelay(1);

  for(i=0 ; i<8 ; i++) {
  	  gpio_set_value(spi_sclk, 0);
  	  if(byMask & 0x80)
  	  	 gpio_set_value(spi_mosi, 1);
  	   else
  	   	 gpio_set_value(spi_mosi, 0);
 
     //udelay(1);
     gpio_set_value(spi_sclk, 1);  	
     //udelay(1);
     byMask<<=1; 	  	
   }
   //udelay(1);

   for(i=0 ; i<8 ; i++)
    {
        data <<= 1;
        gpio_set_value(spi_sclk, 0);
        data |= gpio_get_value(spi_miso);
        //udelay(1);  
        gpio_set_value(spi_sclk, 1);
        //udelay(1);       
    }
   
	gpio_set_value(spi_cs, 1);    

  //printk(KERN_INFO "truly_spi_read_data =%x\n", data);

	return data;
}
//Div2-SW2-BSP,JoeHsu ,---


#ifndef CONFIG_SPI_QSD
static void spi_pin_assign(void)
{
	/* Setting the Default GPIO's */
	/* FIHTDC, Div2-SW2-BSP, Ming, SPI for PR1  */
   	if (fih_get_product_phase()>=Product_PR2||(fih_get_product_id()!=Product_FB0 && fih_get_product_id()!=Product_FD1)) { 
		spi_sclk = *(lcdc_toshiba_pdata->gpio_num);
	spi_cs   = *(lcdc_toshiba_pdata->gpio_num + 1);
		spi_mosi  = *(lcdc_toshiba_pdata->gpio_num + 2);
		spi_miso  = *(lcdc_toshiba_pdata->gpio_num + 3);
    } else { // FB0_PR1 or FD1_PR1, different spi configuration
        spi_sclk = *(lcdc_toshiba_pdata->gpio_num);
    	spi_cs   = *(lcdc_toshiba_pdata->gpio_num + 1);
	 spi_mosi  = *(lcdc_toshiba_pdata->gpio_num + 3);
	 spi_miso  = *(lcdc_toshiba_pdata->gpio_num + 2);
	}
	/* FIHTDC, Div2-SW2-BSP, Ming, SPI for PR1  */
}
#endif

static void toshiba_disp_powerup(void)
{
	if (!toshiba_state.disp_powered_up && !toshiba_state.display_on) {
		/* Reset the hardware first */
		/* Include DAC power up implementation here */
	      toshiba_state.disp_powered_up = TRUE;
	}
}

/* FIHTDC-Div5-SW2-BSP, Chun {*/ 
int disp_on_process=0;
int alarm_flag=0;
extern int suspend_resume_flag;
extern int permit;
/* } FIHTDC-Div5-SW2-BSP, Chun */ 


static void toshiba_disp_on(void)
{
	uint32	data;
	
	if (permit==1)
    	disp_on_process=1;
    permit=1;	
#ifndef CONFIG_SPI_QSD
	gpio_set_value(spi_cs, 0);	/* low */
	gpio_set_value(spi_sclk, 1);	/* high */
	gpio_set_value(spi_mosi, 0);
	gpio_set_value(spi_miso, 0);
#endif

	if (toshiba_state.disp_powered_up && !toshiba_state.display_on) {
	    
#if 1  
        // LT041MDM6x00 Timing sequense
	    toshiba_spi_write(0, 0, 0);
		mdelay(7);
		toshiba_spi_write(0, 0, 0);
		mdelay(7);
		toshiba_spi_write(0, 0, 0);
		mdelay(7);
	
		toshiba_spi_write(0xba, 0x11, 1);

        toshiba_spi_write(0x2b, 0x0000018F, 4);
        mdelay(1);
		toshiba_spi_write(0x36, 0x00, 1);
		toshiba_spi_write(0x3a, 0x60, 1);
		
		toshiba_spi_write(0xb1, 0x5d, 1);
		toshiba_spi_write(0xb2, 0x33, 1);
		
		toshiba_spi_write(0xb3, 0x22, 1);
		toshiba_spi_write(0xb4, 0x02, 1);

		toshiba_spi_write(0xb5, 0x23, 1); 
/* FIHTDC-SW5-MULTIMEDIA, Chance { */
#ifdef CONFIG_FIH_PROJECT_SF4Y6
                toshiba_spi_write(0xb5, 0x1C, 1);
#endif
/* } FIHTDC-SW5-MULTIMEDIA, Chance */
		toshiba_spi_write(0xb6, 0x2e, 1);
		
		toshiba_spi_write(0xb7, 0x03, 1);
		toshiba_spi_write(0xb9, 0x24, 1);
		
		toshiba_spi_write(0xbd, 0xa1, 1);
		toshiba_spi_write(0xbe, 0x00, 1);

		toshiba_spi_write(0xbb, 0x00, 1);
		toshiba_spi_write(0xbf, 0x01, 1);

		toshiba_spi_write(0xc0, 0x11, 1);
		toshiba_spi_write(0xc1, 0x11, 1);
		
		toshiba_spi_write(0xc2, 0x11, 1);

		toshiba_spi_write(0xc3, 0x3232, 2);

		toshiba_spi_write(0xc4, 0x3232, 2);

		toshiba_spi_write(0xc5, 0x3232, 2);

		toshiba_spi_write(0xc6, 0x3232, 2);

		toshiba_spi_write(0xc7, 0x6445, 2);

		toshiba_spi_write(0xc8, 0x44, 1);
		toshiba_spi_write(0xc9, 0x52, 1);

		toshiba_spi_write(0xca, 0x00, 1);

		toshiba_spi_write(0xec, 0x0200, 2);

		toshiba_spi_write(0xcf, 0x01, 1);

		toshiba_spi_write(0xd0, 0x1004, 2);

		toshiba_spi_write(0xd1, 0x01, 1);

		toshiba_spi_write(0xd2, 0x001a, 2);
		
		toshiba_spi_write(0xd3, 0x001a, 2);

		toshiba_spi_write(0xd4, 0x207a, 2);

		toshiba_spi_write(0xd5, 0x18, 1);

		toshiba_spi_write(0xe2, 0x00, 1);
		toshiba_spi_write(0xe3, 0x36, 1);

		toshiba_spi_write(0xe4, 0x0003, 2);

		toshiba_spi_write(0xe5, 0x0003, 2); 

		toshiba_spi_write(0xe6, 0x04, 1);

		toshiba_spi_write(0xe7, 0x030c, 2);
		
		toshiba_spi_write(0xe8, 0x03, 1);
		toshiba_spi_write(0xe9, 0x20, 1);

		toshiba_spi_write(0xea, 0x0404, 2); 
		
		toshiba_spi_write(0xef, 0x3200, 2);
		mdelay(32);
		toshiba_spi_write(0xbc, 0x80, 1);	/* wvga pass through */
		toshiba_spi_write(0x3b, 0x00, 1);
		
		toshiba_spi_write(0xb9, 0x24, 1); 
		toshiba_spi_write(0xb0, 0x16, 1);
		
		toshiba_spi_write(0xb8, 0xfff5, 2);

		toshiba_spi_write(0x11, 0, 0);
		mdelay(5);
		toshiba_spi_write(0x29, 0, 0);
		mdelay(5);
	   
#else  
        // Qualcomm Release
		toshiba_spi_write(0, 0, 0);
		mdelay(7);
		toshiba_spi_write(0, 0, 0);
		mdelay(7);
		toshiba_spi_write(0, 0, 0);
		mdelay(7);
		toshiba_spi_write(0xba, 0x11, 1);
		toshiba_spi_write(0x36, 0x00, 1);
		mdelay(1);
		toshiba_spi_write(0x3a, 0x60, 1);
		toshiba_spi_write(0xb1, 0x5d, 1);
		mdelay(1);
		toshiba_spi_write(0xb2, 0x33, 1);
		toshiba_spi_write(0xb3, 0x22, 1);
		mdelay(1);
		toshiba_spi_write(0xb4, 0x02, 1);
		toshiba_spi_write(0xb5, 0x1e, 1); /* vcs -- adjust brightness */
		mdelay(1);
		toshiba_spi_write(0xb6, 0x27, 1);
		toshiba_spi_write(0xb7, 0x03, 1);
		mdelay(1);
		toshiba_spi_write(0xb9, 0x24, 1);
		toshiba_spi_write(0xbd, 0xa1, 1);
		mdelay(1);
		toshiba_spi_write(0xbb, 0x00, 1);
		toshiba_spi_write(0xbf, 0x01, 1);
		mdelay(1);
		toshiba_spi_write(0xbe, 0x00, 1);
		toshiba_spi_write(0xc0, 0x11, 1);
		mdelay(1);
		toshiba_spi_write(0xc1, 0x11, 1);
		toshiba_spi_write(0xc2, 0x11, 1);
		mdelay(1);
		toshiba_spi_write(0xc3, 0x3232, 2);
		mdelay(1);
		toshiba_spi_write(0xc4, 0x3232, 2);
		mdelay(1);
		toshiba_spi_write(0xc5, 0x3232, 2);
		mdelay(1);
		toshiba_spi_write(0xc6, 0x3232, 2);
		mdelay(1);
		toshiba_spi_write(0xc7, 0x6445, 2);
		mdelay(1);
		toshiba_spi_write(0xc8, 0x44, 1);
		toshiba_spi_write(0xc9, 0x52, 1);
		mdelay(1);
		toshiba_spi_write(0xca, 0x00, 1);
		mdelay(1);
		toshiba_spi_write(0xec, 0x02a4, 2);	/* 0x02a4 */
		mdelay(1);
		toshiba_spi_write(0xcf, 0x01, 1);
		mdelay(1);
		toshiba_spi_write(0xd0, 0xc003, 2);	/* c003 */
		mdelay(1);
		toshiba_spi_write(0xd1, 0x01, 1);
		mdelay(1);
		toshiba_spi_write(0xd2, 0x0028, 2);
		mdelay(1);
		toshiba_spi_write(0xd3, 0x0028, 2);
		mdelay(1);
		toshiba_spi_write(0xd4, 0x26a4, 2);
		mdelay(1);
		toshiba_spi_write(0xd5, 0x20, 1);
		mdelay(1);
		toshiba_spi_write(0xef, 0x3200, 2);
		mdelay(32);
		toshiba_spi_write(0xbc, 0x80, 1);	/* wvga pass through */
		toshiba_spi_write(0x3b, 0x00, 1);
		mdelay(1);
		toshiba_spi_write(0xb0, 0x16, 1);
		mdelay(1);
		toshiba_spi_write(0xb8, 0xfff5, 2);
		mdelay(1);
		toshiba_spi_write(0x11, 0, 0);
		mdelay(5);
		toshiba_spi_write(0x29, 0, 0);
		mdelay(5);
			
#endif // 		
		
		toshiba_state.display_on = TRUE;
	}

	data = 0;
	toshiba_spi_read_bytes(0x04, &data, 3);
	/* FIHTDC-Div5-SW2-BSP, Chun {*/ 
	disp_on_process=0;
	suspend_resume_flag=0;
	alarm_flag=0;
	/* } FIHTDC-Div5-SW2-BSP, Chun */ 

//DIV5-MM-KW-pull down SPI pin for SF4Y6-00+ {
#ifdef CONFIG_FIH_PROJECT_SF4Y6
	gpio_set_value(spi_sclk, 0); /* clk low */
	gpio_set_value(spi_cs, 0);	 /* cs low */
	gpio_set_value(spi_mosi, 0); /* ds low */
#endif
// }DIV5-MM-KW-pull down SPI pin for SF4Y6-00-

	printk(KERN_INFO "toshiba_disp_on: id=%x\n", data);

}

//Div2-SW2-BSP,JoeHsu ,+++
//Initial  & 2.2 Gamma
static void truly_disp_on(void)
{
	uint32	data;
	
	if (permit==1)
    	disp_on_process=1;
    permit=1;	
#ifndef CONFIG_SPI_QSD
	gpio_set_value(spi_cs, 1);	/* hi */
	gpio_set_value(spi_sclk, 1);	/* high */
	gpio_set_value(spi_mosi, 0);
	gpio_set_value(spi_miso, 0);
#endif

	if (toshiba_state.disp_powered_up && !toshiba_state.display_on) {
   
#if 1
    truly_spi_write(0, 0);
		mdelay(7);
    truly_spi_write(0x0000,0x00);	
    truly_spi_write(0xF000,0x55);
    truly_spi_write(0xF001,0xAA);
    truly_spi_write(0xF002,0x52);
    truly_spi_write(0xF003,0x08);
    truly_spi_write(0xF004,0x01);
    //VGMP/VGMN/VCOM SETING
    truly_spi_write(0xBC00,0x00);
    truly_spi_write(0xBC01,0xA0);
    truly_spi_write(0xBC02,0x00);
    truly_spi_write(0xBD00,0x00);
    truly_spi_write(0xBD01,0xA0);
    truly_spi_write(0xBD02,0x00);
    truly_spi_write(0xBE01,0x67);
    //GAMMA SETING  RED  
    truly_spi_write(0xD100,0x00);
    truly_spi_write(0xD101,0x3F);
    truly_spi_write(0xD102,0x00);
    truly_spi_write(0xD103,0x4E);
    truly_spi_write(0xD104,0x00);
    truly_spi_write(0xD105,0x66);
    truly_spi_write(0xD106,0x00);
    truly_spi_write(0xD107,0x7A);
    truly_spi_write(0xD108,0x00);
    truly_spi_write(0xD109,0x8B);
    truly_spi_write(0xD10A,0x00);
    truly_spi_write(0xD10B,0xA8);
    truly_spi_write(0xD10C,0x00);
    truly_spi_write(0xD10D,0xC0);
    truly_spi_write(0xD10E,0x00);
    truly_spi_write(0xD10F,0xE6);
    truly_spi_write(0xD110,0x01);
    truly_spi_write(0xD111,0x04);
    truly_spi_write(0xD112,0x01);
    truly_spi_write(0xD113,0x32);
    truly_spi_write(0xD114,0x01);
    truly_spi_write(0xD115,0x55);
    truly_spi_write(0xD116,0x01);
    truly_spi_write(0xD117,0x8A);
    truly_spi_write(0xD118,0x01);
    truly_spi_write(0xD119,0xB4);
    truly_spi_write(0xD11A,0x01);
    truly_spi_write(0xD11B,0xB5);
    truly_spi_write(0xD11C,0x01);
    truly_spi_write(0xD11D,0xDA);
    truly_spi_write(0xD11E,0x02);
    truly_spi_write(0xD11F,0x01);
    truly_spi_write(0xD120,0x02);
    truly_spi_write(0xD121,0x17);
    truly_spi_write(0xD122,0x02);
    truly_spi_write(0xD123,0x3A);
    truly_spi_write(0xD124,0x02);
    truly_spi_write(0xD125,0x58);
    truly_spi_write(0xD126,0x02);
    truly_spi_write(0xD127,0x8A);
    truly_spi_write(0xD128,0x02);
    truly_spi_write(0xD129,0xB5);
    truly_spi_write(0xD12A,0x02);
    truly_spi_write(0xD12B,0xF6);
    truly_spi_write(0xD12C,0x03);
    truly_spi_write(0xD12D,0x28);
    truly_spi_write(0xD12E,0x03);
    truly_spi_write(0xD12F,0x6C);
    truly_spi_write(0xD130,0x03);
    truly_spi_write(0xD131,0xC3);
    truly_spi_write(0xD132,0x03);
    truly_spi_write(0xD133,0xEB);
    //GAMMA SETING GREEN  
    truly_spi_write(0xD200,0x00);
    truly_spi_write(0xD201,0x3F);
    truly_spi_write(0xD202,0x00);
    truly_spi_write(0xD203,0x4E);
    truly_spi_write(0xD204,0x00);
    truly_spi_write(0xD205,0x66);
    truly_spi_write(0xD206,0x00);
    truly_spi_write(0xD207,0x7A);
    truly_spi_write(0xD208,0x00);
    truly_spi_write(0xD209,0x8B);
    truly_spi_write(0xD20A,0x00);
    truly_spi_write(0xD20B,0xA8);
    truly_spi_write(0xD20C,0x00);
    truly_spi_write(0xD20D,0xC0);
    truly_spi_write(0xD20E,0x00);
    truly_spi_write(0xD20F,0xE6);
    truly_spi_write(0xD210,0x01);
    truly_spi_write(0xD211,0x04);
    truly_spi_write(0xD212,0x01);
    truly_spi_write(0xD213,0x32);
    truly_spi_write(0xD214,0x01);
    truly_spi_write(0xD215,0x55);
    truly_spi_write(0xD216,0x01);
    truly_spi_write(0xD217,0x8A);
    truly_spi_write(0xD218,0x01);
    truly_spi_write(0xD219,0xB4);
    truly_spi_write(0xD21A,0x01);
    truly_spi_write(0xD21B,0xB5);
    truly_spi_write(0xD21C,0x01);
    truly_spi_write(0xD21D,0xDA);
    truly_spi_write(0xD21E,0x02);
    truly_spi_write(0xD21F,0x01);
    truly_spi_write(0xD220,0x02);
    truly_spi_write(0xD221,0x17);
    truly_spi_write(0xD222,0x02);
    truly_spi_write(0xD223,0x3A);
    truly_spi_write(0xD224,0x02);
    truly_spi_write(0xD225,0x58);
    truly_spi_write(0xD226,0x02);
    truly_spi_write(0xD227,0x8A);
    truly_spi_write(0xD228,0x02);
    truly_spi_write(0xD229,0xB5);
    truly_spi_write(0xD22A,0x02);
    truly_spi_write(0xD22B,0xF6);
    truly_spi_write(0xD22C,0x03);
    truly_spi_write(0xD22D,0x28);
    truly_spi_write(0xD22E,0x03);
    truly_spi_write(0xD22F,0x6C);
    truly_spi_write(0xD230,0x03);
    truly_spi_write(0xD231,0xC3);
    truly_spi_write(0xD232,0x03);
    truly_spi_write(0xD233,0xEB);
    //GAMMA SETING BLUE  
    truly_spi_write(0xD300,0x00);
    truly_spi_write(0xD301,0x3F);
    truly_spi_write(0xD302,0x00);
    truly_spi_write(0xD303,0x4E);
    truly_spi_write(0xD304,0x00);
    truly_spi_write(0xD305,0x66);
    truly_spi_write(0xD306,0x00);
    truly_spi_write(0xD307,0x7A);
    truly_spi_write(0xD308,0x00);
    truly_spi_write(0xD309,0x8B);
    truly_spi_write(0xD30A,0x00);
    truly_spi_write(0xD30B,0xA8);
    truly_spi_write(0xD30C,0x00);
    truly_spi_write(0xD30D,0xC0);
    truly_spi_write(0xD30E,0x00);
    truly_spi_write(0xD30F,0xE6);
    truly_spi_write(0xD310,0x01);
    truly_spi_write(0xD311,0x04);
    truly_spi_write(0xD312,0x01);
    truly_spi_write(0xD313,0x32);
    truly_spi_write(0xD314,0x01);
    truly_spi_write(0xD315,0x55);
    truly_spi_write(0xD316,0x01);
    truly_spi_write(0xD317,0x8A);
    truly_spi_write(0xD318,0x01);
    truly_spi_write(0xD319,0xB4);
    truly_spi_write(0xD31A,0x01);
    truly_spi_write(0xD31B,0xB5);
    truly_spi_write(0xD31C,0x01);
    truly_spi_write(0xD31D,0xDA);
    truly_spi_write(0xD31E,0x02);
    truly_spi_write(0xD31F,0x01);
    truly_spi_write(0xD320,0x02);
    truly_spi_write(0xD321,0x17);
    truly_spi_write(0xD322,0x02);
    truly_spi_write(0xD323,0x3A);
    truly_spi_write(0xD324,0x02);
    truly_spi_write(0xD325,0x58);
    truly_spi_write(0xD326,0x02);
    truly_spi_write(0xD327,0x8A);
    truly_spi_write(0xD328,0x02);
    truly_spi_write(0xD329,0xB5);
    truly_spi_write(0xD32A,0x02);
    truly_spi_write(0xD32B,0xF6);
    truly_spi_write(0xD32C,0x03);
    truly_spi_write(0xD32D,0x28);
    truly_spi_write(0xD32E,0x03);
    truly_spi_write(0xD32F,0x6C);
    truly_spi_write(0xD330,0x03);
    truly_spi_write(0xD331,0xC3);
    truly_spi_write(0xD332,0x03);
    truly_spi_write(0xD333,0xEB);
    //GAMMA SETING RED  
    truly_spi_write(0xD400,0x00);
    truly_spi_write(0xD401,0x3F);
    truly_spi_write(0xD402,0x00);
    truly_spi_write(0xD403,0x4E);
    truly_spi_write(0xD404,0x00);
    truly_spi_write(0xD405,0x66);
    truly_spi_write(0xD406,0x00);
    truly_spi_write(0xD407,0x7A);
    truly_spi_write(0xD408,0x00);
    truly_spi_write(0xD409,0x8B);
    truly_spi_write(0xD40A,0x00);
    truly_spi_write(0xD40B,0xA8);
    truly_spi_write(0xD40C,0x00);
    truly_spi_write(0xD40D,0xC0);
    truly_spi_write(0xD40E,0x00);
    truly_spi_write(0xD40F,0xE6);
    truly_spi_write(0xD410,0x01);
    truly_spi_write(0xD411,0x04);
    truly_spi_write(0xD412,0x01);
    truly_spi_write(0xD413,0x32);
    truly_spi_write(0xD414,0x01);
    truly_spi_write(0xD415,0x55);
    truly_spi_write(0xD416,0x01);
    truly_spi_write(0xD417,0x8A);
    truly_spi_write(0xD418,0x01);
    truly_spi_write(0xD419,0xB4);
    truly_spi_write(0xD41A,0x01);
    truly_spi_write(0xD41B,0xB5);
    truly_spi_write(0xD41C,0x01);
    truly_spi_write(0xD41D,0xDA);
    truly_spi_write(0xD41E,0x02);
    truly_spi_write(0xD41F,0x01);
    truly_spi_write(0xD420,0x02);
    truly_spi_write(0xD421,0x17);
    truly_spi_write(0xD422,0x02);
    truly_spi_write(0xD423,0x3A);
    truly_spi_write(0xD424,0x02);
    truly_spi_write(0xD425,0x58);
    truly_spi_write(0xD426,0x02);
    truly_spi_write(0xD427,0x8A);
    truly_spi_write(0xD428,0x02);
    truly_spi_write(0xD429,0xB5);
    truly_spi_write(0xD42A,0x02);
    truly_spi_write(0xD42B,0xF6);
    truly_spi_write(0xD42C,0x03);
    truly_spi_write(0xD42D,0x28);
    truly_spi_write(0xD42E,0x03);
    truly_spi_write(0xD42F,0x6C);
    truly_spi_write(0xD430,0x03);
    truly_spi_write(0xD431,0xC3);
    truly_spi_write(0xD432,0x03);
    truly_spi_write(0xD433,0xEB);
    //GAMMASETING GERREN  
    truly_spi_write(0xD500,0x00);
    truly_spi_write(0xD501,0x3F);
    truly_spi_write(0xD502,0x00);
    truly_spi_write(0xD503,0x4E);
    truly_spi_write(0xD504,0x00);
    truly_spi_write(0xD505,0x66);
    truly_spi_write(0xD506,0x00);
    truly_spi_write(0xD507,0x7A);
    truly_spi_write(0xD508,0x00);
    truly_spi_write(0xD509,0x8B);
    truly_spi_write(0xD50A,0x00);
    truly_spi_write(0xD50B,0xA8);
    truly_spi_write(0xD50C,0x00);
    truly_spi_write(0xD50D,0xC0);
    truly_spi_write(0xD50E,0x00);
    truly_spi_write(0xD50F,0xE6);
    truly_spi_write(0xD510,0x01);
    truly_spi_write(0xD511,0x04);
    truly_spi_write(0xD512,0x01);
    truly_spi_write(0xD513,0x32);
    truly_spi_write(0xD514,0x01);
    truly_spi_write(0xD515,0x55);
    truly_spi_write(0xD516,0x01);
    truly_spi_write(0xD517,0x8A);
    truly_spi_write(0xD518,0x01);
    truly_spi_write(0xD519,0xB4);
    truly_spi_write(0xD51A,0x01);
    truly_spi_write(0xD51B,0xB5);
    truly_spi_write(0xD51C,0x01);
    truly_spi_write(0xD51D,0xDA);
    truly_spi_write(0xD51E,0x02);
    truly_spi_write(0xD51F,0x01);
    truly_spi_write(0xD520,0x02);
    truly_spi_write(0xD521,0x17);
    truly_spi_write(0xD522,0x02);
    truly_spi_write(0xD523,0x3A);
    truly_spi_write(0xD524,0x02);
    truly_spi_write(0xD525,0x58);
    truly_spi_write(0xD526,0x02);
    truly_spi_write(0xD527,0x8A);
    truly_spi_write(0xD528,0x02);
    truly_spi_write(0xD529,0xB5);
    truly_spi_write(0xD52A,0x02);
    truly_spi_write(0xD52B,0xF6);
    truly_spi_write(0xD52C,0x03);
    truly_spi_write(0xD52D,0x28);
    truly_spi_write(0xD52E,0x03);
    truly_spi_write(0xD52F,0x6C);
    truly_spi_write(0xD530,0x03);
    truly_spi_write(0xD531,0xC3);
    truly_spi_write(0xD532,0x03);
    truly_spi_write(0xD533,0xEB);
    //GAMMA SETING BLUE 
    truly_spi_write(0xD600,0x00);
    truly_spi_write(0xD601,0x3F);
    truly_spi_write(0xD602,0x00);
    truly_spi_write(0xD603,0x4E);
    truly_spi_write(0xD604,0x00);
    truly_spi_write(0xD605,0x66);
    truly_spi_write(0xD606,0x00);
    truly_spi_write(0xD607,0x7A);
    truly_spi_write(0xD608,0x00);
    truly_spi_write(0xD609,0x8B);
    truly_spi_write(0xD60A,0x00);
    truly_spi_write(0xD60B,0xA8);
    truly_spi_write(0xD60C,0x00);
    truly_spi_write(0xD60D,0xC0);
    truly_spi_write(0xD60E,0x00);
    truly_spi_write(0xD60F,0xE6);
    truly_spi_write(0xD610,0x01);
    truly_spi_write(0xD611,0x04);
    truly_spi_write(0xD612,0x01);
    truly_spi_write(0xD613,0x32);
    truly_spi_write(0xD614,0x01);
    truly_spi_write(0xD615,0x55);
    truly_spi_write(0xD616,0x01);
    truly_spi_write(0xD617,0x8A);
    truly_spi_write(0xD618,0x01);
    truly_spi_write(0xD619,0xB4);
    truly_spi_write(0xD61A,0x01);
    truly_spi_write(0xD61B,0xB5);
    truly_spi_write(0xD61C,0x01);
    truly_spi_write(0xD61D,0xDA);
    truly_spi_write(0xD61E,0x02);
    truly_spi_write(0xD61F,0x01);
    truly_spi_write(0xD620,0x02);
    truly_spi_write(0xD621,0x17);
    truly_spi_write(0xD622,0x02);
    truly_spi_write(0xD623,0x3A);
    truly_spi_write(0xD624,0x02);
    truly_spi_write(0xD625,0x58);
    truly_spi_write(0xD626,0x02);
    truly_spi_write(0xD627,0x8A);
    truly_spi_write(0xD628,0x02);
    truly_spi_write(0xD629,0xB5);
    truly_spi_write(0xD62A,0x02);
    truly_spi_write(0xD62B,0xF6);
    truly_spi_write(0xD62C,0x03);
    truly_spi_write(0xD62D,0x28);
    truly_spi_write(0xD62E,0x03);
    truly_spi_write(0xD62F,0x6C);
    truly_spi_write(0xD630,0x03);
    truly_spi_write(0xD631,0xC3);
    truly_spi_write(0xD632,0x03);
    truly_spi_write(0xD633,0xEB);
    //AVDD VOLTAGE SETTING
    truly_spi_write(0xB000,0x03);
    truly_spi_write(0xB001,0x03);
    truly_spi_write(0xB002,0x03);
    //AVEE VOLTAGE SETTING9 
    truly_spi_write(0xB100,0x00);
    truly_spi_write(0xB101,0x00);
    truly_spi_write(0xB102,0x00);
    //VGLX VOLTAGE SETTING
    truly_spi_write(0xBA00,0x14);
    truly_spi_write(0xBA01,0x14);
    truly_spi_write(0xBA02,0x14);
    //BGH VOLTAGE SETTING
    truly_spi_write(0xB900,0x24);
    truly_spi_write(0xB901,0x24);
    truly_spi_write(0xB902,0x24);
    //ENABLE PAGE 0;
    truly_spi_write(0xF000,0x55);
    truly_spi_write(0xF001,0xAA);
    truly_spi_write(0xF002,0x52);
    truly_spi_write(0xF003,0x08);
    truly_spi_write(0xF004,0x00);
    //RAM KEEP 
    truly_spi_write(0xB100,0xCC);
    //Z-INVERSION 
    truly_spi_write(0xBC00,0x05);
    truly_spi_write(0xBC01,0x05);
    truly_spi_write(0xBC02,0x05);
    //SOURCE EQ
    truly_spi_write(0xB800,0x01);
    //Porch Adjust
    truly_spi_write(0xBD02,0x07);
    truly_spi_write(0xBD03,0x31);
    truly_spi_write(0xBE02,0x07);
    truly_spi_write(0xBE03,0x31);
    truly_spi_write(0xBF02,0x07);
    truly_spi_write(0xBF03,0x31);
    //ENABLE LV3
    truly_spi_write(0xFF00,0xAA);
    truly_spi_write(0xFF01,0x55);
    truly_spi_write(0xFF02,0x25);
    truly_spi_write(0xFF03,0x01);
    truly_spi_write(0xFF04,0x11);   
    truly_spi_write(0xF306,0x10);
    truly_spi_write(0xF408,0x00);
    //TE ON
    truly_spi_write(0x3500,0x00);
    //OTHER SET
    truly_spi_write(0x3600,0x00);
    truly_spi_write(0x3A00,0x60);

    //ENABLE PAGE 0
    truly_spi_write(0xF000,0x55);  
    truly_spi_write(0xF001,0xAA); 
    truly_spi_write(0xF002,0x52); 
    truly_spi_write(0xF003,0x08); 
    truly_spi_write(0xF004,0x00); 

    truly_spi_write(0xB400,0x10);  //enhance color         
    truly_spi_write(0xB000,0xA8);  //RGB mode2,falling edge 

    truly_spi_write(0x1100,0x00);  //start up
		mdelay(120);  
    truly_spi_write(0x2900,0x00);  //display on
		mdelay(5);
			
#endif //
		toshiba_state.display_on = TRUE;
	}

	data = 0;
  truly_spi_write_cmd(0x0000);
  truly_spi_write_data(0x00); 	
	truly_spi_read_cmd(0x0401);
	data = truly_spi_read_data();	
	/* FIHTDC-Div5-SW2-BSP, Chun {*/ 
	disp_on_process=0;
	suspend_resume_flag=0;
	alarm_flag=0;
	/* } FIHTDC-Div5-SW2-BSP, Chun */ 

//DIV5-MM-KW-pull down SPI pin for SF4Y6-00+ {
#ifdef CONFIG_FIH_PROJECT_SF4Y6
	gpio_set_value(spi_sclk, 0); /* clk low */
	gpio_set_value(spi_cs, 0);	 /* cs low */
	gpio_set_value(spi_mosi, 0); /* ds low */
#endif
// }DIV5-MM-KW-pull down SPI pin for SF4Y6-00-

	printk(KERN_INFO "truly_disp_on: id=%x\n", data);

}
//Div2-SW2-BSP,JoeHsu ,---

static int lcdc_toshiba_panel_on(struct platform_device *pdev)
{

    printk(KERN_INFO "[DISPLAY] %s: disp_initialized=%d, display_on=%d, disp_powered_up=%d.\n", 
                    __func__, (int)toshiba_state.disp_initialized, (int)toshiba_state.display_on, (int)toshiba_state.disp_powered_up);
	
	if (!toshiba_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */
		if (lcdc_toshiba_pdata->panel_config_gpio)
			lcdc_toshiba_pdata->panel_config_gpio(1);
		toshiba_disp_powerup();

		//Div2-SW2-BSP,JoeHsu
  	if (panel_type == 1) {
  		  //printk(KERN_INFO "Truly panel ...\n");
  		  truly_disp_on();
  	 }else {
  	 	  //printk(KERN_INFO "Toshiba panel ...\n");
		    toshiba_disp_on();
		  }
		toshiba_state.disp_initialized = TRUE;
	}
	return 0;
}
int panel_off_process=0;/* FIHTDC-Div5-SW2-BSP, Chun */ 

static int lcdc_toshiba_panel_off(struct platform_device *pdev)
{
    /* FIHTDC-Div2-SW2-BSP, Ming, LCM { */
		printk(KERN_INFO "[DISPLAY] %s: disp_initialized=%d, display_on=%d, disp_powered_up=%d.\n", 
                   __func__, (int)toshiba_state.disp_initialized, (int)toshiba_state.display_on, (int)toshiba_state.disp_powered_up);
    /* } FIHTDC-Div2-SW2-BSP, Ming, LCM */
      
    panel_off_process=1;/* FIHTDC-Div5-SW2-BSP, Chun { */ 
    
	if (toshiba_state.disp_powered_up && toshiba_state.display_on) {
		/* Main panel power off (Deep standby in) */

    if (panel_type == 1) {
    	  truly_spi_write(0x2800,0x00); //Display off
    	  mdelay(1);
    	  truly_spi_write(0x1000,0x00); //Sleep in
    	  mdelay(120);
    	  truly_spi_write(0x4F00,0x01); //Deep standby mode
     }else {
    		toshiba_spi_write(0x28, 0, 0);	/* display off */
    		mdelay(1);
    		toshiba_spi_write(0xb8, 0x8002, 2);	/* output control */
    		mdelay(1);
    		toshiba_spi_write(0x10, 0x00, 1);	/* sleep mode in */
    		mdelay(85);		/* wait 85 msec */
    		toshiba_spi_write(0xb0, 0x00, 1);	/* deep standby in */
    		mdelay(1);
    	}
    	
		if (lcdc_toshiba_pdata->panel_config_gpio)
			lcdc_toshiba_pdata->panel_config_gpio(0);
		toshiba_state.display_on = FALSE;
		toshiba_state.disp_initialized = FALSE;

	//DIV5-MM-KW-pull down SPI pin for SF4Y6-00+ {
	#if defined(CONFIG_FIH_PROJECT_SF4Y6)
		gpio_set_value(spi_sclk, 0); /* clk low */
		gpio_set_value(spi_cs, 0);	 /* cs low */
		gpio_set_value(spi_mosi, 0); /* ds low */
	#endif
	// }DIV5-MM-KW-pull down SPI pin for SF4Y6-00-

	}
	panel_off_process=0;/* } FIHTDC-Div5-SW2-BSP, Chun */ 
	
	return 0;
}

extern int bl_level_framework_store;/* FIHTDC-Div5-SW2-BSP, Chun */ 
static void lcdc_toshiba_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;
	int ret = -EPERM;
	//int i = 0;

	bl_level = mfd->bl_level;
	bl_level_framework_store=bl_level;
	printk(KERN_INFO "[DISPLAY] %s: level=%d\n", __func__, bl_level);
	/* FIHTDC-Div5-SW2-BSP, Chun {*/ 
    if (panel_off_process==1 ||disp_on_process==1||suspend_resume_flag==1)
    { 
    	bl_level=0;
    	alarm_flag=1;
    }	
    //printk(KERN_INFO "[DISPLAY] %s: After change,level=%d\n", __func__, bl_level);
    //printk(KERN_INFO "[DISPLAY] %s: bl_level_framework_store=%d\n", __func__,bl_level_framework_store);
	/* }FIHTDC-Div5-SW2-BSP, Chun */
	 
/* FIHTDC-Div2-SW2-BSP, Ming, Backlight { */
	 if (fih_get_product_phase()>=Product_PR2||(fih_get_product_id()!=Product_FB0 && fih_get_product_id()!=Product_FD1)) { 
        ret = gpio_tlmm_config(GPIO_CFG(98, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    	if (ret)
    		printk(KERN_INFO "[DISPLAY] %s: gpio_tlmm_config: 98 failed...\n", __func__);
///        mdelay(5);	
        
		if (bl_level == 0) {
			/* SW5-1-MM-KW-Backlight_PWM-01+ { */
			if ((fih_get_product_phase() > Product_PR2) && (fih_get_product_id() == Product_SF6)) {
				// GP_MD_REG (CLK_MD)
				writel( ((mfd->panel_info.bl_regs.gp_clk_m & 0x01ff) << 16) | 0xffff, MSM_CLK_CTL_BASE + 0x58);
				// GP_NS_REG (CLK_NS)
				writel( (~(mfd->panel_info.bl_regs.gp_clk_n - mfd->panel_info.bl_regs.gp_clk_m) << 16) | 0xb58, MSM_CLK_CTL_BASE + 0x5c);
		    } else {
				writel(0x0, MSM_CLK_CTL_BASE + 0x5c);
			}
			/* SW5-1-MM-KW-Backlight_PWM-01- } */

		    if (display_backlight_on == TRUE) {
		        clk_disable(gp_clk);	
		        display_backlight_on = FALSE;
		    }
		} else {		    
            // M/N:D counter, M=1 for modulo-n counter
        	// TCXO(SYS_CLK)=19.2MHz, PreDivSel=Div-4, N=240, 20kHz=19.2MHz/4/240
    	    // D = duty_cycle x N, 2D = duty_cycle x 2N	
    	    if (alarm_flag!=1) /* FIHTDC-Div5-SW2-BSP, Chun */ 
    	    {	
    	    	if (display_backlight_on == FALSE) {   	    	    
		        	clk_enable(gp_clk);			        	
		        	display_backlight_on = TRUE;
		    	}
				/* SW5-1-MM-KW-Backlight_PWM-00+ { */
				if ((fih_get_product_phase() > Product_PR2) && (fih_get_product_id() == Product_SF6)) {
					// M/N:D counter, M=1 for modulo-n counter
					// TCXO(SYS_CLK)=19.2MHz, PreDivSel=Div-4, N=150, 32kHz=19.2MHz/4/150
					// D = duty_cycle x N, 2D = duty_cycle x 2N
					mfd->panel_info.bl_regs.gp_clk_d = PWM[bl_level];
					// GP_MD_REG (CLK_MD)
					writel( ((mfd->panel_info.bl_regs.gp_clk_m & 0x01ff) << 16) | (~(mfd->panel_info.bl_regs.gp_clk_d * 2) & 0xffff), MSM_CLK_CTL_BASE + 0x58);
					// GP_NS_REG (CLK_NS)
					writel( (~(mfd->panel_info.bl_regs.gp_clk_n - mfd->panel_info.bl_regs.gp_clk_m) << 16) | 0xb58, MSM_CLK_CTL_BASE + 0x5c);
				} else {
					writel((1U << 16) | (~(bl_level * 480 / 100) & 0xffff), MSM_CLK_CTL_BASE + 0x58);

    		    	// GP_NS_REG (CLK_NS)
    		    	writel((~(239)<< 16) | 0xb58, MSM_CLK_CTL_BASE + 0x5c);
				}
				/* SW5-1-MM-KW-Backlight_PWM-00- } */
			}	
		}
    } else { // FB0_PR1, different backlight gpio configuration
     printk(KERN_INFO "[DISPLAY] %s: PR1, only backlight on/off.\n", __func__);
        gpio_request(83, "lcm_pwm");
         if (bl_level == 0)
        	gpio_set_value(83, 0);
        else
        	gpio_set_value(83, 1); 
    }
	/*while (i++ < 3) {
	//	ret = pmic_set_led_intensity(LED_LCD, bl_level);
    //if (ret == 0)
	//		return;
	//	msleep(10);
	}

	printk(KERN_WARNING "%s: can't set lcd backlight!\n",
				__func__);
    */
/* } FIHTDC-Div2-SW2-BSP, Ming, Backlight */ 
}

//Div2-SW2-BSP,JoeHsu ,+++
static int panel_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	char ver[24];
	int len;

	if (panel_type == 0)
 	    strcpy(ver, "TOSHIBA");
  else         	
	    strcpy(ver, "TRULY");


	len = snprintf(page, PAGE_SIZE, "%s\n",
		ver);
		
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;	
}
//Div2-SW2-BSP,JoeHsu ,---

static int __init toshiba_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		lcdc_toshiba_pdata = pdev->dev.platform_data;
#ifndef CONFIG_SPI_QSD
		spi_pin_assign();
#endif
		return 0;
	}
	/* FIHTDC-Div2-SW2-BSP, Ming, Backlight { */	
	gp_clk = clk_get(NULL, "gp_clk");
    if (IS_ERR(gp_clk)) 
    {
        printk(KERN_ERR "[DISPLAY] %s: could not get gp_clk.\n", __func__);
        gp_clk = NULL;
    }
    /* } FIHTDC-Div2-SW2-BSP, Ming, Backlight */
	msm_fb_add_device(pdev);
	return 0;
}

#ifdef CONFIG_SPI_QSD
static int __devinit lcdc_toshiba_spi_probe(struct spi_device *spi)
{
	lcdc_toshiba_spi_client = spi;
	lcdc_toshiba_spi_client->bits_per_word = 32;
	return 0;
}
static int __devexit lcdc_toshiba_spi_remove(struct spi_device *spi)
{
	lcdc_toshiba_spi_client = NULL;
	return 0;
}

static struct spi_driver lcdc_toshiba_spi_driver = {
	.driver = {
		.name  = LCDC_TOSHIBA_SPI_DEVICE_NAME,
		.owner = THIS_MODULE,
	},
	.probe         = lcdc_toshiba_spi_probe,
	.remove        = __devexit_p(lcdc_toshiba_spi_remove),
};
#endif
static struct platform_driver this_driver = {
	.probe  = toshiba_probe,
	.driver = {
		.name   = "lcdc_toshiba_wvga",
	},
};

static struct msm_fb_panel_data toshiba_panel_data = {
	.on = lcdc_toshiba_panel_on,
	.off = lcdc_toshiba_panel_off,
	.set_backlight = lcdc_toshiba_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_toshiba_wvga",
	.id	= 1,
	.dev	= {
		.platform_data = &toshiba_panel_data,
	}
};

static int __init lcdc_toshiba_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;
#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
	if (mddi_get_client_id() != 0)
		return 0;

	ret = msm_fb_detect_client("lcdc_toshiba_wvga_pt");
	if (ret)
		return 0;

#endif

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &toshiba_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	/* 30Mhz mdp_lcdc_pclk and mdp_lcdc_pad_pcl */
	pinfo->clk_rate = 24576000; //30720000;  // FIHTDC-Div2-SW2-BSP, Ming, 25MHz
	/* SW5-1-MM-KW-Backlight_PWM-00+ { */
	if ((fih_get_product_phase() > Product_PR2) && (fih_get_product_id() == Product_SF6)) {
		pinfo->bl_regs.bl_type = BL_TYPE_GP_CLK;
		pinfo->bl_regs.gp_clk_m = 1;
		pinfo->bl_regs.gp_clk_n = 150;
		pinfo->bl_max = 10;
		pinfo->bl_min = 1;
	} else {
		pinfo->bl_max = 100; //15; // FIHTDC-Div2-SW2-BSP, Ming, Backlight 
		pinfo->bl_min = 1;
	}
	/* SW5-1-MM-KW-Backlight_PWM-00- } */
	/*Div2-SW6-SC-Add_panel_size-00+{*/
	pinfo->width = 53;  //53.28mm 
	pinfo->height = 88; //88.80mm
	/*Div2-SW6-SC-Add_panel_size-00+}*/

#if 1
    // LT041MDM6x00 Timing sequense
    pinfo->lcdc.h_back_porch = 8;   // HBP
	pinfo->lcdc.h_front_porch = 16; // HFP
	pinfo->lcdc.h_pulse_width = 8;  // HSW
	pinfo->lcdc.v_back_porch = 2;	// VBP
	pinfo->lcdc.v_front_porch = 4;  // VFP
	pinfo->lcdc.v_pulse_width = 2;  // VSW
#else
	pinfo->lcdc.h_back_porch = 184;	/* hsw = 8 + hbp=184 */
	pinfo->lcdc.h_front_porch = 4;
	pinfo->lcdc.h_pulse_width = 8;
	pinfo->lcdc.v_back_porch = 2;	/* vsw=1 + vbp = 2 */
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 1;
#endif

		//Div2-SW2-BSP,JoeHsu
  	gpio_set_value(spi_cs, 1);	/* hi */
  	gpio_set_value(spi_sclk, 1);	/* high */
  	gpio_set_value(spi_mosi, 0);
  	mdelay(120);	
    truly_spi_write(0, 0);
  	truly_spi_read_cmd(0x0401);
	
  	if (truly_spi_read_data() == 0x80) {
    		panel_type = 1;
    		printk(KERN_INFO "truly panel ...\n");
    		    
        pinfo->lcdc.h_back_porch =  3;//4;  // HBP
      	pinfo->lcdc.h_front_porch = 3;//2;  // HFP
      	pinfo->lcdc.h_pulse_width = 3;//2;  // HSW
      	pinfo->lcdc.v_back_porch =  3; 	// VBP
      	pinfo->lcdc.v_front_porch = 6;  // VFP
      	pinfo->lcdc.v_pulse_width = 2;  // VSW  
    }

  create_proc_read_entry ("panel_type", 0, NULL,panel_read_proc, NULL);

	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret) {
		printk(KERN_ERR "%s not able to register the device\n",
			 __func__);
		goto fail_driver;
	}
#ifdef CONFIG_SPI_QSD
	ret = spi_register_driver(&lcdc_toshiba_spi_driver);

	if (ret) {
		printk(KERN_ERR "%s not able to register spi\n", __func__);
		goto fail_device;
	}
#endif
	return ret;

#ifdef CONFIG_SPI_QSD
fail_device:
	platform_device_unregister(&this_device);
#endif
fail_driver:
	platform_driver_unregister(&this_driver);
	return ret;
}

device_initcall(lcdc_toshiba_panel_init);
