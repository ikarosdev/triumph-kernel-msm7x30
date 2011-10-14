#ifndef _SF8_KYBD_H_
#define _SF8_KYBD_H_

struct sf8_kybd_platform_data {
    unsigned int pmic_gpio_vol_up;
    unsigned int pmic_gpio_vol_dn;
    unsigned int pmic_gpio_cover_det;
    
    unsigned int sys_gpio_vol_up;
    unsigned int sys_gpio_vol_dn;
    unsigned int sys_gpio_cover_det;
};

#endif