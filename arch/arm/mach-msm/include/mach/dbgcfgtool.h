/**+===========================================================================
  File: dbgcfgtool.h

  Description:


  Note:


  Author:
        Hebbel Chao Oct-20-2009

-------------------------------------------------------------------------------
** FIHSPEC CONFIDENTIAL
** Copyright(c) 2009 FIHSPEC Corporation. All Rights Reserved.
**
** The source code contained or described herein and all documents related
** to the source code (Material) are owned by Mobinnova Technology Corporation.
** The Material is protected by worldwide copyright and trade secret laws and
** treaty provisions. No part of the Material may be used, copied, reproduced,
** modified, published, uploaded, posted, transmitted, distributed, or disclosed
** in any way without Mobinnova prior express written permission.
============================================================================+*/
#ifndef __DBGCFGTOOL_H__
#define __DBGCFGTOOL_H__

#define DEV_IOCTLID 0xD1
#define DBG_IOCTL_CMD_GET_DBGCFG_GROUP _IOWR(DEV_IOCTLID, 0, dbgcfg_ioctl_arg)
#define DBG_IOCTL_CMD_SET_DBGCFG_GROUP _IOWR(DEV_IOCTLID, 1, dbgcfg_ioctl_arg)
#define DBG_IOCTL_CMD_GET_DBGCFG_BIT   _IOWR(DEV_IOCTLID, 2, dbgcfg_ioctl_arg)
#define DBG_IOCTL_CMD_GET_ERROR_ACTION  _IOR(DEV_IOCTLID, 3, dbgcfg_ioctl_arg)
#define DBG_IOCTL_CMD_SET_ERROR_ACTION  _IOW(DEV_IOCTLID, 4, dbgcfg_ioctl_arg)
#define DBG_IOCTL_CMD_SET_DETECT_HW_RESET _IOW(DEV_IOCTLID, 5, dbgcfg_ioctl_arg)	//SW2-5-1-DbgCfgToolPorting-01+

/* Export kernel APIs */
int DbgCfgGetByBit(unsigned int bid,unsigned int *bvalue);


typedef struct
{
    union
    {
        unsigned int group_id;
        unsigned int bit_id;
    } id;
    unsigned int value;
}dbgcfg_ioctl_arg;


#define CHECK_SUM_MASK 0xFF000000
/*--------------------------------------------------------------------------* 
 * FIH debug configuration data (120 bits for config + 8 bits for checksum)
 *
 *   |-  UART_CFG  -|-     NA     -|-  MISC_CFG  -|- MODEM_CFG  -|-  CheckSum  -|
 *   |-- 32 bits  --|-- 32 bits  --|-- 32 bits  --|-- 24 bits  --|--  8 bits  --|
 *
 * This enumeration needs to synchronize with /android/system/core/include/dbgcfgtool.h
 *--------------------------------------------------------------------------*/
enum
{
    DEBUG_UART_CFG_START = 0,
    DEBUG_UARTMSG_CFG = DEBUG_UART_CFG_START,
    DEBUG_PRINTF_UARTMSG_CFG,
    DEBUG_ANDROID_UARTMSG_CFG,
    DEBUG_CPU_USAGE_CFG,
    DEBUG_RPCMSG_CFG,
    //SW-2-5-1-BH-AlogUart-00*[
    DEBUG_ANDROID_UARTMSG_MAIN_CFG,
    DEBUG_ANDROID_UARTMSG_SYSTEM_CFG,
    DEBUG_ANDROID_UARTMSG_RADIO_CFG,
    DEBUG_ANDROID_UARTMSG_EVENTS_CFG,
    //SW-2-5-1-BH-AlogUart-00*]
    DEBUG_MISC_CFG_SART = 64,
    DEBUG_DISABLE_POWER_OFF_CHARGING_CFG = DEBUG_MISC_CFG_SART,
    DEBUG_AGING_TEST_CFG,
    DEBUG_FORCE_TRIGGER_RAMDUMP_CFG,
    DEBUG_MODEM_CFG_START = 96,
    DEBUG_MODEM_LOGGER_CFG = DEBUG_MODEM_CFG_START,
    DEBUG_USB_QXDM_MODE_CFG,
    DEBUG_AUTO_START_EML_CFG,
    DEBUG_WP_DBGBUF_CFG,
    DEBUG_RAM_DUMP_TO_SD_CFG,
    MAX_DEBUG_CFG_NUM = 119		//MAX<=119
};



#endif	//__DBGCFGTOOL_H__
