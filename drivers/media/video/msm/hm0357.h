/*
 *     hm0357.h - Camera Sensor Config
 *
 *     Copyright (C) 2010 Kent Kwan <kentkwan@fihspec.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#ifndef HM0357_H
#define HM0357_H

#include <linux/types.h>
#include <mach/camera.h>

#include <mach/vreg.h>
#include <mach/gpio.h>

int hm0357_power_on(void);
int hm0357_power_off(void);

extern struct hm0357_reg hm0357_regs;

enum hm0357_width {
	FC_WORD_LEN,
	FC_BYTE_LEN,
	BYTE_LEN
};

struct hm0357_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum hm0357_width width;
	unsigned short mdelay_time;
};

struct hm0357_reg {

	const struct hm0357_i2c_reg_conf *inittbl;
	uint16_t inittbl_size;
};

#endif /* HM0357_H */
