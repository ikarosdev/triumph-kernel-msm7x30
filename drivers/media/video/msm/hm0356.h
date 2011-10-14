/*
 *     hm0356.h - Camera Sensor Config
 *
 *     Copyright (C) 2010 Kent Kwan <kentkwan@fihspec.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#ifndef HM0356_H
#define HM0356_H

#include <linux/types.h>
#include <mach/camera.h>

#include <mach/vreg.h>
#include <mach/gpio.h>

int hm0356_power_on(void);
int hm0356_power_off(void);

extern struct hm0356_reg hm0356_regs;

enum hm0356_width {
	FC_WORD_LEN,
	FC_BYTE_LEN
};

struct hm0356_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum hm0356_width width;
	unsigned short mdelay_time;
};

struct hm0356_reg {

	const struct hm0356_i2c_reg_conf *inittbl;
	uint16_t inittbl_size;
};

#endif /* HM0356_H */
