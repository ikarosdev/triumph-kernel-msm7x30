/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef __MSM_BATTERY_H__
#define __MSM_BATTERY_H__

#define AC_CHG     0x00000001
#define USB_CHG    0x00000002

enum {
    BATTERY_REGISTRATION_SUCCESSFUL = 0,
    BATTERY_DEREGISTRATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
    BATTERY_MODIFICATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
    BATTERY_INTERROGATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
    BATTERY_CLIENT_TABLE_FULL = 1,
    BATTERY_REG_PARAMS_WRONG = 2,
    BATTERY_DEREGISTRATION_FAILED = 4,
    BATTERY_MODIFICATION_FAILED = 8,
    BATTERY_INTERROGATION_FAILED = 16,
    /* Client's filter could not be set because perhaps it does not exist */
    BATTERY_SET_FILTER_FAILED         = 32,
    /* Client's could not be found for enabling or disabling the individual
     * client */
    BATTERY_ENABLE_DISABLE_INDIVIDUAL_CLIENT_FAILED  = 64,
    BATTERY_LAST_ERROR = 128,
};

enum {
    BATTERY_VOLTAGE_UP = 0,
    BATTERY_VOLTAGE_DOWN,
    BATTERY_VOLTAGE_ABOVE_THIS_LEVEL,
    BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
    BATTERY_VOLTAGE_LEVEL,
    BATTERY_ALL_ACTIVITY,
    VBATT_CHG_EVENTS,
    BATTERY_VOLTAGE_UNKNOWN,
};

/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type {
    /* The charger is good      */
    CHARGER_STATUS_GOOD,
    /* The charger is bad       */
    CHARGER_STATUS_BAD,
    /* The charger is weak      */
    CHARGER_STATUS_WEAK,
    /* Invalid charger status.  */
    CHARGER_STATUS_INVALID
};

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type {
    /* The charger is removed                 */
    CHARGER_TYPE_NONE,
    /* The charger is a regular wall charger   */
    CHARGER_TYPE_WALL,
    /* The charger is a PC USB                 */
    CHARGER_TYPE_USB_PC,
    /* The charger is a wall USB charger       */
    CHARGER_TYPE_USB_WALL,
    /* The charger is a USB carkit             */
    CHARGER_TYPE_USB_CARKIT,
    /* Invalid charger hardware status.        */
    CHARGER_TYPE_INVALID
};

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type {
    /* The battery is good        */
    BATTERY_STATUS_GOOD,
    /* The battery is cold/hot    */
    BATTERY_STATUS_BAD_TEMP,
    /* The battery is bad         */
    BATTERY_STATUS_BAD,
    /* The battery is removed     */
    BATTERY_STATUS_REMOVED,     /* on v2.2 only */
    BATTERY_STATUS_INVALID_v1 = BATTERY_STATUS_REMOVED,
    /* Invalid battery status.    */
    BATTERY_STATUS_INVALID
};

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type {
    /* The battery voltage is dead/very low (less than 3.2V) */
    BATTERY_LEVEL_DEAD,
    /* The battery voltage is weak/low (between 3.2V and 3.4V) */
    BATTERY_LEVEL_WEAK,
    /* The battery voltage is good/normal(between 3.4V and 4.2V) */
    BATTERY_LEVEL_GOOD,
    /* The battery voltage is up to full (close to 4.2V) */
    BATTERY_LEVEL_FULL,
    /* Invalid battery voltage level. */
    BATTERY_LEVEL_INVALID
};

struct batt_info_interface {
    int (*get_batt_voltage)(void);
    int (*get_batt_capacity)(void);
    int (*get_batt_temp)(void);
    int (*get_batt_health)(void);
    u32 (*get_batt_status)(void);
    u32 (*get_chg_source)(void);
};

struct msm_psy_batt_pdata {
    u32 voltage_max_design;
    u32 voltage_min_design;
    u32 avail_chg_sources;
    u32 batt_technology;
    struct batt_info_interface batt_info_if;  
};

u32 msm_batt_get_batt_status(void);
u32 msm_batt_get_chg_source(void);
int msm_batt_info_not_support(void);
struct batt_info_interface* get_batt_info_if(void);
#endif
