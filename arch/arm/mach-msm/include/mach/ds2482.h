#ifndef _DS2482_H_
#define _DS2482_H_

/* Function Commands */
#define CMD_DEVICE_RESET        0xF0
#define CMD_SET_READ_POINT      0xE1
#define CMD_WRITE_CONFIGURATION 0xD2
#define CMD_OW_RESET            0xB4
#define CMD_OW_SINGLE_BIT       0x87
#define CMD_OW_WRITE_BYTE       0xA5
#define CMD_OW_READ_BYTE        0x96
#define CMD_OW_TRIPLET          0x78

/* Valid Pointers */
#define REGISTER_STATUS         0xF0
#define REGISTER_READ_DATA      0xE1
#define REGISTER_CONFIGURATION  0xC3

struct ds2482_platform_data {
        unsigned int pmic_gpio_ds2482_SLPZ;
        unsigned int sys_gpio_ds2482_SLPZ;
        unsigned int sys_gpio_gauge_ls_en;
};

/* Status Register Bit Assignment */
enum {
    STATUS_REG_BIT_1WB,
    STATUS_REG_BIT_PPD,
    STATUS_REG_BIT_SD,
    STATUS_REG_BIT_LL,
    STATUS_REG_BIT_RST,
    STATUS_REG_BIT_SBR,
    STATUS_REG_BIT_TSB,
    STATUS_REG_BIT_DIR,
};

#define STATUS_REG_BIT_MASK_FOR_CHECK_RESET ((1 << STATUS_REG_BIT_1WB) | (1 << STATUS_REG_BIT_PPD) | (1 << STATUS_REG_BIT_SD) | (1 << STATUS_REG_BIT_RST) | (1 << STATUS_REG_BIT_SBR) | (1 << STATUS_REG_BIT_TSB) | (1 << STATUS_REG_BIT_DIR))

/* Configuration Register Bit Assignment */
enum {
    CONFIGURATION_REG_BIT_APU,
    CONFIGURATION_REG_BIT_1,
    CONFIGURATION_REG_BIT_SPU,
    CONFIGURATION_REG_BIT_1WS,
    CONFIGURATION_REG_BIT_NAPU,
    CONFIGURATION_REG_BIT_5,
    CONFIGURATION_REG_BIT_NSPU,
    CONFIGURATION_REG_BIT_N1WS,
};
#endif
