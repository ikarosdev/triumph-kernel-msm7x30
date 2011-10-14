/*
 *     hm0356_reg.c - Camera Sensor Config
 *
 *     Copyright (C) 2010 Kent Kwan <kentkwan@fihspec.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#include "hm0356.h"

struct hm0356_i2c_reg_conf const fc_init_tbl[] = {
	{ 0x0021, 0x00, FC_BYTE_LEN, 0 },
	{ 0x001E, 0x01, FC_BYTE_LEN, 0 },
	{ 0x0018, 0x81, FC_BYTE_LEN, 0 },
	{ 0x001C, 0x3B, FC_BYTE_LEN, 0 },
	{ 0x0019, 0x80, FC_BYTE_LEN, 0 },
	{ 0x0026, 0x03, FC_BYTE_LEN, 0 },

//Basic
	{ 0x0006, 0x08, FC_BYTE_LEN, 0 },
	{ 0x0022, 0x47, FC_BYTE_LEN, 0 },
	{ 0x0039, 0x12, FC_BYTE_LEN, 0 },
	{ 0x005D, 0x80, FC_BYTE_LEN, 0 },
	{ 0x005E, 0x88, FC_BYTE_LEN, 0 },
	{ 0x005F, 0x60, FC_BYTE_LEN, 0 }, //Contrast Center
	{ 0x0060, 0x50, FC_BYTE_LEN, 0 }, //Saturation
	{ 0x0064, 0x2C, FC_BYTE_LEN, 0 }, //decrease edge enhance to reduce ¿÷¾¦ª¬
	{ 0x00A7, 0x0C, FC_BYTE_LEN, 0 },
	{ 0x00A8, 0x0C, FC_BYTE_LEN, 0 },
	{ 0x00A9, 0xF1, FC_BYTE_LEN, 0 }, //select option 1 for UV denoise
	{ 0x00AA, 0x10, FC_BYTE_LEN, 0 },
	{ 0x00B6, 0x14, FC_BYTE_LEN, 0 },
	{ 0x00B7, 0x14, FC_BYTE_LEN, 0 },
	{ 0x00B8, 0x0E, FC_BYTE_LEN, 0 }, //Bayer De-Noise Strength
	{ 0x00B9, 0x0C, FC_BYTE_LEN, 0 }, //Y De-Noise BPC
	{ 0x00BB, 0x06, FC_BYTE_LEN, 0 },
	{ 0x00F4, 0x0D, FC_BYTE_LEN, 0 },
	{ 0x00F5, 0x89, FC_BYTE_LEN, 0 },

//AE windowing
	{ 0x00D5, 0x18, FC_BYTE_LEN, 0 },
	{ 0x00D6, 0x20, FC_BYTE_LEN, 0 },
	{ 0x00D7, 0x90, FC_BYTE_LEN, 0 },
	{ 0x00D8, 0xC0, FC_BYTE_LEN, 0 },
//AWB windowing
	{ 0x00EB, 0x18, FC_BYTE_LEN, 0 },
	{ 0x00EC, 0x20, FC_BYTE_LEN, 0 },
	{ 0x00ED, 0x90, FC_BYTE_LEN, 0 },
	{ 0x00EE, 0xC0, FC_BYTE_LEN, 0 },

//AE
	{ 0x0025, 0x09, FC_BYTE_LEN, 0 },
	{ 0x0030, 0x40, FC_BYTE_LEN, 0 },
	{ 0x0031, 0x03, FC_BYTE_LEN, 0 },
	{ 0x0032, 0xE0, FC_BYTE_LEN, 0 },
	{ 0x0033, 0x10, FC_BYTE_LEN, 0 },
	{ 0x0034, 0x02, FC_BYTE_LEN, 0 },
	{ 0x0035, 0x80, FC_BYTE_LEN, 0 },
	{ 0x0038, 0x7D, FC_BYTE_LEN, 0 },
	{ 0x00DA, 0x4B, FC_BYTE_LEN, 0 },
	{ 0x00DB, 0x35, FC_BYTE_LEN, 0 },

//AE Framerate Control
	{ 0x0040, 0x01, FC_BYTE_LEN, 0 },
	{ 0x0041, 0xF5, FC_BYTE_LEN, 0 },
	{ 0x0042, 0x02, FC_BYTE_LEN, 0 },
	{ 0x0043, 0x72, FC_BYTE_LEN, 0 },
	{ 0x0044, 0x05, FC_BYTE_LEN, 0 },
	{ 0x0045, 0xD0, FC_BYTE_LEN, 0 },
	{ 0x0046, 0x22, FC_BYTE_LEN, 0 },
	{ 0x0047, 0x84, FC_BYTE_LEN, 0 },
	{ 0x0048, 0xB9, FC_BYTE_LEN, 0 },
	{ 0x0049, 0x55, FC_BYTE_LEN, 0 },
	{ 0x004A, 0x1E, FC_BYTE_LEN, 0 },

//AWB A5 WLM 20091015
	{ 0x00C3, 0x40, FC_BYTE_LEN, 0 },
	{ 0x00C4, 0x68, FC_BYTE_LEN, 0 },
	{ 0x00C5, 0x22, FC_BYTE_LEN, 0 },
	{ 0x00C6, 0x7F, FC_BYTE_LEN, 0 },
	{ 0x00C7, 0x42, FC_BYTE_LEN, 0 },
	{ 0x00C8, 0x8C, FC_BYTE_LEN, 0 },
	{ 0x00C9, 0x3F, FC_BYTE_LEN, 0 },
	{ 0x00CA, 0x52, FC_BYTE_LEN, 0 },
	{ 0x00CD, 0x90, FC_BYTE_LEN, 0 },
	{ 0x00CE, 0x60, FC_BYTE_LEN, 0 },
	{ 0x00CF, 0xD0, FC_BYTE_LEN, 0 },

//CCM A5 WLM 20091015
	{ 0x0050, 0x00, FC_BYTE_LEN, 0 },
	{ 0x0051, 0x91, FC_BYTE_LEN, 0 },
	{ 0x0052, 0x8C, FC_BYTE_LEN, 0 },
	{ 0x0053, 0x64, FC_BYTE_LEN, 0 },
	{ 0x0054, 0xD5, FC_BYTE_LEN, 0 },
	{ 0x0055, 0xAF, FC_BYTE_LEN, 0 },
	{ 0x0056, 0x25, FC_BYTE_LEN, 0 },
	{ 0x0057, 0x44, FC_BYTE_LEN, 0 },
	{ 0x0058, 0xD2, FC_BYTE_LEN, 0 },
	{ 0x0059, 0x7D, FC_BYTE_LEN, 0 },
	{ 0x005A, 0x1B, FC_BYTE_LEN, 0 },
	{ 0x005B, 0xBD, FC_BYTE_LEN, 0 },
	{ 0x005C, 0xD9, FC_BYTE_LEN, 0 },

//LSC fixed green corner 20100325
	{ 0x0080, 0x10, FC_BYTE_LEN, 0 },
	{ 0x0081, 0x00, FC_BYTE_LEN, 0 },
	{ 0x0082, 0xD0, FC_BYTE_LEN, 0 },
	{ 0x0083, 0x00, FC_BYTE_LEN, 0 },
	{ 0x0084, 0x90, FC_BYTE_LEN, 0 },
	{ 0x0085, 0x84, FC_BYTE_LEN, 0 },
	{ 0x0086, 0x00, FC_BYTE_LEN, 0 },
	{ 0x0087, 0x70, FC_BYTE_LEN, 0 },
	{ 0x0088, 0x84, FC_BYTE_LEN, 0 },
	{ 0x0089, 0x00, FC_BYTE_LEN, 0 },
	{ 0x008A, 0x70, FC_BYTE_LEN, 0 },
	{ 0x008B, 0x68, FC_BYTE_LEN, 0 },
	{ 0x008C, 0x00, FC_BYTE_LEN, 0 },
	{ 0x008D, 0x70, FC_BYTE_LEN, 0 },
	{ 0x008E, 0x11, FC_BYTE_LEN, 0 },
	{ 0x008F, 0x10, FC_BYTE_LEN, 0 },
	{ 0x0090, 0x11, FC_BYTE_LEN, 0 },
	{ 0x0091, 0x10, FC_BYTE_LEN, 0 },
	{ 0x0092, 0x11, FC_BYTE_LEN, 0 },
	{ 0x0093, 0x10, FC_BYTE_LEN, 0 },
	{ 0x0094, 0x11, FC_BYTE_LEN, 0 },
	{ 0x0095, 0x10, FC_BYTE_LEN, 0 },
	{ 0x0096, 0x01, FC_BYTE_LEN, 0 },
	{ 0x0097, 0x01, FC_BYTE_LEN, 0 },
	{ 0x0098, 0x01, FC_BYTE_LEN, 0 },
	{ 0x0099, 0x01, FC_BYTE_LEN, 0 },
	{ 0x009A, 0x00, FC_BYTE_LEN, 0 },
	{ 0x009B, 0x00, FC_BYTE_LEN, 0 },
	{ 0x009C, 0x00, FC_BYTE_LEN, 0 },
	{ 0x009D, 0x00, FC_BYTE_LEN, 0 },
	{ 0x009E, 0x3E, FC_BYTE_LEN, 0 },
	{ 0x009F, 0x3E, FC_BYTE_LEN, 0 },
	{ 0x00A0, 0x3E, FC_BYTE_LEN, 0 },
	{ 0x00A1, 0x3E, FC_BYTE_LEN, 0 },
	{ 0x00A2, 0xEF, FC_BYTE_LEN, 0 },
	{ 0x00A3, 0xEF, FC_BYTE_LEN, 0 },
	{ 0x00A4, 0xEF, FC_BYTE_LEN, 0 },
	{ 0x00A5, 0xEF, FC_BYTE_LEN, 0 },

//Gamma
	{ 0x006C, 0x40, FC_BYTE_LEN, 0 },
	{ 0x006D, 0x55, FC_BYTE_LEN, 0 },
	{ 0x006E, 0xAA, FC_BYTE_LEN, 0 },
	{ 0x006F, 0x3F, FC_BYTE_LEN, 0 },
	{ 0x0070, 0x18, FC_BYTE_LEN, 0 },
	{ 0x0071, 0x38, FC_BYTE_LEN, 0 },
	{ 0x0072, 0x89, FC_BYTE_LEN, 0 },
	{ 0x0073, 0x32, FC_BYTE_LEN, 0 },
	{ 0x0074, 0x6C, FC_BYTE_LEN, 0 },
	{ 0x0075, 0x9E, FC_BYTE_LEN, 0 },
	{ 0x0076, 0xCD, FC_BYTE_LEN, 0 },
	{ 0x0077, 0xFD, FC_BYTE_LEN, 0 },
	{ 0x0078, 0x1F, FC_BYTE_LEN, 0 },
	{ 0x0079, 0x4C, FC_BYTE_LEN, 0 },
	{ 0x007A, 0x98, FC_BYTE_LEN, 0 },
	{ 0x007B, 0xCD, FC_BYTE_LEN, 0 },
	{ 0x007C, 0x23, FC_BYTE_LEN, 0 },
	{ 0x007D, 0x60, FC_BYTE_LEN, 0 },
	{ 0x007E, 0x91, FC_BYTE_LEN, 0 },
	{ 0x007F, 0x92, FC_BYTE_LEN, 0 },

//Bank 1
	{ 0x0021, 0x01, FC_BYTE_LEN, 0 },
	{ 0x003A, 0x00, FC_BYTE_LEN, 0 },
	{ 0x005D, 0x50, FC_BYTE_LEN, 0 },
	{ 0x005E, 0x80, FC_BYTE_LEN, 0 },
	{ 0x005F, 0x80, FC_BYTE_LEN, 0 },
	{ 0x0060, 0x04, FC_BYTE_LEN, 0 }, //Saturation alpha1=1/2
	{ 0x0064, 0x04, FC_BYTE_LEN, 0 }, //Edge enhance alpha1=1/2
	{ 0x0080, 0x08, FC_BYTE_LEN, 0 },  //troy add for garmin
	{ 0x00B6, 0x28, FC_BYTE_LEN, 0 },
	{ 0x00B7, 0x28, FC_BYTE_LEN, 0 },
	{ 0x00B8, 0x08, FC_BYTE_LEN, 0 }, //Bayer De-Noise Strength
	{ 0x00B9, 0x08, FC_BYTE_LEN, 0 },
	{ 0x00BB, 0x08, FC_BYTE_LEN, 0 },
	{ 0x00AD, 0x03, FC_BYTE_LEN, 0 }, //Add original pixel for Y denoise

//Bank 3
	{ 0x0021, 0x03, FC_BYTE_LEN, 0 },
	{ 0x0068, 0x20, FC_BYTE_LEN, 0 },
	{ 0x006C, 0x28, FC_BYTE_LEN, 0 },
	{ 0x0071, 0x00, FC_BYTE_LEN, 0 },
	{ 0x0073, 0x70, FC_BYTE_LEN, 0 },
	{ 0x007C, 0x0B, FC_BYTE_LEN, 0 },
	{ 0x0080, 0x88, FC_BYTE_LEN, 0 },
	{ 0x0081, 0x08, FC_BYTE_LEN, 0 },
	{ 0x0083, 0xCA, FC_BYTE_LEN, 0 },
	{ 0x0092, 0x6B, FC_BYTE_LEN, 0 },
	{ 0x0095, 0x08, FC_BYTE_LEN, 0 },
	{ 0x0096, 0x01, FC_BYTE_LEN, 0 },
	{ 0x00A0, 0x02, FC_BYTE_LEN, 0 },
	{ 0x00A1, 0x00, FC_BYTE_LEN, 0 },
	{ 0x00A2, 0x00, FC_BYTE_LEN, 0 },
	{ 0x00A3, 0x02, FC_BYTE_LEN, 0 },
	{ 0x00A4, 0x00, FC_BYTE_LEN, 0 },
	{ 0x00A5, 0x84, FC_BYTE_LEN, 0 },
	{ 0x00A6, 0x00, FC_BYTE_LEN, 0 },
	{ 0x00AB, 0xFF, FC_BYTE_LEN, 0 },
	{ 0x00AC, 0xD0, FC_BYTE_LEN, 0 },
	{ 0x00AD, 0xA0, FC_BYTE_LEN, 0 },
	{ 0x00AE, 0x60, FC_BYTE_LEN, 0 },
	{ 0x00B7, 0x14, FC_BYTE_LEN, 0 },
	{ 0x00B8, 0x10, FC_BYTE_LEN, 0 },
	{ 0x00BB, 0x10, FC_BYTE_LEN, 0 },
	{ 0x0021, 0x00, FC_BYTE_LEN, 0 },

//newBLC 
	{ 0x0021, 0x03, FC_BYTE_LEN, 0 },
	{ 0x006C, 0x2A, FC_BYTE_LEN, 0 },
	{ 0x006D, 0xB0, FC_BYTE_LEN, 0 },
	{ 0x006E, 0x38, FC_BYTE_LEN, 0 },
	{ 0x0070, 0x50, FC_BYTE_LEN, 0 },
	{ 0x0071, 0x03, FC_BYTE_LEN, 0 },
	{ 0x0073, 0x70, FC_BYTE_LEN, 0 },
	{ 0x007C, 0x0B, FC_BYTE_LEN, 0 },
	{ 0x0021, 0x00, FC_BYTE_LEN, 0 },

//fixed green corner 20100325
	{ 0x0021, 0x03, FC_BYTE_LEN, 0 },
	{ 0x00B8, 0x05, FC_BYTE_LEN, 0 },
	{ 0x0021, 0x00, FC_BYTE_LEN, 0 },

//Start
	{ 0x0000, 0x01, FC_BYTE_LEN, 0 },
	{ 0x0005, 0x01, FC_BYTE_LEN, 0 }, //Turn on rolling shutter
};

struct hm0356_reg hm0356_regs = {
	.inittbl = fc_init_tbl,
	.inittbl_size = ARRAY_SIZE(fc_init_tbl),
};
