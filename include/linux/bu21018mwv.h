#ifndef _bu21018mwv_h_
#define _bu21018mwv_h_

#define TS_MAX_X				705
#define TS_MIN_X				1
#define TS_MAX_Y				1152
#define TS_MIN_Y				1

#define  TOUCH_BTN0_THON		0x15
#define  TOUCH_BTN1_THON		0x12
#define  TOUCH_BTN2_THON		0x12
#define  TOUCH_BTN3_THON		0x15

#define  TOUCH_BTN0_THOFF		0x5
#define  TOUCH_BTN1_THOFF		0x5
#define  TOUCH_BTN2_THOFF		0x5
#define  TOUCH_BTN3_THOFF		0x5


#define  RAW_SIN35_REG			0x23
#define  RAW_SIN34_REG			0x22
#define  RAW_SIN13_REG			0x0D
#define  RAW_SIN12_REG			0x0C

#define PRE_INIT_TIME			2

// ioctl
#define READ_TH					0
#define READ_INIT				1
#define READ_FW					16388
#define READ_REG				3
#define BTN_MSG					4
#define COOR_MSG				5
#define RPT_DELAY				6

// FLAG
//#define BU21018MWV_SUSPEND
#ifndef CONFIG_FIH_FTM
#define BU21018MWV_MT
#endif

enum
{
    FALSE = 0,
    TRUE,
};

enum
{
    EVENT_NONE = 0,
    EVENT_TOUCH,
    EVENT_KEY,
};

enum
{
    V_KEY_NONE = 0,
    V_KEY_BACK,
    V_KEY_MENU,
    V_KEY_HOME,
    V_KEY_SEARCH,
};

enum
{
	HW_UNKNOW = 0,
	HW_PR1,
	HW_MIX,
	HW_PR2,
	HW_FD1_PR3,
	HW_FD1_PR4
};

struct bu21018mwv_info {
    struct i2c_client *client;
    struct input_dev  *touch_input;
    struct input_dev  *keyevent_input;
    struct work_struct wqueue;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend es;
#endif
    int irq;
    bool suspend_state;
    struct timer_list timer;
    struct work_struct pre_init_work;
    struct work_struct test_work;
} *bu21018mwv;

#endif
