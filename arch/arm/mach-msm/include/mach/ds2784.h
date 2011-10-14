#ifndef _DS2784_H_
#define _DS2784_H_

#define DS2784_PROTECTION_REG                   0x00
#define DS2784_STATUS_REG                       0x01
#define DS2784_RAAC_REG_H                       0x02
#define DS2784_RAAC_REG_L                       0x03
#define DS2784_RSAC_REG_H                       0x04
#define DS2784_RSAC_REG_L                       0x05
#define DS2784_RARC_REG                         0x06
#define DS2784_RSRC_REG                         0x07
#define DS2784_AVG_CURR_REG_H                   0x08
#define DS2784_AVG_CURR_REG_L                   0x09
#define DS2784_TEMP_REG_H                       0x0A
#define DS2784_TEMP_REG_L                       0x0B
#define DS2784_VOL_REG_H                        0x0C
#define DS2784_VOL_REG_L                        0x0D
#define DS2784_CURR_REG_H                       0x0E
#define DS2784_CURR_REG_L                       0x0F
#define DS2784_ACCUMULATED_CURRENT_REG_H        0x10
#define DS2784_ACCUMULATED_CURRENT_REG_L        0x11
#define DS2784_AGE_SCALAR_REG                   0x14
#define DS2784_SPECIAL_FEATURE_REG              0x15
#define DS2784_FULL_REG_H                       0x16
#define DS2784_FULL_REG_L                       0x17
#define DS2784_ACTIVE_EMPTY_REG_H               0x18
#define DS2784_ACTIVE_EMPTY_REG_L               0x19
#define DS2784_STANDBY_EMPTY_REG_H              0x1A
#define DS2784_STANDBY_EMPTY_REG_L              0x1B
#define DS2784_EEPROM_REG                       0x1F
#define DS2784_CTRL_REG                         0x60
#define DS2784_ACCUMULATION_BIAS_REG            0x61
#define DS2784_AGING_CAPACITY_REG_H             0x62
#define DS2784_AGING_CAPACITY_REG_L             0x63
#define DS2784_CHARGE_VOL_REG                   0x64
#define DS2784_MIN_CHARGE_CURR_REG              0x65
#define DS2784_ACTIVE_EMPTY_VOL_REG             0x66
#define DS2784_ACTIVE_EMPTY_CURR_REG            0x67
#define DS2784_SENSE_RESISTOR_PRIME_REG         0x69
#define DS2784_FULL_40_REG_H                    0x6A
#define DS2784_FULL_40_REG_L                    0x6B
#define DS2784_CURR_MEASUREMENT_GAIN_REG_H      0x78
#define DS2784_CURR_MEASUREMENT_GAIN_REG_L      0x79
#define DS2784_SENSE_RESISTOR_TEMP_COMPENSATION_REG 0x7A 
#define DS2784_CURR_OFF_BIAS_REG                0x7B
#define DS2784_PROTECTOR_THRESHOLD_REG          0x7F

#define DS2784CMD_READ_NET_ADDR                 0x33
#define DS2784CMD_COPY_DATA                     0x48
#define DS2784CMD_READ_DATA                     0x69
#define DS2784CMD_LOCK                          0x6A
#define DS2784CMD_WRITE_DATA                    0x6C
#define DS2784CMD_RECALL_DATA                   0xB8
#define DS2784CMD_RESET                         0xC4
#define DS2784CMD_SKIP_NET_ADDR                 0xCC
#define DS2784CMD_SET_OVERDRIVE                 0x8B

#ifndef BATTERY_OVER_TEMP_LOW
#define BATTERY_OVER_TEMP_LOW       10
#endif
#ifndef BATTERY_OVER_TEMP_HIGH
#define BATTERY_OVER_TEMP_HIGH      440
#endif

enum {
    DS2784_CTRL_REG_BIT0,
    DS2784_CTRL_REG_PSDQ,
    DS2784_CTRL_REG_PSPIO,
    DS2784_CTRL_REG_BIT3,
    DS2784_CTRL_REG_RNAOP,
    DS2784_CTRL_REG_PMOD,
    DS2784_CTRL_REG_UVEN,
    DS2784_CTRL_REG_NBEN,
};

enum {
    DS2784_PROTECTION_REG_DE,
    DS2784_PROTECTION_REG_CE,
    DS2784_PROTECTION_REG_DC,
    DS2784_PROTECTION_REG_CC,
    DS2784_PROTECTION_REG_DOC,
    DS2784_PROTECTION_REG_COC,
    DS2784_PROTECTION_REG_UV,
    DS2784_PROTECTION_REG_OV,
};

enum {
    DS2784_PROTECTOR_THRESHOLD_REG_OC0,
    DS2784_PROTECTOR_THRESHOLD_REG_OC1,
    DS2784_PROTECTOR_THRESHOLD_REG_SC0,
    DS2784_PROTECTOR_THRESHOLD_REG_VOV0,
    DS2784_PROTECTOR_THRESHOLD_REG_VOV1,
    DS2784_PROTECTOR_THRESHOLD_REG_VOV2,
    DS2784_PROTECTOR_THRESHOLD_REG_VOV3,
    DS2784_PROTECTOR_THRESHOLD_REG_VOV4,
};

enum {
    DS2784_STATUS_REG_BIT0,
    DS2784_STATUS_REG_PORF,
    DS2784_STATUS_REG_UVF,
    DS2784_STATUS_REG_BIT3,
    DS2784_STATUS_REG_LEARNF,
    DS2784_STATUS_REG_SEF,
    DS2784_STATUS_REG_AEF,
    DS2784_STATUS_REG_CHGTF,
};

enum {
    DS2784_SPECIAL_FEATURE_REG_PIOB,  
};

enum {
    DS2784_EEPROM_REG_BL0,
    DS2784_EEPROM_REG_BL1,
    DS2784_EEPROM_REG_BIT2,
    DS2784_EEPROM_REG_BIT3,
    DS2784_EEPROM_REG_BIT4,
    DS2784_EEPROM_REG_BIT5,
    DS2784_EEPROM_REG_LOCK,
    DS2784_EEPROM_REG_EEC,
};

enum _battery_info_type_ {
    BATT_CAPACITY_INFO,         // 0x06 RARC - Remaining Active Relative Capacity
    BATT_VOLTAGE_INFO,          // 0x0c Voltage MSB
    BATT_TEMPERATURE_INFO,      // 0x0a Temperature MSB
    BATT_CURRENT_INFO,          // 0x0e Current MSB
    BATT_AVCURRENT_INFO,        // 0x08 Average Current MSB
    BATT_STATUS_INFO,           // 0x01 Status
    BATT_ACCCURRENT_INFO,       // 0x10 Accumulated Current Register MSB
    BATT_AGE_SCALAR_INFO,       // 0x14 Age Scalar
    BATT_FULL_INFO,             // 0x16 Full Capacity MSB
    BATT_FULL40_INFO,           // 0x6A Full40 MSB
    BATT_FAMILY_CODE,           // 0x33
    BATT_GET_RSNS,              // 0x69
    BATT_ACTIVE_EMPTY_INFO,
    BATT_STANDBY_EMPTY_INFO,
    BATT_ACTIVE_EMPTY_VOLTAGE_INFO,
    BATT_GET_MANUFACTURER_INFO,
    BATT_GET_DEVICE_TYPE,
};

int ds2784_batt_get_batt_capacity(void);
int ds2784_batt_get_batt_current(void);
int ds2784_batt_set_batt_health(void);
int ds2784_batt_get_batt_temp(void);
int ds2784_batt_get_batt_voltage(void);
#endif
