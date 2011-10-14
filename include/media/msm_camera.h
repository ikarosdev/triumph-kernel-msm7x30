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

#ifndef __LINUX_MSM_CAMERA_H
#define __LINUX_MSM_CAMERA_H

#ifdef MSM_CAMERA_BIONIC
#include <sys/types.h>
#endif
#include <linux/types.h>
#include <asm/sizes.h>
#include <linux/ioctl.h>
#ifdef MSM_CAMERA_GCC
#include <time.h>
#else
#include <linux/time.h>
#endif

#define MSM_CAM_IOCTL_MAGIC 'm'

#define MSM_CAM_IOCTL_GET_SENSOR_INFO \
	_IOR(MSM_CAM_IOCTL_MAGIC, 1, struct msm_camsensor_info *)

#define MSM_CAM_IOCTL_REGISTER_PMEM \
	_IOW(MSM_CAM_IOCTL_MAGIC, 2, struct msm_pmem_info *)

#define MSM_CAM_IOCTL_UNREGISTER_PMEM \
	_IOW(MSM_CAM_IOCTL_MAGIC, 3, unsigned)

#define MSM_CAM_IOCTL_CTRL_COMMAND \
	_IOW(MSM_CAM_IOCTL_MAGIC, 4, struct msm_ctrl_cmd *)

#define MSM_CAM_IOCTL_CONFIG_VFE  \
	_IOW(MSM_CAM_IOCTL_MAGIC, 5, struct msm_camera_vfe_cfg_cmd *)

#define MSM_CAM_IOCTL_GET_STATS \
	_IOR(MSM_CAM_IOCTL_MAGIC, 6, struct msm_camera_stats_event_ctrl *)

#define MSM_CAM_IOCTL_GETFRAME \
	_IOR(MSM_CAM_IOCTL_MAGIC, 7, struct msm_camera_get_frame *)

#define MSM_CAM_IOCTL_ENABLE_VFE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 8, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_CTRL_CMD_DONE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 9, struct camera_cmd *)

#define MSM_CAM_IOCTL_CONFIG_CMD \
	_IOW(MSM_CAM_IOCTL_MAGIC, 10, struct camera_cmd *)

