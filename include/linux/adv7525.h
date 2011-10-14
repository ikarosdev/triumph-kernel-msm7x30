/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _ADV7525_H_
#define _ADV7525_H_
#define ADV7525_DRV_NAME 		"adv7525"
#include <linux/wakelock.h>
#define HDMI_XRES			1280
#define HDMI_YRES			720
#define HDMI_PIXCLOCK_MAX		74250
/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */     
///#define ADV7520_EDIDI2CSLAVEADDRESS   	0xA0
#define ADV7525_EDIDI2CSLAVEADDRESS   	0x7E
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */  
#define DEBUG  0

/* Configure the 20-bit 'N' used with the CTS to
regenerate the audio clock in the receiver
Pixel clock: 74.25 Mhz, Audio sampling: 44.1 Khz -> N
value = 6272 */
#define ADV7525_AUDIO_CTS_20BIT_N   6272

/* HDMI EDID Length  */
#define HDMI_EDID_MAX_LENGTH    256

/* HDMI EDID DTDs  */
#define HDMI_EDID_MAX_DTDS      4

/* all video formats defined by EIA CEA 861D */
#define HDMI_VFRMT_640x480p60_4_3	0
#define HDMI_VFRMT_720x480p60_4_3	1
#define HDMI_VFRMT_720x480p60_16_9	2
#define HDMI_VFRMT_1280x720p60_16_9	3
#define HDMI_VFRMT_1920x1080i60_16_9	4
#define HDMI_VFRMT_720x480i60_4_3	5
#define HDMI_VFRMT_1440x480i60_4_3	HDMI_VFRMT_720x480i60_4_3
#define HDMI_VFRMT_720x480i60_16_9	6
#define HDMI_VFRMT_1440x480i60_16_9	HDMI_VFRMT_720x480i60_16_9
#define HDMI_VFRMT_720x240p60_4_3	7
#define HDMI_VFRMT_1440x240p60_4_3	HDMI_VFRMT_720x240p60_4_3
#define HDMI_VFRMT_720x240p60_16_9	8
#define HDMI_VFRMT_1440x240p60_16_9	HDMI_VFRMT_720x240p60_16_9
#define HDMI_VFRMT_2880x480i60_4_3	9
#define HDMI_VFRMT_2880x480i60_16_9	10
#define HDMI_VFRMT_2880x240p60_4_3	11
#define HDMI_VFRMT_2880x240p60_16_9	12
#define HDMI_VFRMT_1440x480p60_4_3	13
#define HDMI_VFRMT_1440x480p60_16_9	14
#define HDMI_VFRMT_1920x1080p60_16_9	15
#define HDMI_VFRMT_720x576p50_4_3	16
#define HDMI_VFRMT_720x576p50_16_9	17
#define HDMI_VFRMT_1280x720p50_16_9	18
#define HDMI_VFRMT_1920x1080i50_16_9	19
#define HDMI_VFRMT_720x576i50_4_3	20
#define HDMI_VFRMT_1440x576i50_4_3	HDMI_VFRMT_720x576i50_4_3
#define HDMI_VFRMT_720x576i50_16_9	21
#define HDMI_VFRMT_1440x576i50_16_9	HDMI_VFRMT_720x576i50_16_9
#define HDMI_VFRMT_720x288p50_4_3	22
#define HDMI_VFRMT_1440x288p50_4_3	HDMI_VFRMT_720x288p50_4_3
#define HDMI_VFRMT_720x288p50_16_9	23
#define HDMI_VFRMT_1440x288p50_16_9	HDMI_VFRMT_720x288p50_16_9
#define HDMI_VFRMT_2880x576i50_4_3	24
#define HDMI_VFRMT_2880x576i50_16_9	25
#define HDMI_VFRMT_2880x288p50_4_3	26
#define HDMI_VFRMT_2880x288p50_16_9	27
#define HDMI_VFRMT_1440x576p50_4_3	28
#define HDMI_VFRMT_1440x576p50_16_9	29
#define HDMI_VFRMT_1920x1080p50_16_9	30
#define HDMI_VFRMT_1920x1080p24_16_9	31
#define HDMI_VFRMT_1920x1080p25_16_9	32
#define HDMI_VFRMT_1920x1080p30_16_9	33
#define HDMI_VFRMT_2880x480p60_4_3	34
#define HDMI_VFRMT_2880x480p60_16_9	35
#define HDMI_VFRMT_2880x576p50_4_3	36
#define HDMI_VFRMT_2880x576p50_16_9	37
#define HDMI_VFRMT_1920x1250i50_16_9	38
#define HDMI_VFRMT_1920x1080i100_16_9	39
#define HDMI_VFRMT_1280x720p100_16_9	40
#define HDMI_VFRMT_720x576p100_4_3	41
#define HDMI_VFRMT_720x576p100_16_9	42
#define HDMI_VFRMT_720x576i100_4_3	43
#define HDMI_VFRMT_1440x576i100_4_3	HDMI_VFRMT_720x576i100_4_3
#define HDMI_VFRMT_720x576i100_16_9	44
#define HDMI_VFRMT_1440x576i100_16_9	HDMI_VFRMT_720x576i100_16_9
#define HDMI_VFRMT_1920x1080i120_16_9	45
#define HDMI_VFRMT_1280x720p120_16_9	46
#define HDMI_VFRMT_720x480p120_4_3	47
#define HDMI_VFRMT_720x480p120_16_9	48
#define HDMI_VFRMT_720x480i120_4_3	49
#define HDMI_VFRMT_1440x480i120_4_3	HDMI_VFRMT_720x480i120_4_3
#define HDMI_VFRMT_720x480i120_16_9	50
#define HDMI_VFRMT_1440x480i120_16_9	HDMI_VFRMT_720x480i120_16_9
#define HDMI_VFRMT_720x576p200_4_3	51
#define HDMI_VFRMT_720x576p200_16_9	52
#define HDMI_VFRMT_720x576i200_4_3	53
#define HDMI_VFRMT_1440x576i200_4_3	HDMI_VFRMT_720x576i200_4_3
#define HDMI_VFRMT_720x576i200_16_9	54
#define HDMI_VFRMT_1440x576i200_16_9	HDMI_VFRMT_720x576i200_16_9
#define HDMI_VFRMT_720x480p240_4_3	55
#define HDMI_VFRMT_720x480p240_16_9	56
#define HDMI_VFRMT_720x480i240_4_3	57
#define HDMI_VFRMT_1440x480i240_4_3	HDMI_VFRMT_720x480i240_4_3
#define HDMI_VFRMT_720x480i240_16_9	58
#define HDMI_VFRMT_1440x480i240_16_9	HDMI_VFRMT_720x480i240_16_9
#define HDMI_VFRMT_MAX			59
#define HDMI_VFRMT_FORCE_32BIT		0x7FFFFFFF
/*  EDID - Extended Display ID Data structs  */

