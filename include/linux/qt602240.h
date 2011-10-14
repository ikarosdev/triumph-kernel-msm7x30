#ifndef _qt602240_h_
#define _qt602240_h_

//Div2-D5-Peripheral-FG-AddMultiTouch-01*[
#ifndef CONFIG_FIH_FTM
#define QT602240_MT
#endif
//Div2-D5-Peripheral-FG-AddMultiTouch-01*]

//Div2-D5-Peripheral-FG-4Y6TouchPorting-00*[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
#include <linux/mfd/pmic8058.h>

#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)    (sys_gpio - NR_GPIO_IRQS)
#define PM8058_GPIO_TP_INT_N        9	//PMIC GPIO Number 10
#define GPIO_TP_INT_N               PM8058_GPIO_PM_TO_SYS(PM8058_GPIO_TP_INT_N)

struct pm8058_gpio touch_interrupt = {
    .direction      = PM_GPIO_DIR_IN,
    .pull           = PM_GPIO_PULL_NO,
    .vin_sel        = 2,
    .out_strength   = PM_GPIO_STRENGTH_NO,
    .function       = PM_GPIO_FUNC_NORMAL,
    .inv_int_pol    = 0,	//Div2-D5-Peripheral-FG-4Y6TouchFineTune-00*
};
#else
#define GPIO_TP_INT_N               42
#endif
//Div2-D5-Peripheral-FG-4Y6TouchPorting-00*]
#define GPIO_TP_RST_N               56
#define DEFAULT_REG_NUM             64

#define MSB(uint16_t)  (((uint8_t* )&uint16_t)[1])
#define LSB(uint16_t)  (((uint8_t* )&uint16_t)[0])

#define TOUCH_DEVICE_NAME           "qt602240"
#define TOUCH_DEVICE_VREG           "gp9"
#define TOUCH_DEVICE_I2C_ADDRESS    0x4Bu

#define LOW                         0
#define HIGH                        1
#define FALSE                       0
#define TRUE                        1

#define TS_MAX_POINTS               2
#define TS_MAX_X                    1023
#define TS_MIN_X                    0
//Div2-D5-Peripheral-FG-4Y6TouchPorting-00*[
#if defined(CONFIG_FIH_PROJECT_SF4Y6)
#define TS_MAX_Y                    975
#else
#define TS_MAX_Y                    1023
#endif
#define TS_MIN_Y                    0
//Div5-BSP-CW-TouchKey-00+[
#define TS_MENUKEYL_X                   50
#define TS_MENUKEYR_X                   205
#define TS_HOMEKEYL_X                   459
#define TS_HOMEKEYR_X                   614
#define TS_BACKKEYL_X                   818
#define TS_BACKKEYR_X                   973
//Div5-BSP-CW-TouchKey-00+]
//Div2-D5-Peripheral-FG-4Y6TouchPorting-00*]

#define DB_LEVEL1                   1
#define DB_LEVEL2                   2
#define TCH_DBG(level, message, ...) \
    do { \
        if ((level) <= qt_debug_level) \
            printk("[TOUCH] %s : " message "\n", __FUNCTION__, ## __VA_ARGS__); \
    } while (0)
#define TCH_ERR(msg, ...) printk("[TOUCH_ERR] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define TCH_MSG(msg, ...) printk("[TOUCH] " msg "\n", ## __VA_ARGS__)

struct point_info{
    int x;
    int y;
    int num;
    int first_area;
    int last_area;
};

struct qt602240_info {
    struct i2c_client *client;
    struct input_dev  *touch_input;
    struct input_dev  *keyevent_input;
    struct work_struct wqueue;
    struct point_info points[TS_MAX_POINTS];
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend es;
#endif
    int irq;
    bool suspend_state;
    int first_finger_id;
    bool facedetect;
    uint8_t T7[3];
} *qt602240;

enum ts_state {
    NO_TOUCH = 0,
    PRESS_TOUCH_AREA,
    PRESS_KEY1_AREA,
    PRESS_KEY2_AREA,
    PRESS_KEY3_AREA
};

#endif