#define MSM_CAM_IOCTL_DISABLE_VFE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 11, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_PAD_REG_RESET2 \
	_IOW(MSM_CAM_IOCTL_MAGIC, 12, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_VFE_APPS_RESET \
	_IOW(MSM_CAM_IOCTL_MAGIC, 13, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_RELEASE_FRAME_BUFFER \
	_IOW(MSM_CAM_IOCTL_MAGIC, 14, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_RELEASE_STATS_BUFFER \
	_IOW(MSM_CAM_IOCTL_MAGIC, 15, struct msm_stats_buf *)

#define MSM_CAM_IOCTL_AXI_CONFIG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 16, struct msm_camera_vfe_cfg_cmd *)

#define MSM_CAM_IOCTL_GET_PICTURE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 17, struct msm_camera_ctrl_cmd *)

#define MSM_CAM_IOCTL_SET_CROP \
	_IOW(MSM_CAM_IOCTL_MAGIC, 18, struct crop_info *)

#define MSM_CAM_IOCTL_PICT_PP \
	_IOW(MSM_CAM_IOCTL_MAGIC, 19, uint8_t *)

#define MSM_CAM_IOCTL_PICT_PP_DONE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 20, struct msm_snapshot_pp_status *)

#define MSM_CAM_IOCTL_SENSOR_IO_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 21, struct sensor_cfg_data *)

#define MSM_CAM_IOCTL_FLASH_LED_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 22, unsigned *)

#define MSM_CAM_IOCTL_UNBLOCK_POLL_FRAME \
	_IO(MSM_CAM_IOCTL_MAGIC, 23)

#define MSM_CAM_IOCTL_CTRL_COMMAND_2 \
	_IOW(MSM_CAM_IOCTL_MAGIC, 24, struct msm_ctrl_cmd *)

#define MSM_CAM_IOCTL_AF_CTRL \
	_IOR(MSM_CAM_IOCTL_MAGIC, 25, struct msm_ctrl_cmt_t *)

#define MSM_CAM_IOCTL_AF_CTRL_DONE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 26, struct msm_ctrl_cmt_t *)

#define MSM_CAM_IOCTL_CONFIG_VPE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 27, struct msm_camera_vpe_cfg_cmd *)

#define MSM_CAM_IOCTL_AXI_VPE_CONFIG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 28, struct msm_camera_vpe_cfg_cmd *)

#define MSM_CAM_IOCTL_STROBE_FLASH_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 29, uint32_t *)

#define MSM_CAM_IOCTL_STROBE_FLASH_CHARGE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 30, uint32_t *)

#define MSM_CAM_IOCTL_STROBE_FLASH_RELEASE \
	_IO(MSM_CAM_IOCTL_MAGIC, 31)

#define MSM_CAM_IOCTL_ERROR_CONFIG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 32, uint32_t *)

#define MSM_CAM_IOCTL_ABORT_CAPTURE \
	_IO(MSM_CAM_IOCTL_MAGIC, 33)
//Div6D1-HL-Camera-BringUp-00+{
#ifdef CONFIG_FIH_CONFIG_GROUP
#define MSM_CAM_IOCTL_GET_FIH_SENSOR_INFO  \
	_IOR(MSM_CAM_IOCTL_MAGIC, 34, struct msm_camsensor_info *)
#endif
//Div6D1-HL-Camera-BringUp-00+}
#define MSM_CAMERA_LED_OFF  0
#define MSM_CAMERA_LED_LOW  1
#define MSM_CAMERA_LED_HIGH 2

#define MSM_CAMERA_STROBE_FLASH_NONE 0
#define MSM_CAMERA_STROBE_FLASH_XENON 1

#define MAX_SENSOR_NUM  3
#define MAX_SENSOR_NAME 32

#define PP_SNAP  0x01
#define PP_RAW_SNAP ((0x01)<<1)
#define PP_PREV  ((0x01)<<2)
#define PP_MASK		(PP_SNAP|PP_RAW_SNAP|PP_PREV)

#define MSM_CAM_CTRL_CMD_DONE  0
#define MSM_CAM_SENSOR_VFE_CMD 1

/*****************************************************
 *  structure
 *****************************************************/

/* define five type of structures for userspace <==> kernel
 * space communication:
 * command 1 - 2 are from userspace ==> kernel
 * command 3 - 4 are from kernel ==> userspace
 *
 * 1. control command: control command(from control thread),
 *                     control status (from config thread);
 */
struct msm_ctrl_cmd {
	uint16_t type;
	uint16_t length;
	void *value;
	uint16_t status;
	uint32_t timeout_ms;
	int resp_fd; /* FIXME: to be used by the kernel, pass-through for now */
};

struct msm_vfe_evt_msg {
	unsigned short type;	/* 1 == event (RPC), 0 == message (adsp) */
	unsigned short msg_id;
	unsigned int len;	/* size in, number of bytes out */
	uint32_t frame_id;
	void *data;
};

struct msm_vpe_evt_msg {
	unsigned short type; /* 1 == event (RPC), 0 == message (adsp) */
	unsigned short msg_id;
	unsigned int len; /* size in, number of bytes out */
	uint32_t frame_id;
	void *data;
};

#define MSM_CAM_RESP_CTRL         0
#define MSM_CAM_RESP_STAT_EVT_MSG 1
#define MSM_CAM_RESP_V4L2         2
#define MSM_CAM_RESP_MAX          3

/* this one is used to send ctrl/status up to config thread */
struct msm_stats_event_ctrl {
	/* 0 - ctrl_cmd from control thread,
	 * 1 - stats/event kernel,
	 * 2 - V4L control or read request */
	int resptype;
	int timeout_ms;
	struct msm_ctrl_cmd ctrl_cmd;
	/* struct  vfe_event_t  stats_event; */
	struct msm_vfe_evt_msg stats_event;
};

/* 2. config command: config command(from config thread); */
struct msm_camera_cfg_cmd {
	/* what to config:
	 * 1 - sensor config, 2 - vfe config */
	uint16_t cfg_type;

	/* sensor config type */
	uint16_t cmd_type;
	uint16_t queue;
	uint16_t length;
	void *value;
};

#define CMD_GENERAL			0
#define CMD_AXI_CFG_OUT1		1
#define CMD_AXI_CFG_SNAP_O1_AND_O2	2
#define CMD_AXI_CFG_OUT2		3
#define CMD_PICT_T_AXI_CFG		4
#define CMD_PICT_M_AXI_CFG		5
#define CMD_RAW_PICT_AXI_CFG		6

#define CMD_FRAME_BUF_RELEASE		7
#define CMD_PREV_BUF_CFG		8
#define CMD_SNAP_BUF_RELEASE		9
#define CMD_SNAP_BUF_CFG		10
#define CMD_STATS_DISABLE		11
#define CMD_STATS_AEC_AWB_ENABLE	12
#define CMD_STATS_AF_ENABLE		13
#define CMD_STATS_AEC_ENABLE		14
#define CMD_STATS_AWB_ENABLE		15
#define CMD_STATS_ENABLE  		16

#define CMD_STATS_AXI_CFG		17
#define CMD_STATS_AEC_AXI_CFG		18
#define CMD_STATS_AF_AXI_CFG 		19
#define CMD_STATS_AWB_AXI_CFG		20
#define CMD_STATS_RS_AXI_CFG		21
#define CMD_STATS_CS_AXI_CFG		22
#define CMD_STATS_IHIST_AXI_CFG		23
#define CMD_STATS_SKIN_AXI_CFG		24

#define CMD_STATS_BUF_RELEASE		25
#define CMD_STATS_AEC_BUF_RELEASE	26
#define CMD_STATS_AF_BUF_RELEASE	27
#define CMD_STATS_AWB_BUF_RELEASE	28
#define CMD_STATS_RS_BUF_RELEASE	29
#define CMD_STATS_CS_BUF_RELEASE	30
#define CMD_STATS_IHIST_BUF_RELEASE	31
#define CMD_STATS_SKIN_BUF_RELEASE	32

#define UPDATE_STATS_INVALID		33
#define CMD_AXI_CFG_SNAP_GEMINI		34
#define CMD_AXI_CFG_SNAP		35
#define CMD_AXI_CFG_PREVIEW		36
#define CMD_AXI_CFG_VIDEO		37

#define CMD_STATS_IHIST_ENABLE 38
#define CMD_STATS_RS_ENABLE 39
#define CMD_STATS_CS_ENABLE 40
#define CMD_VPE 41
#define CMD_AXI_CFG_VPE 42

/* vfe config command: config command(from config thread)*/
struct msm_vfe_cfg_cmd {
	int cmd_type;
	uint16_t length;
	void *value;
};

struct msm_vpe_cfg_cmd {
	int cmd_type;
	uint16_t length;
	void *value;
};

#define MAX_CAMERA_ENABLE_NAME_LEN 32
struct camera_enable_cmd {
	char name[MAX_CAMERA_ENABLE_NAME_LEN];
};

#define MSM_PMEM_OUTPUT1		0
#define MSM_PMEM_OUTPUT2		1
#define MSM_PMEM_OUTPUT1_OUTPUT2	2
#define MSM_PMEM_THUMBNAIL		3
#define MSM_PMEM_MAINIMG		4
#define MSM_PMEM_RAW_MAINIMG		5
#define MSM_PMEM_AEC_AWB		6
#define MSM_PMEM_AF			7
#define MSM_PMEM_AEC			8
#define MSM_PMEM_AWB			9
#define MSM_PMEM_RS		    	10
#define MSM_PMEM_CS	    		11
#define MSM_PMEM_IHIST			12
#define MSM_PMEM_SKIN			13
#define MSM_PMEM_VIDEO			14
#define MSM_PMEM_PREVIEW		15
#define MSM_PMEM_VIDEO_VPE		16
#define MSM_PMEM_MAX			17

#define STAT_AEAW			0
#define STAT_AEC			1
#define STAT_AF				2
#define STAT_AWB			3
#define STAT_RS				4
#define STAT_CS				5
#define STAT_IHIST			6
#define STAT_SKIN			7
#define STAT_MAX			8

#define FRAME_PREVIEW_OUTPUT1		0
#define FRAME_PREVIEW_OUTPUT2		1
#define FRAME_SNAPSHOT			2
#define FRAME_THUMBNAIL			3
#define FRAME_RAW_SNAPSHOT		4
#define FRAME_MAX			5

struct msm_pmem_info {
	int type;
	int fd;
	void *vaddr;
	uint32_t offset;
	uint32_t len;
	uint32_t y_off;
	uint32_t cbcr_off;
	uint8_t active;
};

struct outputCfg {
	uint32_t height;
	uint32_t width;

	uint32_t window_height_firstline;
	uint32_t window_height_lastline;
};

#define OUTPUT_1	0
#define OUTPUT_2	1
#define OUTPUT_1_AND_2            2   /* snapshot only */
#define OUTPUT_1_AND_3            3   /* video */
#define CAMIF_TO_AXI_VIA_OUTPUT_2 4
#define OUTPUT_1_AND_CAMIF_TO_AXI_VIA_OUTPUT_2 5
#define OUTPUT_2_AND_CAMIF_TO_AXI_VIA_OUTPUT_1 6
#define LAST_AXI_OUTPUT_MODE_ENUM = OUTPUT_2_AND_CAMIF_TO_AXI_VIA_OUTPUT_1 7

#define MSM_FRAME_PREV_1	0
#define MSM_FRAME_PREV_2	1
#define MSM_FRAME_ENC		2

#define OUTPUT_TYPE_P		(1<<0)
#define OUTPUT_TYPE_T		(1<<1)
#define OUTPUT_TYPE_S		(1<<2)
#define OUTPUT_TYPE_V		(1<<3)
#define OUTPUT_TYPE_L		(1<<4)

struct msm_frame {
	struct timespec ts;
	int path;
	unsigned long buffer;
	uint32_t y_off;
	uint32_t cbcr_off;
	int fd;

	void *cropinfo;
	int croplen;
	uint32_t error_code;
};

#define MSM_CAMERA_ERR_MASK (0xFFFFFFFF & 1)

struct msm_stats_buf {
	int type;
	unsigned long buffer;
	int fd;
};

#define MSM_V4L2_VID_CAP_TYPE	0
#define MSM_V4L2_STREAM_ON	1
#define MSM_V4L2_STREAM_OFF	2
#define MSM_V4L2_SNAPSHOT	3
#define MSM_V4L2_QUERY_CTRL	4
#define MSM_V4L2_GET_CTRL	5
#define MSM_V4L2_SET_CTRL	6
#define MSM_V4L2_QUERY		7
#define MSM_V4L2_GET_CROP	8
#define MSM_V4L2_SET_CROP	9
#define MSM_V4L2_MAX		10

#define V4L2_CAMERA_EXIT 	43
struct crop_info {
	void *info;
	int len;
};

struct msm_postproc {
	int ftnum;
	struct msm_frame fthumnail;
	int fmnum;
	struct msm_frame fmain;
};

struct msm_snapshot_pp_status {
	void *status;
};

#define CFG_SET_MODE			0
#define CFG_SET_EFFECT			1
#define CFG_START			2
#define CFG_PWR_UP			3
#define CFG_PWR_DOWN			4
#define CFG_WRITE_EXPOSURE_GAIN		5
#define CFG_SET_DEFAULT_FOCUS		6
#define CFG_MOVE_FOCUS			7
#define CFG_REGISTER_TO_REAL_GAIN	8
#define CFG_REAL_TO_REGISTER_GAIN	9
#define CFG_SET_FPS			10
#define CFG_SET_PICT_FPS		11
#define CFG_SET_BRIGHTNESS		12
#define CFG_SET_CONTRAST		13
#define CFG_SET_ZOOM			14
#define CFG_SET_EXPOSURE_MODE		15
#define CFG_SET_WB			16
#define CFG_SET_ANTIBANDING		17
#define CFG_SET_EXP_GAIN		18
#define CFG_SET_PICT_EXP_GAIN		19
#define CFG_SET_LENS_SHADING		20
#define CFG_GET_PICT_FPS		21
#define CFG_GET_PREV_L_PF		22
#define CFG_GET_PREV_P_PL		23
#define CFG_GET_PICT_L_PF		24
#define CFG_GET_PICT_P_PL		25
#define CFG_GET_AF_MAX_STEPS		26
#define CFG_GET_PICT_MAX_EXP_LC		27
#define CFG_SEND_WB_INFO    28
//Div6D1-HL-Camera-BringUp-00*{
#ifdef CONFIG_FIH_CONFIG_GROUP
#define CFG_SET_LEDMOD 29
#define CFG_SET_EXPOSUREMOD 30
#define CFG_SET_SATURATION 31
#define CFG_SET_SHARPNESS 32
#define CFG_SET_HUE	 33
#define CFG_SET_GAMMA 34
#define CFG_SET_AUTOEXPOSURE 35
#define CFG_SET_AUTOFOCUS 36
#define CFG_SET_METERINGMOD 37
#define CFG_SET_SCENEMOD 38
#define CFG_SET_FOCUSREC 39//Div6D1-CL-Camera-autofocus-01+
#define CFG_STROBE_FLASH 41//Div2-SW6-MM-HL-Camera-Flash-02+
#define CFG_SET_ISO 42//Div2-SW6-MM-HL-Camera-ISO-00+
#define CFG_SET_CAF 43//Div2-SW6-MM-HL-Camera-CAF-00+
#define CFG_SET_touchAEC 44
#define CFG_MAX	45//Div2-SW6-MM-HL-Camera-ISO-00*
#else
#define CFG_MAX 			29
#endif
//Div6D1-HL-Camera-BringUp-00*}


#define MOVE_NEAR	0
#define MOVE_FAR	1

#define SENSOR_PREVIEW_MODE		0
#define SENSOR_SNAPSHOT_MODE		1
#define SENSOR_RAW_SNAPSHOT_MODE	2
#define SENSOR_VIDEO_120FPS_MODE	3
#define SENSOR_RESET_MODE 4//Div2-SW6-MM-CL-mt9p111noImage-00+

#define SENSOR_QTR_SIZE			0
#define SENSOR_FULL_SIZE		1
#define SENSOR_QVGA_SIZE		2
#define SENSOR_INVALID_SIZE		3

#define CAMERA_EFFECT_OFF		0
#define CAMERA_EFFECT_MONO		1
#define CAMERA_EFFECT_NEGATIVE		2
#define CAMERA_EFFECT_SOLARIZE		3
#define CAMERA_EFFECT_SEPIA		4
#define CAMERA_EFFECT_POSTERIZE		5
#define CAMERA_EFFECT_WHITEBOARD	6
#define CAMERA_EFFECT_BLACKBOARD	7
#define CAMERA_EFFECT_AQUA		8
//Div6D1-HL-Camera-BringUp-00*{
#ifdef CONFIG_FIH_CONFIG_GROUP
#define CAMERA_EFFECT_BLUISH		9
#define CAMERA_EFFECT_REDDISH		10
#define CAMERA_EFFECT_GREENISH		11
//Div2-SW6-MM-MC-ImplementCameraColorBarMechanism-00*{
#define CAMERA_EFFECT_COLORBAR		12
#define CAMERA_EFFECT_MAX		13
//Div2-SW6-MM-MC-ImplementCameraColorBarMechanism-00*}


#define CAMERA_WB_MIN_MINUS_1		0
#define CAMERA_WB_AUTO			1
#define CAMERA_WB_CUSTOM		2
#define CAMERA_WB_INCANDESCENT		3
#define CAMERA_WB_FLUORESCENT		4
#define CAMERA_WB_DAYLIGHT		5
#define CAMERA_WB_CLOUDY_DAYLIGHT	6
#define CAMERA_WB_TWILIGHT		7
#define CAMERA_WB_SHADE			8
#define CAMERA_WB_MAX_PLUS_1		9

#define CAMERA_BRIGHTNESS_MIN		0
#define CAMERA_BRIGHTNESS_0		0
#define CAMERA_BRIGHTNESS_1		1
#define CAMERA_BRIGHTNESS_2		2
#define CAMERA_BRIGHTNESS_3		3
#define CAMERA_BRIGHTNESS_4		4
#define CAMERA_BRIGHTNESS_5		5
#define CAMERA_BRIGHTNESS_DEFAULT	5
#define CAMERA_BRIGHTNESS_6		6
#define CAMERA_BRIGHTNESS_7		7
#define CAMERA_BRIGHTNESS_8		8
#define CAMERA_BRIGHTNESS_9		9
#define CAMERA_BRIGHTNESS_10		10
#define CAMERA_BRIGHTNESS_MAX		10

#define CAMERA_ANTIBANDING_OFF		0
#define CAMERA_ANTIBANDING_60HZ		1
#define CAMERA_ANTIBANDING_50HZ		2
#define CAMERA_ANTIBANDING_AUTO		3
#define CAMERA_MAX_ANTIBANDING		4

#define LED_MODE_OFF 	0
#define LED_MODE_AUTO 	1
#define LED_MODE_ON 	2
#define LED_MODE_TORCH 	3


#define CAMERA_AEC_FRAME_AVERAGE		0
#define CAMERA_AEC_CENTER_WEIGHTED		1
#define CAMERA_AEC_SPOT_METERING		2
#define CAMERA_AEC_MAX_MODES			3

#define CAMERA_SATURATION_MINUS2		0
#define CAMERA_SATURATION_MINUS1		1
#define CAMERA_SATURATION_ZERO			2
#define CAMERA_SATURATION_POSITIVE1		3
#define CAMERA_SATURATION_POSITIVE2		4

#define CAMERA_SHARPNESS_MINUS2		0	//Div6D1-HL-Camera-Sharpness-00+
#define CAMERA_SHARPNESS_MINUS1		1	//Div6D1-HL-Camera-Sharpness-00+
#define CAMERA_SHARPNESS_ZERO		2	//Div6D1-HL-Camera-Sharpness-00*
#define CAMERA_SHARPNESS_POSITIVE1	3	//Div6D1-HL-Camera-Sharpness-00*
#define CAMERA_SHARPNESS_POSITIVE2	4	//Div6D1-HL-Camera-Sharpness-00*

#define CAMERA_CONTRAST_MINUS2		0
#define CAMERA_CONTRAST_MINUS1		1
#define CAMERA_CONTRAST_ZERO			2
#define CAMERA_CONTRAST_POSITIVE1	3
#define CAMERA_CONTRAST_POSITIVE2	4

#define CAMERA_SCENE_MODE_AUTO		0
#define CAMERA_SCENE_MODE_LANDSCAPE	1
#define CAMERA_SCENE_MODE_PORTRAIT	2
#define CAMERA_SCENE_MODE_NIGHT		3
#define CAMERA_SCENE_MODE_NIGHTPORTRAIT		4
#define CAMERA_SCENE_MODE_SUNSET		5

//Div6D1-CL-Camera-SensorInfo-01+{
#define CAMERA_BESTSHOT_OFF 0
#define CAMERA_BESTSHOT_LANDSCAPE 1
#define CAMERA_BESTSHOT_SNOW 2
#define CAMERA_BESTSHOT_BEACH 3
#define CAMERA_BESTSHOT_SUNSET 4
#define CAMERA_BESTSHOT_NIGHT 5
#define CAMERA_BESTSHOT_PORTRAIT 6
#define CAMERA_BESTSHOT_BACKLIGHT 7
#define CAMERA_BESTSHOT_SPORTS 8
#define CAMERA_BESTSHOT_ANTISHAKE 9
#define CAMERA_BESTSHOT_FLOWERS 10
#define CAMERA_BESTSHOT_CANDLELIGHT 11
#define CAMERA_BESTSHOT_FIREWORKS 12
#define CAMERA_BESTSHOT_PARTY 13
#define CAMERA_BESTSHOT_NIGHT_PORTRAIT 14
#define CAMERA_BESTSHOT_THEATRE 15
#define CAMERA_BESTSHOT_ACTION 16

#define FPS_MODE_AUTO 0
#define FPS_MODE_FIXED 1

#define AF_MODE_UNCHANGED -1
#define AF_MODE_NORMAL   0
#define AF_MODE_MACRO 1
#define AF_MODE_AUTO 2
//Div6D1-CL-Camera-SensorInfo-01+}
//Div6D1-HL-Camera-Camcorder_HD-00+{
//Div6D1-CL-Camera-D1_WVGA-00*{
#define CAMERA_PRECONFIG_VGA 0
#define CAMERA_PRECONFIG_720P 1
#define CAMERA_PRECONFIG_D1 2
#define CAMERA_PRECONFIG_WVGA 3
//Div6D1-CL-Camera-D1_WVGA-00*}
//Div6D1-HL-Camera-Camcorder_HD-00+}
#else
#define CAMERA_EFFECT_MAX		9
#endif
//Div6D1-HL-Camera-BringUp-00*}


struct sensor_pict_fps {
	uint16_t prevfps;
	uint16_t pictfps;
};

struct exp_gain_cfg {
	uint16_t gain;
	uint32_t line;
};

struct focus_cfg {
	int32_t steps;
	int dir;
};

struct fps_cfg {
	uint16_t f_mult;
	uint16_t fps_div;
	uint32_t pict_fps_div;
};
struct wb_info_cfg {
	uint16_t red_gain;
	uint16_t green_gain;
	uint16_t blue_gain;
};

struct touchAEC{
    int enable;
    uint32_t AEC_X;
    uint32_t AEC_Y;
};

//Div6D1-CL-Camera-autofocus-01+{
 struct camera_focus_rectangle{  
    /* Focus Window dimensions */
    int16_t x_upper_left;           
    int16_t y_upper_left;
    int16_t width; 
    int16_t height;
} ;
//Div6D1-CL-Camera-autofocus-01+}

 //Div2-SW6-MM-HL-Camera-ISO-01+{
 //Div2-SW6-MM-HL-Camera-ISO-00+{
 /* Enum Type for different ISO Mode supported */
typedef enum
{
  CAMERA_ISO_ISO_AUTO = 0,
  CAMERA_ISO_ISO_DEBLUR,
  CAMERA_ISO_ISO_100,
  CAMERA_ISO_ISO_200,
  CAMERA_ISO_ISO_400,
  CAMERA_ISO_ISO_800,
  CAMERA_ISO_ISO_1600,
  CAMERA_ISO_ISO_MAX
} camera_iso_mode;
//Div2-SW6-MM-HL-Camera-ISO-00+}
//Div2-SW6-MM-HL-Camera-ISO-01+}

struct sensor_cfg_data {
    int cfgtype;
    int mode;
    int rs;
    uint8_t max_steps;

    union {
        int8_t effect;
        //Div6D1-HL-Camera-BringUp-00+{
#ifdef CONFIG_FIH_CONFIG_GROUP
        int8_t wb;
        int8_t antibanding;
        int8_t brightness;
        int8_t ledmod;
        int8_t exposuremod;
        int8_t saturation;
        int8_t sharpness;
        int8_t contrast;
        int8_t hue;
        int8_t gamma;
        int8_t autoexposure;
        int8_t autofocus;
        int8_t meteringmod;
        int8_t scenemod;
        int8_t CAF;//Div2-SW6-MM-HL-Camera-CAF-00+
        struct touchAEC AECIndex;
        struct camera_focus_rectangle focusrec;//Div6D1-CL-Camera-autofocus-01+        
        int8_t hd;//Div6D1-HL-Camera-Camcorder_HD-00+
        camera_iso_mode iso;//Div2-SW6-MM-HL-Camera-ISO-00+
#endif
        //Div6D1-HL-Camera-BringUp-00+}
        uint8_t lens_shading;
        uint16_t prevl_pf;
        uint16_t prevp_pl;
        uint16_t pictl_pf;
        uint16_t pictp_pl;
        uint32_t pict_max_exp_lc;
        uint16_t p_fps;
        struct sensor_pict_fps gfps;
        struct exp_gain_cfg exp_gain;
        struct focus_cfg focus;
        struct fps_cfg fps;
        struct wb_info_cfg wb_info;
    } cfg;
};

#define GET_NAME			0
#define GET_PREVIEW_LINE_PER_FRAME	1
#define GET_PREVIEW_PIXELS_PER_LINE	2
#define GET_SNAPSHOT_LINE_PER_FRAME	3
#define GET_SNAPSHOT_PIXELS_PER_LINE	4
#define GET_SNAPSHOT_FPS		5
#define GET_SNAPSHOT_MAX_EP_LINE_CNT	6

struct msm_camsensor_info {
    char name[MAX_SENSOR_NAME];
    uint8_t flash_enabled;
    uint8_t sensor_Orientation;//Div6D1-CL-Camera-SensorInfo-00+
    int8_t total_steps;
    //Div6D1-CL-Camera-SensorInfo-01+{
    //Div6D1-CL-Camera-SensorInfo-02*{
    uint32_t autoexposure;
    uint32_t effects;
    uint32_t wb;
    uint32_t antibanding;
    uint32_t flash;
    uint32_t focus;
    uint32_t ISO;
    uint32_t lensshade;
    uint32_t scenemode;
    uint32_t continuous_af;
    uint32_t touchafaec;
    uint32_t frame_rate_modes;
    //Div6D1-CL-Camera-SensorInfo-02*}
    int8_t  max_brightness;
    int8_t  max_contrast;
    int8_t  max_saturation;
    int8_t  max_sharpness;
    int8_t  min_brightness;
    int8_t  min_contrast;
    int8_t  min_saturation;
    int8_t  min_sharpness;
    //Div6D1-CL-Camera-SensorInfo-01+}
};
#endif /* __LINUX_MSM_CAMERA_H */