/*  Video Descriptor Block  */
struct hdmi_edid_dtd_video {
	u8   pixel_clock[2];          /* 54-55 */
	u8   horiz_active;            /* 56 */
	u8   horiz_blanking;          /* 57 */
	u8   horiz_high;              /* 58 */
	u8   vert_active;             /* 59 */
	u8   vert_blanking;           /* 60 */
	u8   vert_high;               /* 61 */
	u8   horiz_sync_offset;       /* 62 */
	u8   horiz_sync_pulse;        /* 63 */
	u8   vert_sync_pulse;         /* 64 */
	u8   sync_pulse_high;         /* 65 */
	u8   horiz_image_size;        /* 66 */
	u8   vert_image_size;	      /* 67 */
	u8   image_size_high;         /* 68 */
	u8   horiz_border;            /* 69 */
	u8   vert_border;             /* 70 */
	u8   misc_settings;           /* 71 */
} ;

/*  EDID structure  */
struct hdmi_edid {			/* Bytes */
	u8   edid_header[8];            /* 00-07 */
	u8   manufacturerID[2];      	/* 08-09 */
	u8   product_id[2];           	/* 10-11 */
	u8   serial_number[4];        	/* 12-15 */
	u8   week_manufactured;       	/* 16 */
	u8   year_manufactured;       	/* 17 */
	u8   edid_version;            	/* 18 */
	u8   edid_revision;           	/* 19 */

	u8   video_in_definition;      	/* 20 */
	u8   max_horiz_image_size;      /* 21 */
	u8   max_vert_image_size;       /* 22 */
	u8   display_gamma;           	/* 23 */
	u8   power_features;          	/* 24 */
	u8   chroma_info[10];         	/* 25-34 */
	u8   timing_1;                	/* 35 */
	u8   timing_2;               	/* 36 */
	u8   timing_3;              	/* 37 */
	u8   std_timings[16];         	/* 38-53 */

	struct hdmi_edid_dtd_video dtd[4];   /* 54-125 */

	u8   extension_edid;          	/* 126 */
	u8   checksum;               	/* 127 */

	u8   extension_tag;           	/* 00 (extensions follow EDID) */
	u8   extention_rev;           	/* 01 */
	u8   offset_dtd;              	/* 02 */
	u8   num_dtd;                 	/* 03 */

	u8   data_block[123];         	/* 04 - 126 */
	u8   extension_checksum;      	/* 127 */
} ;

struct hdmi_state {
	struct kobject kobj;
	int hdmi_connection_state;
	struct wake_lock wlock;
};

/* FIHTDC, Div2-SW2-BSP SungSCLee, HDMI { */
/* The Logic ID for HDMI TX Core. Currently only support 1 HDMI TX Core. */
struct hdmi_edid_video_mode_property_type {
	u32	video_code;
	u32	active_h;
	u32	active_v;
	bool	interlaced;
	u32	total_h;
	u32	total_blank_h;
	u32	total_v;
	u32	total_blank_v;
	/* Must divide by 1000 to get the frequency */
	u32	freq_h;
	/* Must divide by 1000 to get the frequency */
	u32	freq_v;
	/* Must divide by 1000 to get the frequency */
	u32	pixel_freq;
	/* Must divide by 1000 to get the frequency */
	u32	refresh_rate;
	bool	aspect_ratio_4_3;
};
/* } FIHTDC, Div2-SW2-BSP SungSCLee, HDMI */

#endif
