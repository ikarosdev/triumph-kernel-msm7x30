/*
 *     tcm9001md.h - Camera Sensor Config
 *
 *     Copyright (C) 2010 Kent Kwan <kentkwan@fihspec.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#ifndef TCM9001MD_H
#define TCM9001MD_H

#include <linux/types.h>
#include <mach/camera.h>

#include <mach/vreg.h>
#include <mach/gpio.h>

int tcm9001md_power_on(void);
int tcm9001md_power_off(void);

extern struct tcm9001md_reg tcm9001md_regs;

enum tcm9001md_width {
	FC_WORD_LEN,
	FC_BYTE_LEN
};

struct tcm9001md_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum tcm9001md_width width;
	unsigned short mdelay_time;
};

struct tcm9001md_reg {
	const struct tcm9001md_i2c_reg_conf *inittbl;
	uint16_t inittbl_size;
};

#endif /* TCM9001MD_H */
