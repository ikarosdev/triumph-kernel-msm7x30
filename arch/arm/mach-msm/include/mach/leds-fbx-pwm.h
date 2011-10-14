#ifndef _LEDS_FBX_PWM_
#define _LEDS_FBX_PWM_

#include <mach/pmic.h>

enum {
    FBX_R_LED           = LOW_CURRENT_LED_DRV0,
    FBX_G_LED           = LOW_CURRENT_LED_DRV1,
    FBX_CAPS_KEY_LED    = LOW_CURRENT_LED_DRV2
};

enum {
    LPG_PMIC_GPIO24,
    LPG_PMIC_GPIO25,
    LPG_PMIC_GPIO26,
    LPG_KYPD_DRV,
    LPG_LED_DRV0,
    LPG_LED_DRV1,
    LPG_LED_DRV2,
    LPG_FLASH_DRV0 = LPG_LED_DRV2,
    LPG_FLASH_DRV1,
};

struct leds_fbx_pwm_platform_data {
    unsigned int r_led_ctl;
};

#endif
