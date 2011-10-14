#ifndef _FBX_KYBD_H_
#define _FBX_KYBD_H_

struct fbx_kybd_platform_data {
    unsigned int pmic_gpio_cam_f;
    unsigned int pmic_gpio_cam_t;
    unsigned int pmic_gpio_vol_up;
    unsigned int pmic_gpio_vol_dn;
    unsigned int sys_gpio_cam_f;
    unsigned int sys_gpio_cam_t;
    unsigned int sys_gpio_vol_up;
    unsigned int sys_gpio_vol_dn;
	unsigned int hook_sw_pin; /* Div1-FW3-BSP-AUDIO */
};
	
/* Div1-FW3-BSP-AUDIO */
extern int fbx_hookswitch_irqsetup(bool activate_irq);
#endif
