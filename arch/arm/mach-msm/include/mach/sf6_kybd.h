#ifndef _SF6_KYBD_H_
#define _SF6_KYBD_H_

struct sf6_kybd_platform_data {
    unsigned int pmic_gpio_cam_f;
    unsigned int pmic_gpio_cam_t;
    unsigned int pmic_gpio_vol_up;
    unsigned int pmic_gpio_vol_dn;
    unsigned int sys_gpio_cam_f;
    unsigned int sys_gpio_cam_t;
    unsigned int sys_gpio_vol_up;
    unsigned int sys_gpio_vol_dn;
};


#endif
